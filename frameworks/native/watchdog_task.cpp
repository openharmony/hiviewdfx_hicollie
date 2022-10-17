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
#include <thread>

#include <unistd.h>

#include "hisysevent.h"
#include "xcollie_utils.h"

namespace OHOS {
namespace HiviewDFX {
WatchdogTask::WatchdogTask(std::string name, std::shared_ptr<AppExecFwk::EventHandler> handler,
    TimeOutCallback timeOutCallback, uint64_t interval)
    : name(name), task(nullptr), timeOutCallback(timeOutCallback)
{
    checker = std::make_shared<HandlerChecker>(name, handler);
    checkInterval = interval;
    nextTickTime = GetCurrentTickMillseconds();
    isTaskScheduled = false;
    isOneshotTask = false;
}

WatchdogTask::WatchdogTask(std::string name, Task&& task, uint64_t delay, uint64_t interval,  bool isOneshot)
    : name(name), task(std::move(task)), timeOutCallback(nullptr), checker(nullptr)
{
    checkInterval = interval;
    nextTickTime = GetCurrentTickMillseconds() + delay;
    isTaskScheduled = false;
    isOneshotTask = isOneshot;
}

void WatchdogTask::Run(uint64_t now)
{
    constexpr int RESET_RATIO = 2;
    if ((checkInterval != 0) && (now - nextTickTime > (RESET_RATIO * checkInterval))) {
        XCOLLIE_LOGI("checker thread may be blocked, reset next tick time."
            "now:%{public}" PRIu64 " expect:%{public}" PRIu64 " interval:%{public}" PRIu64 "",
            now, nextTickTime, checkInterval);
        nextTickTime = now;
        isTaskScheduled = false;
        return;
    }

    if (task != nullptr) {
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
    int32_t gid = getgid();
    int32_t uid = getuid();
    time_t curTime = time(nullptr);
    std::string sendMsg = std::string((ctime(&curTime) == nullptr) ? "" : ctime(&curTime)) +
        "\n" + msg + "\n";
    sendMsg += checker->GetDumpInfo();
    HiSysEvent::Write("FRAMEWORK", eventName, HiSysEvent::EventType::FAULT,
        "PID", pid,
        "TGID", gid,
        "UID", uid,
        "MODULE_NAME", name,
        "PROCESS_NAME", GetSelfProcName(),
        "MSG", sendMsg);
    XCOLLIE_LOGI("send event [FRAMEWORK,%{public}s], msg=%{public}s", eventName.c_str(), msg.c_str());
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
            SendEvent(description, "SERVICE_WARNING");
        }
    } else {
        XCOLLIE_LOGI("Watchdog happened, send event twice, and skip exiting process");
        std::string description = GetBlockDescription(checkInterval / 1000) +
            ", report twice instead of exiting process."; // 1s = 1000ms
        if (timeOutCallback != nullptr) {
            timeOutCallback(name, waitState);
        } else {
            SendEvent(description, "SERVICE_BLOCK");
            std::this_thread::sleep_for(std::chrono::seconds(3)); // after 3s exit
            _exit(0);
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
