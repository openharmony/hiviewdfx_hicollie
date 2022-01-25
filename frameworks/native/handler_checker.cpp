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

int HandlerChecker::GetCheckState()
{
    if (isCompleted_) {
        taskSlow = false;
        return CheckStatus::COMPLETED;
    } else {
        if (!taskSlow) {
            taskSlow = true;
            return CheckStatus::WAITING;
        } else {
            return CheckStatus::WAITED_HALF;
        }
    }
}

std::shared_ptr<AppExecFwk::EventHandler> HandlerChecker::GetHandler() const
{
    return handler_;
}
} // end of namespace HiviewDFX
} // end of namespace OHOS