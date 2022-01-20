/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "handler_checker.h"
#include <unistd.h>

#include "hisysevent.h"
#include "xcollie_utils.h"

namespace OHOS {
namespace HiviewDFX {
void HandlerChecker::ScheduleCheck()
{
    if (!isCompleted_ || handler_ == nullptr) {
        return;
    }
    isCompleted_.store(false);
    auto f = [this] () {
        this->isCompleted_.store(true);
    };
    handler_->PostTask(f, "task", 0);
}

void HandlerChecker::CheckState(const unsigned int& interval)
{
    if (isCompleted_) {
        XCOLLIE_LOGI("task completed");
        taskSlow = false;
        return;
    } else {
        if (!taskSlow) {
            taskSlow = true;
            SendEvent(getpid(), name_, "watchdog: thread " + name_ + " blocked " + std::to_string(interval) + "s");
        } else {
            XCOLLIE_LOGI("Watchdog happened, killing process");
            _exit(1);
        }
    }
}

std::shared_ptr<AppExecFwk::EventHandler> HandlerChecker::GetHandler() const
{
    return handler_;
}

void HandlerChecker::SendEvent(int tid, const std::string &threadName, const std::string &keyMsg) const
{
    pid_t pid = getpid();
    gid_t gid = getgid();
    time_t curTime = time(nullptr);
    std::string sendMsg = std::string((ctime(&curTime) == nullptr) ? "" : ctime(&curTime)) + "\n" +
        keyMsg + "\ntimeout tid: " + std::to_string(tid) +
        "\ntimeout function: " + threadName;
    HiSysEvent::Write("FRAMEWORK", "WATCHDOG", HiSysEvent::EventType::FAULT,
        "PID", pid, "TGID", gid, "MSG", sendMsg);
    XCOLLIE_LOGI("send event FRAMEWORK_WATCHDOG, msg=%s", keyMsg.c_str());
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
