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
#include "hisysevent.h"
#include "watchdog_inner.h"
#include "xcollie_define.h"
#include "xcollie_utils.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr const char* BBOX_PATH = "/dev/bbox";
#ifdef SUSPEND_CHECK_ENABLE
constexpr const char* LAST_SUSPEND_TIME_PATH = "/sys/power/last_sr";
constexpr double SUSPEND_TIME_RATIO = 0.2;
#endif
constexpr int COUNT_LIMIT_NUM_MAX_RATIO = 2;
constexpr int TIME_LIMIT_NUM_MAX_RATIO = 2;
constexpr int UID_TYPE_THRESHOLD = 20000;
constexpr int BUFF_STACK_SIZE = 20 * 1024;
constexpr const char* CORE_PROCS[] = {
    "anco_service_broker", "aptouch_daemon", "foundation", "init",
    "multimodalinput", "com.ohos.sceneboard", "render_service"
};
}
int64_t WatchdogTask::curId = 0;
struct HstackVal {
    uint32_t magic;
    pid_t tid;
    char hstackLogBuff[BUFF_STACK_SIZE];
};
WatchdogTask::WatchdogTask(std::string name, std::shared_ptr<AppExecFwk::EventHandler> handler,
    TimeOutCallback timeOutCallback, uint64_t interval)
    : name(name), task(nullptr), timeOutCallback(timeOutCallback), timeout(0), func(nullptr), arg(nullptr), flag(0),
      watchdogTid(0), timeLimit(0), countLimit(0), bootTimeStart(0), monoTimeStart(0), reportCount(0)
{
    id = ++curId;
    checker = std::make_shared<HandlerChecker>(name, handler);
    checkInterval = interval;
    nextTickTime = GetCurrentTickMillseconds();
    isOneshotTask = false;
}

WatchdogTask::WatchdogTask(uint64_t interval, IpcFullCallback func, void *arg, unsigned int flag)
    : name(IPC_FULL_TASK), task(nullptr), timeOutCallback(nullptr), timeout(0), func(std::move(func)), arg(arg),
      flag(flag), watchdogTid(0), timeLimit(0), countLimit(0), bootTimeStart(0), monoTimeStart(0), reportCount(0)
{
    id = ++curId;
    checker = std::make_shared<HandlerChecker>(IPC_FULL_TASK, nullptr);
    checkInterval = interval;
    nextTickTime = GetCurrentTickMillseconds();
    isOneshotTask = false;
}

WatchdogTask::WatchdogTask(std::string name, Task&& task, uint64_t delay, uint64_t interval,  bool isOneshot)
    : name(name), task(std::move(task)), timeOutCallback(nullptr), checker(nullptr), timeout(0), func(nullptr),
      arg(nullptr), flag(0), watchdogTid(0), timeLimit(0), countLimit(0), bootTimeStart(0), monoTimeStart(0),
      reportCount(0)
{
    id = ++curId;
    checkInterval = interval;
    nextTickTime = GetCurrentTickMillseconds() + delay;
    isOneshotTask = isOneshot;
}

WatchdogTask::WatchdogTask(std::string name, unsigned int timeout, XCollieCallback func, void *arg, unsigned int flag)
    : name(name), task(nullptr), timeOutCallback(nullptr), checker(nullptr), timeout(timeout), func(std::move(func)),
      arg(arg), flag(flag), timeLimit(0), countLimit(0), reportCount(0)
{
    id = ++curId;
    checkInterval = 0;
    nextTickTime = GetCurrentTickMillseconds() + timeout;
    isOneshotTask = true;
    watchdogTid = getproctid();
    CalculateTimes(bootTimeStart, monoTimeStart);
}

WatchdogTask::WatchdogTask(std::string name, unsigned int timeLimit, int countLimit)
    : name(name), task(nullptr), timeOutCallback(nullptr), timeout(0), func(nullptr), arg(nullptr), flag(0),
      isOneshotTask(false), watchdogTid(0), timeLimit(timeLimit), countLimit(countLimit), bootTimeStart(0),
      monoTimeStart(0), reportCount(0)
{
    id = ++curId;
    checkInterval = timeLimit / TIME_LIMIT_NUM_MAX_RATIO;
    nextTickTime = GetCurrentTickMillseconds();
}

void WatchdogTask::DoCallback()
{
    std::string faultTimeStr = "\nFault time:" + FormatTime("%Y/%m/%d-%H:%M:%S") + "\n";
    if (func) {
        XCOLLIE_LOGE("XCollieInner::DoTimerCallback %{public}s callback", name.c_str());
        func(arg);
    }
    if (WatchdogInner::GetInstance().IsCallbackLimit(flag)) {
        XCOLLIE_LOGE("Too many callback triggered in a short time, %{public}s skip", name.c_str());
        return;
    }
    if (flag & XCOLLIE_FLAG_LOG) {
        /* send to freezedetector */
        std::string msg = "timeout: " + name + " to check " + std::to_string(timeout) + "ms ago";
        SendXCollieEvent(name, msg, faultTimeStr);
    }
    if (getuid() > UID_TYPE_THRESHOLD) {
        XCOLLIE_LOGI("check uid is app, do not exit");
        return;
    }
    if (flag & XCOLLIE_FLAG_RECOVERY) {
        XCOLLIE_LOGE("%{public}s blocked, after timeout %{public}llu ,process will exit", name.c_str(),
            static_cast<long long>(timeout));
        std::thread exitFunc([] {
            std::string description = "timeout, exit...";
            WatchdogInner::LeftTimeExitProcess(description);
        });
        if (exitFunc.joinable()) {
            exitFunc.detach();
        }
    }
}

#ifdef SUSPEND_CHECK_ENABLE
bool WatchdogTask::ShouldSkipCheckForSuspend(uint64_t &now, double &lastSuspendStartTime, double &lastSuspendEndTime)
{
    if (lastSuspendStartTime < 0 || lastSuspendEndTime < 0) {
        return false;
    }
    if (lastSuspendStartTime > lastSuspendEndTime) {
        return true;
    }
    return (lastSuspendEndTime - lastSuspendStartTime) > (SUSPEND_TIME_RATIO * checkInterval) &&
        (static_cast<double>(now) - lastSuspendStartTime) < checkInterval;
}
#endif

void WatchdogTask::Run(uint64_t now)
{
    if (countLimit > 0) {
        TimerCountTask();
        return;
    }
#ifdef SUSPEND_CHECK_ENABLE
    auto [lastSuspendStartTime, lastSuspendEndTime] = GetSuspendTime(LAST_SUSPEND_TIME_PATH, now);
    if (ShouldSkipCheckForSuspend(now, lastSuspendStartTime, lastSuspendEndTime)) {
        XCOLLIE_LOGI("in suspend status, ship check, reset next tick time.interval:%{public}" PRIu64
            " now:%{public}" PRIu64 " lastSuspendStartTime:%{public}f lastSuspendEndTime:%{public}f",
            checkInterval, now, lastSuspendStartTime, lastSuspendEndTime);
        return;
    }
#endif
    constexpr int resetRatio = 2;
    if ((checkInterval != 0) && (now - nextTickTime > (resetRatio * checkInterval))) {
        XCOLLIE_LOGI("checker thread may be blocked, reset next tick time."
            "now:%{public}" PRIu64 " expect:%{public}" PRIu64 " interval:%{public}" PRIu64 "",
            now, nextTickTime, checkInterval);
        nextTickTime = now;
        return;
    }

    if (timeout != 0) {
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
    if (checker) {
        EvaluateCheckerState();
        // While state is completed, check!
        checker->ScheduleCheck();
    }
}

void WatchdogTask::SendEvent(const std::string &msg, const std::string &eventName, const std::string& faultTimeStr)
{
    int32_t pid = getprocpid();
    if (IsProcessDebug(pid)) {
        XCOLLIE_LOGI("heap dump or debug for %{public}d, don't report.", pid);
        return;
    }
    uint32_t gid = getgid();
    uint32_t uid = getuid();
    time_t curTime = time(nullptr);
    std::string sendMsg = std::string((ctime(&curTime) == nullptr) ? "" : ctime(&curTime)) + "\n" + msg + "\n";
    sendMsg += checker->GetDumpInfo();

    watchdogTid = pid;
    std::string tidFrontStr = "Thread ID = ";
    std::string tidRearStr = ") is running";
    std::size_t frontPos = sendMsg.find(tidFrontStr);
    std::size_t rearPos = sendMsg.find(tidRearStr);
    std::size_t startPos = frontPos + tidFrontStr.length();
    if (frontPos != std::string::npos && rearPos != std::string::npos && rearPos > startPos) {
        size_t tidLength = rearPos - startPos;
        if (tidLength < std::to_string(INT32_MAX).length()) {
            std::string tidStr = sendMsg.substr(startPos, tidLength);
            if (std::all_of(std::begin(tidStr), std::end(tidStr), [] (const char &c) {
                return isdigit(c);
            })) {
                watchdogTid = std::stoi(tidStr);
            }
        }
    }

    sendMsg += faultTimeStr;
#ifdef HISYSEVENT_ENABLE
    std::string processName = GetSelfProcName();
    std::string moduleName = (name == IPC_FULL_TASK) ? (processName + "_" + name) : name;
    int ret = HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, eventName, HiSysEvent::EventType::FAULT,
        "PID", pid, "TID", watchdogTid, "TGID", gid, "UID", uid, "MODULE_NAME", moduleName,
        "PROCESS_NAME", processName, "MSG", sendMsg, "STACK", GetProcessStacktrace());
    if (ret == ERR_OVER_SIZE) {
        std::string stack;
        GetBacktraceStringByTid(stack, watchdogTid, 0, true);
        ret = HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, eventName, HiSysEvent::EventType::FAULT,
            "PID", pid, "TID", watchdogTid, "TGID", gid, "UID", uid, "MODULE_NAME", moduleName,
            "PROCESS_NAME", processName, "MSG", sendMsg, "STACK", stack);
    }

    XCOLLIE_LOGI("hisysevent write result=%{public}d, send event [FRAMEWORK,%{public}s], msg=%{public}s",
        ret, eventName.c_str(), msg.c_str());
#else
       XCOLLIE_LOGI("hisysevent not exists");
#endif
}

void WatchdogTask::DumpKernelStack(struct HstackVal& val, int& ret) const
{
    int fd = open(BBOX_PATH, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        XCOLLIE_LOGE("open %{public}s failed", BBOX_PATH);
        return;
    }
    ret = ioctl(fd, LOGGER_GET_STACK, &val);
    close(fd);
    if (ret != 0) {
        XCOLLIE_LOGE("XCollieDumpKernel getStack failed");
    } else {
        XCOLLIE_LOGI("XCollieDumpKernel buff is %{public}s", val.hstackLogBuff);
    }
}

void WatchdogTask::SendXCollieEvent(const std::string &timerName, const std::string &keyMsg,
    const std::string& faultTimeStr) const
{
    int32_t pid = getprocpid();
    if (IsProcessDebug(pid)) {
        XCOLLIE_LOGI("heap dump or debug for %{public}d, don't report.", pid);
        return;
    }
    uint32_t gid = getgid();
    uint32_t uid = getuid();
    time_t curTime = time(nullptr);
    std::string sendMsg = std::string((ctime(&curTime) == nullptr) ? "" : ctime(&curTime)) + "\n" +
        "timeout timer: " + timerName + "\n" + keyMsg + faultTimeStr;

    struct HstackVal val;
    if (memset_s(&val, sizeof(val), 0, sizeof(val)) != 0) {
        XCOLLIE_LOGE("memset val failed\n");
        return;
    }
    val.tid = watchdogTid;
    val.magic = MAGIC_NUM;
    int ret = 0;
    DumpKernelStack(val, ret);
    std::string eventName = "APP_HICOLLIE";
    std::string stack;
    std::string processName = GetSelfProcName();
    if (uid <= UID_TYPE_THRESHOLD) {
        eventName = (std::find(std::begin(CORE_PROCS), std::end(CORE_PROCS), processName) != std::end(CORE_PROCS) &&
            (flag & XCOLLIE_FLAG_RECOVERY))
            ? "SERVICE_TIMEOUT"
            : "SERVICE_TIMEOUT_WARNING";
        stack = GetProcessStacktrace();
    } else if (!GetBacktraceStringByTid(stack, watchdogTid, 0, true)) {
        XCOLLIE_LOGE("get tid:%{public}d BacktraceString failed", watchdogTid);
    }
#ifdef HISYSEVENT_ENABLE
    int result = HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, eventName, HiSysEvent::EventType::FAULT, "PID", pid,
        "TID", watchdogTid, "TGID", gid, "UID", uid, "MODULE_NAME", timerName, "PROCESS_NAME", processName,
        "MSG", sendMsg, "STACK", stack + "\n"+ (ret != 0 ? "" : val.hstackLogBuff), "SPECIFICSTACK_NAME",
            WatchdogInner::GetInstance().GetSpecifiedProcessName(), "TASK_NAME", name);
    XCOLLIE_LOGI("hisysevent write result=%{public}d, send event [FRAMEWORK,%{public}s], "
        "msg=%{public}s", result, eventName.c_str(), keyMsg.c_str());
#else
       XCOLLIE_LOGI("hisysevent not exists");
#endif
}

void WatchdogTask::EvaluateCheckerState()
{
    int waitState = checker->GetCheckState();
    if (waitState == CheckStatus::COMPLETED) {
        reportCount = 0;
        return;
    }
    std::string faultTimeStr = "\nFault time:" + FormatTime("%Y/%m/%d-%H:%M:%S") + "\n";
    if (func) { // only ipc full exec func
        func(arg);
        flag = (flag & XCOLLIE_FLAG_LOG) ? XCOLLIE_FLAG_LOG : XCOLLIE_FLAG_NOOP;
    }

    XCOLLIE_LOGI("%{public}s block happened, waitState = %{public}d", name.c_str(), waitState);
    if (timeOutCallback) {
        timeOutCallback(name, waitState);
        return;
    }
    std::string description = GetBlockDescription(checkInterval / TO_MILLISECOND_MULTPLE);
    if (waitState == CheckStatus::WAITED_HALF) {
        description += "\nReportCount = " + std::to_string(++reportCount);
        if (name != IPC_FULL_TASK) {
            SendEvent(description, "SERVICE_WARNING", faultTimeStr);
        } else if (flag & XCOLLIE_FLAG_LOG) {
            SendEvent(description, "IPC_FULL_WARNING", faultTimeStr);
        }
    } else {
        description += ", report twice instead of exiting process.\nReportCount = " + std::to_string(++reportCount);
        if (name != IPC_FULL_TASK) {
            SendEvent(description, "SERVICE_BLOCK", faultTimeStr);
        } else if (flag & XCOLLIE_FLAG_LOG) {
            SendEvent(description, "IPC_FULL", faultTimeStr);
        }
        // peer binder log is collected in hiview asynchronously
        // if blocked process exit early, binder blocked state will change
        // thus delay exit and let hiview have time to collect log.
        if ((name != IPC_FULL_TASK) || (flag & XCOLLIE_FLAG_RECOVERY)) {
            WatchdogInner::KillPeerBinderProcess(description);
        }
    }
}

std::string WatchdogTask::GetBlockDescription(uint64_t interval)
{
    std::string desc = "Watchdog: thread(" + name + ") blocked " + std::to_string(interval) + "s";
    return desc;
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
