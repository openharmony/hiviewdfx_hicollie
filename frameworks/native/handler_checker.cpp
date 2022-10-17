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
    if (!handler_->PostTask(f, "XCollie Watchdog Task", 0, AppExecFwk::EventQueue::Priority::IMMEDIATE)) {
        XCOLLIE_LOGE("XCollie Watchdog Task PostTask False.");
    }
}

int HandlerChecker::GetCheckState()
{
    if (isCompleted_) {
        taskSlow = false;
        return CheckStatus::COMPLETED;
    } else {
        if (!taskSlow) {
            taskSlow = true;
            return CheckStatus::WAITED_HALF;
        } else {
            return CheckStatus::WAITING;
        }
    }
}

std::string HandlerChecker::GetDumpInfo()
{
    std::string ret;
    if (handler_ != nullptr) {
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
