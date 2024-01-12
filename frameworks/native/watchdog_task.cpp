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
int64_t WatchdogTask::curId = 0;
const char* g_bboxPath = "/dev/bbox";
struct HstackVal {
    uint32_t magic;
    pid_t tid;
    char hstackLogBuff[BUFF_STACK_SIZE];
};
WatchdogTask::WatchdogTask(std::string name, std::shared_ptr<AppExecFwk::EventHandler> handler,
    TimeOutCallback timeOutCallback, uint64_t interval)
    : name(name), task(nullptr), timeOutCallback(timeOutCallback), timeout(0), func(nullptr),
      arg(nullptr), flag(0)
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
      arg(nullptr), flag(0)
{
    id = ++curId;
    checkInterval = interval;
    nextTickTime = GetCurrentTickMillseconds() + delay;
    isTaskScheduled = false;
    isOneshotTask = isOneshot;
}

WatchdogTask::WatchdogTask(std::string name, unsigned int timeout, XCollieCallback func, void *arg, unsigned int flag)
    : name(name), task(nullptr), timeOutCallback(nullptr), checker(nullptr), timeout(timeout), func(std::move(func)),
      arg(arg), flag(flag)
{
    id = ++curId;
    checkInterval = 0;
    nextTickTime = GetCurrentTickMillseconds() + timeout;
    isTaskScheduled = false;
    isOneshotTask = true;
    watchdogTid = gettid();
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
    if (flag & XCOLLIE_FLAG_RECOVERY) {
        XCOLLIE_LOGE("%{public}s blocked, after timeout %{public}llu ,process will exit", name.c_str(),
            static_cast<long long>(timeout));
        std::thread exitFunc([]() {
            std::string description = "timeout, exit...";
            WatchdogInner::LeftTimeExitProcess(description.c_str());
        });
        if (exitFunc.joinable()) {
            exitFunc.detach();
        }
    }
}

void WatchdogTask::Run(uint64_t now)
{
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
        DoCallback();
    } else if (task != nullptr) {
        task();
    } else {
        RunHandlerCheckerTask();
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

void WatchdogTask::SendEvent(const std::string &msg, const std::string &eventName) const
{
    int32_t pid = getpid();
    if (IsProcessDebug(pid)) {
        XCOLLIE_LOGI("heap dump for %{public}d, don't report.", pid);
        return;
    }
    uint32_t gid = getgid();
    uint32_t uid = getuid();
    time_t curTime = time(nullptr);
    std::string sendMsg = std::string((ctime(&curTime) == nullptr) ? "" : ctime(&curTime)) +
        "\n" + msg + "\n";
    sendMsg += checker->GetDumpInfo();
    HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, eventName, HiSysEvent::EventType::FAULT,
        "PID", pid,
        "TGID", gid,
        "UID", uid,
        "MODULE_NAME", name,
        "PROCESS_NAME", GetSelfProcName(),
        "MSG", sendMsg,
        "STACK", GetProcessStacktrace());
    XCOLLIE_LOGI("send event [FRAMEWORK,%{public}s], msg=%{public}s", eventName.c_str(), msg.c_str());
}

void WatchdogTask::SendXCollieEvent(const std::string &timerName, const std::string &keyMsg) const
{
    int32_t pid = getpid();
    uint32_t gid = getgid();
    uint32_t uid = getuid();
    time_t curTime = time(nullptr);
    std::string sendMsg = std::string((ctime(&curTime) == nullptr) ? "" : ctime(&curTime)) + "\n"+
        "timeout timer: " + timerName + "\n" +keyMsg;

    struct HstackVal val;
    if (memset_s(&val, sizeof(val), 0, sizeof(val)) != 0) {
        XCOLLIE_LOGE("memset val failed\n");
        return;
    }
    val.tid = watchdogTid;
    val.magic = MAGIC_NUM;
    int fd = open(g_bboxPath, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        XCOLLIE_LOGE("open %s failed", name.c_str());
        return;
    }
    int ret = ioctl(fd, LOGGER_GET_STACK, &val);
    close(fd);
    if (ret != 0) {
        XCOLLIE_LOGE("XCollieDumpKernel getStack failed");
    } else {
        XCOLLIE_LOGI("XCollieDumpKernel buff is %{public}s", val.hstackLogBuff);
    }
    
    HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, "SERVICE_TIMEOUT", HiSysEvent::EventType::FAULT,
        "PID", pid,
        "TGID", gid,
        "UID", uid,
        "MODULE_NAME", timerName,
        "PROCESS_NAME", GetSelfProcName(),
        "MSG", sendMsg,
        "STACK", GetProcessStacktrace()+ "\n"+ (ret != 0 ? "" : val.hstackLogBuff));
    XCOLLIE_LOGI("send event [FRAMEWORK,SERVICE_TIMEOUT], msg=%{public}s", keyMsg.c_str());
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
            name.compare(IPC_FULL) == 0 ? SendEvent(description, IPC_FULL) :
                SendEvent(description, "SERVICE_BLOCK");
            // peer binder log is collected in hiview asynchronously
            // if blocked process exit early, binder blocked state will change
            // thus delay exit and let hiview have time to collect log.
            WatchdogInner::LeftTimeExitProcess(description.c_str());
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
} // end of namespace HiviewDFX
} // end of namespace OHOS
