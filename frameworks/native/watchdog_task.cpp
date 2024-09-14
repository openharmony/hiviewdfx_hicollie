/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "watchdog_task.h"

#include <cinttypes>
#include <ctime>
#include <cstdio>
#include <securec.h>
#include <thread>

#include <fcntl.h>
#include <dlfcn.h>
#include <unistd.h>

#include "backtrace_local.h"
#include "musl_preinit_common.h"
#include "watchdog_inner.h"
#include "xcollie_define.h"
#include "xcollie_utils.h"

#ifdef HISYSEVENT_ENABLE
#include "hisysevent.h"
#endif

namespace OHOS {
namespace HiviewDFX {
namespace {
static const int COUNT_LIMIT_NUM_MAX_RATIO = 2;
static const int TIME_LIMIT_NUM_MAX_RATIO = 2;
static const int UID_TYPE_THRESHOLD = 20000;
const int BUFF_STACK_SIZE = 20 * 1024;
constexpr int32_t SAMGR_INIT_UID = 5555;
constexpr const char* CORE_PROCS[] = {
    "anco_service_br", "aptouch_daemon", "foundation", "init", "multimodalinput", "ohos.sceneboard", "render_service"
};
}
int64_t WatchdogTask::curId = 0;
WatchdogTask::WatchdogTask(std::string name, std::shared_ptr<AppExecFwk::EventHandler> handler,
    TimeOutCallback timeOutCallback, uint64_t interval)
    : name(name), task(nullptr), timeOutCallback(timeOutCallback), timeout(0), func(nullptr),
      arg(nullptr), flag(0), timeLimit(0), countLimit(0)
{
    id = ++curId;
    checker = std::make_shared<HandlerChecker>(name, handler);
    checkInterval = interval;
    nextTickTime = GetCurrentTickMillseconds();
    isTaskScheduled = false;
    isOneshotTask = false;
}

WatchdogTask::WatchdogTask(std::string name, Task&& task, uint64_t delay, uint64_t interval,  bool isOneshot)
    : name(name), task(std::move(task)), timeOutCallback(nullptr), checker(nullptr), timeout(0), func(nullptr),
      arg(nullptr), flag(0), watchdogTid(0), timeLimit(0), countLimit(0)
{
    id = ++curId;
    checkInterval = interval;
    nextTickTime = GetCurrentTickMillseconds() + delay;
    isTaskScheduled = false;
    isOneshotTask = isOneshot;
}

WatchdogTask::WatchdogTask(std::string name, unsigned int timeout, XCollieCallback func, void *arg, unsigned int flag)
    : name(name), task(nullptr), timeOutCallback(nullptr), checker(nullptr), timeout(timeout), func(std::move(func)),
      arg(arg), flag(flag), timeLimit(0), countLimit(0)
{
    id = ++curId;
    checkInterval = 0;
    nextTickTime = GetCurrentTickMillseconds() + timeout;
    isTaskScheduled = false;
    isOneshotTask = true;
    watchdogTid = getproctid();
}

WatchdogTask::WatchdogTask(std::string name, unsigned int timeLimit, int countLimit)
    : name(name), task(nullptr), timeOutCallback(nullptr), timeout(0), func(nullptr), arg(nullptr), flag(0),
      isTaskScheduled(false), isOneshotTask(false), watchdogTid(0), timeLimit(timeLimit), countLimit(countLimit)
{
    id = ++curId;
    checkInterval = timeLimit / TIME_LIMIT_NUM_MAX_RATIO;
    nextTickTime = GetCurrentTickMillseconds();
}

void WatchdogTask::DoCallback()
{
    if (func) {
        XCOLLIE_LOGE("XCollieInner::DoTimerCallback %{public}s callback", name.c_str());
        func(arg);
    }
    if (WatchdogInner::GetInstance().IsCallbackLimit(flag)) {
        XCOLLIE_LOGE("Too many callback triggered in a short time, %{public}s skip", name.c_str());
        return;
    }
    if (flag & XCOLLIE_FLAG_LOG) {
        /* send to freezedetetor */
        std::string msg = "timeout: " + name + " to check " + std::to_string(timeout) + "ms ago";
        SendXCollieEvent(name, msg);
    }
    if (getuid() > UID_TYPE_THRESHOLD) {
        XCOLLIE_LOGI("check uid is app, do not exit");
        return;
    }
    if (flag & XCOLLIE_FLAG_RECOVERY) {
        XCOLLIE_LOGE("%{public}s blocked, after timeout %{public}llu ,process will exit", name.c_str(),
            static_cast<long long>(timeout));
        std::thread exitFunc([]() {
            std::string description = "timeout, exit...";
            WatchdogInner::LeftTimeExitProcess(description);
        });
        if (exitFunc.joinable()) {
            exitFunc.detach();
        }
    }
}

void WatchdogTask::Run(uint64_t now)
{
    if (countLimit > 0) {
        TimerCountTask();
        return;
    }

    constexpr int resetRatio = 2;
    if ((checkInterval != 0) && (now - nextTickTime > (resetRatio * checkInterval))) {
        XCOLLIE_LOGI("checker thread may be blocked, reset next tick time."
            "now:%{public}" PRIu64 " expect:%{public}" PRIu64 " interval:%{public}" PRIu64 "",
            now, nextTickTime, checkInterval);
        nextTickTime = now;
        isTaskScheduled = false;
        return;
    }

    if (timeout != 0) {
        if (IsMemHookOn()) {
            return;
        }
        DoCallback();
    } else if (task != nullptr) {
        task();
    } else {
        RunHandlerCheckerTask();
    }
}

void WatchdogTask::TimerCountTask()
{
    int size = static_cast<int>(triggerTimes.size());
    if (size < countLimit) {
        return;
    }
    XCOLLIE_LOGD("timeLimit : %{public}" PRIu64 ", countLimit : %{public}d, triggerTimes size : %{public}d",
        timeLimit, countLimit, size);

    while (size >= countLimit) {
        uint64_t timeInterval = triggerTimes[size -1] - triggerTimes[size - countLimit];
        if (timeInterval < timeLimit) {
            std::string sendMsg = name + " occured " + std::to_string(countLimit) + " times in " +
                std::to_string(timeInterval) + " ms, " + message;
#ifdef HISYSEVENT_ENABLE
            HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, name, HiSysEvent::EventType::FAULT,
                "PID", getprocpid(), "PROCESS_NAME", GetSelfProcName(), "MSG", sendMsg);
#else
       XCOLLIE_LOGI("hisysevent not exists");
#endif
            triggerTimes.clear();
            return;
        }
        size--;
    }

    if (triggerTimes.size() > static_cast<unsigned long>(countLimit * COUNT_LIMIT_NUM_MAX_RATIO)) {
        triggerTimes.erase(triggerTimes.begin(), triggerTimes.end() - countLimit);
    }
}

void WatchdogTask::RunHandlerCheckerTask()
{
    if (checker == nullptr) {
        return;
    }

    if (!isTaskScheduled) {
        checker->ScheduleCheck();
        isTaskScheduled = true;
    } else {
        if (EvaluateCheckerState() == CheckStatus::COMPLETED) {
            // allow next check
            isTaskScheduled = false;
        }
    }
}

void WatchdogTask::SendEvent(const std::string &msg, const std::string &eventName)
{
    int32_t pid = getprocpid();
    if (IsProcessDebug(pid)) {
        XCOLLIE_LOGI("heap dump or debug for %{public}d, don't report.", pid);
        return;
    }
    uint32_t gid = getgid();
    uint32_t uid = getuid();
    time_t curTime = time(nullptr);
    std::string sendMsg = std::string((ctime(&curTime) == nullptr) ? "" : ctime(&curTime)) +
        "\n" + msg + "\n";
    sendMsg += checker->GetDumpInfo();

    watchdogTid = pid;
    std::string tidFrontStr = "Thread ID = ";
    std::string tidRearStr = ") is running";
    std::size_t frontPos = sendMsg.find(tidFrontStr);
    std::size_t rearPos = sendMsg.find(tidRearStr);
    if (frontPos != std::string::npos && rearPos != std::string::npos) {
        size_t tidLength = rearPos - frontPos - tidFrontStr.length();
        if (tidLength > 0 && tidLength < std::to_string(INT32_MAX).length()) {
            std::string tidStr = sendMsg.substr(frontPos + tidFrontStr.length(), tidLength);
            if (std::all_of(std::begin(tidStr), std::end(tidStr), [] (const char &c) {
                return isdigit(c);
            })) {
                watchdogTid = std::stoi(tidStr);
            }
        }
    }

#ifdef HISYSEVENT_ENABLE
    int ret = HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, eventName, HiSysEvent::EventType::FAULT,
        "PID", pid,
        "TID", watchdogTid,
        "TGID", gid,
        "UID", uid,
        "MODULE_NAME", name,
        "PROCESS_NAME", GetSelfProcName(),
        "MSG", sendMsg);
    XCOLLIE_LOGI("hisysevent write result=%{public}d, send event [FRAMEWORK,%{public}s], msg=%{public}s",
        ret, eventName.c_str(), msg.c_str());
#else
       XCOLLIE_LOGI("hisysevent not exists");
#endif
}

void WatchdogTask::SendXCollieEvent(const std::string &timerName, const std::string &keyMsg) const
{
    XCOLLIE_LOGD("SendXCollieEvent start");
    int32_t pid = getprocpid();
    if (IsProcessDebug(pid)) {
        XCOLLIE_LOGI("heap dump or debug for %{public}d, don't report.", pid);
        return;
    }
    uint32_t gid = getgid();
    uint32_t uid = getuid();
    time_t curTime = time(nullptr);
    std::string sendMsg = std::string((ctime(&curTime) == nullptr) ? "" : ctime(&curTime)) + "\n"+
        "timeout timer: " + timerName + "\n" +keyMsg;

    std::string userStack = "";
    if (uid == SAMGR_INIT_UID) {
        XCOLLIE_LOGD("DumpUserStack dump init stack start");
        if (!GetBacktraceStringByTid(userStack, 1, 0, true)) {
            XCOLLIE_LOGE("get tid:1 BacktraceString failed");
        }
        XCOLLIE_LOGD("DumpUserStack dump init stack end");
    }

    std::string eventName = "APP_HICOLLIE";
    std::string processName = GetSelfProcName();
    std::string stack = "";
    if (uid <= UID_TYPE_THRESHOLD) {
        eventName = std::find(std::begin(CORE_PROCS), std::end(CORE_PROCS), processName) != std::end(CORE_PROCS) ?
            "SERVICE_TIMEOUT" : "SERVICE_TIMEOUT_WARNING";
        stack = GetProcessStacktrace();
    } else if (!GetBacktraceStringByTid(stack, watchdogTid, 0, true)) {
        XCOLLIE_LOGE("get tid:%{public}d BacktraceString failed", watchdogTid);
    }
#ifdef HISYSEVENT_ENABLE
    int result = HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, eventName, HiSysEvent::EventType::FAULT, "PID", pid,
        "TID", watchdogTid, "TGID", gid, "UID", uid, "MODULE_NAME", timerName, "PROCESS_NAME", processName,
        "MSG", sendMsg, "STACK", stack + "\n"+ userStack);
    XCOLLIE_LOGI("hisysevent write result=%{public}d, send event [FRAMEWORK,%{public}s], "
        "msg=%{public}s", result, eventName.c_str(), keyMsg.c_str());
#else
       XCOLLIE_LOGI("hisysevent not exists");
#endif
}

int WatchdogTask::EvaluateCheckerState()
{
    int waitState = checker->GetCheckState();
    if (waitState == CheckStatus::COMPLETED) {
        return waitState;
    } else if (waitState == CheckStatus::WAITED_HALF) {
        XCOLLIE_LOGI("Watchdog half-block happened, send event");
        std::string description = GetBlockDescription(checkInterval / 1000); // 1s = 1000ms
        if (timeOutCallback != nullptr) {
            timeOutCallback(name, waitState);
        } else {
            if (IsMemHookOn()) {
                return waitState;
            }
            if (name.compare(IPC_FULL) != 0) {
                SendEvent(description, "SERVICE_WARNING");
            }
        }
    } else {
        XCOLLIE_LOGI("Watchdog happened, send event twice.");
        std::string description = GetBlockDescription(checkInterval / 1000) +
            ", report twice instead of exiting process."; // 1s = 1000ms
        if (timeOutCallback != nullptr) {
            timeOutCallback(name, waitState);
        } else {
            if (IsMemHookOn()) {
                return waitState;
            }
            if (name.compare(IPC_FULL) == 0) {
                SendEvent(description, IPC_FULL);
            } else {
                SendEvent(description, "SERVICE_BLOCK");
            }
            // peer binder log is collected in hiview asynchronously
            // if blocked process exit early, binder blocked state will change
            // thus delay exit and let hiview have time to collect log.
            WatchdogInner::KillPeerBinderProcess(description);
        }
    }
    return waitState;
}

std::string WatchdogTask::GetBlockDescription(uint64_t interval)
{
    std::string desc = "Watchdog: thread(";
    desc += name;
    desc += ") blocked " + std::to_string(interval) + "s";
    return desc;
}

bool WatchdogTask::IsMemHookOn()
{
    bool hookFlag = __get_global_hook_flag();
    if (hookFlag) {
        XCOLLIE_LOGI("memory hook is on, timeout task will skip");
    }
    return hookFlag;
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
