/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#include "ipc_skeleton.h"
#include "musl_preinit_common.h"
#include "xcollie_utils.h"

namespace OHOS {
namespace HiviewDFX {
void HandlerChecker::ScheduleCheck()
{
    if (!handler_) {
        return;
    }

    bool expected = true;
    if (!isCompleted_.compare_exchange_strong(expected, false)) {
        return;
    }

    auto weak = weak_from_this();
    auto checkTask = [weak]() {
        auto self = weak.lock();
        if (self) {
            if (self->name_ == IPC_FULL_TASK) {
                if (__get_global_hook_flag() && __get_hook_flag()) {
                    __set_hook_flag(false);
                }
                IPCDfx::BlockUntilThreadAvailable();
            }
            self->isCompleted_.store(true);
        }
    };

    std::string taskName = (name_ == IPC_FULL_TASK) ? "XCollie IpcCheck Task" : "XCollie Watchdog Task";
    if (!handler_->PostTask(checkTask, taskName, 0, priority_)) {
        XCOLLIE_LOGE("post %{public}s false.", taskName.c_str());
    }
}

int HandlerChecker::GetCheckState()
{
    if (isCompleted_.load()) {
        taskSlow_ = false;
        return CheckStatus::COMPLETED;
    }
    if (!taskSlow_) {
        taskSlow_ = true;
        return CheckStatus::WAITED_HALF;
    }
    return CheckStatus::WAITING;
}

std::string HandlerChecker::GetDumpInfo()
{
    std::string ret;
    if (handler_) {
        HandlerDumper handlerDumper;
        handler_->Dump(handlerDumper);
        ret = handlerDumper.GetDumpInfo();
    }
    return ret;
}


std::shared_ptr<AppExecFwk::EventHandler> HandlerChecker::GetHandler() const
{
    return handler_;
}

void HandlerDumper::Dump(const std::string &message)
{
    XCOLLIE_LOGD("message is %{public}s", message.c_str());
    dumpInfo_ += message;
}

std::string HandlerDumper::GetTag()
{
    return "";
}

std::string HandlerDumper::GetDumpInfo()
{
    return dumpInfo_;
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
