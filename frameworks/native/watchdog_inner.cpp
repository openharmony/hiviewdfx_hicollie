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

#include "watchdog_inner.h"
#include "xcollie_utils.h"

namespace OHOS {
namespace HiviewDFX {
WatchdogInner::WatchdogInner()
{
}

WatchdogInner::~WatchdogInner()
{
    Stop();
}

int WatchdogInner::AddThread(const std::string& name, std::shared_ptr<AppExecFwk::EventHandler> handler,
    unsigned int timeval)
{
    if (name.empty() || handler == nullptr) {
        XCOLLIE_LOGE("Add thread fail, invalid args!");
        return -1;
    }
    std::unique_lock<std::mutex> lock(lock_);
    SetTimeval(timeval);
    for (auto [k, v] : handlerMap_) {
        if (k == name || v->GetHandler() == handler) {
            return 0;
        }
    }
    if (handlerMap_.size() >= MAX_WATCH_NUM) {
        return -1;
    }
    auto checker = std::make_shared<HandlerChecker>(name, handler);
    if (checker == nullptr) {
        return -1;
    }
    handlerMap_.insert(std::make_pair(name, checker));
    if (threadLoop_ == nullptr) {
        threadLoop_ = std::make_unique<std::thread>(&WatchdogInner::Start, this);
        XCOLLIE_LOGI("Watchdog is running!");
    }
    XCOLLIE_LOGI("Add thread success : %{public}s, timethreshold : %{public}d", name.c_str(), timeval);
    return 0;
}

bool WatchdogInner::Start()
{
    XCOLLIE_LOGI("Run watchdog!");
    while (!isNeedStop_) {
        {
            std::unique_lock lock(lock_);
            for (auto info : handlerMap_) {
                info.second->ScheduleCheck();
            }
        }
        unsigned int interval = GetCheckInterval();
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        {
            std::unique_lock lock(lock_);
            for (auto info : handlerMap_) {
                info.second->CheckState(interval / 1000); // 1s = 1000ms
            }
        }
    }
    return true;
}

bool WatchdogInner::Stop()
{
    isNeedStop_.store(true);
    if (threadLoop_ != nullptr && threadLoop_->joinable()) {
        threadLoop_->join();
        threadLoop_ = nullptr;
    }
    return true;
}

void WatchdogInner::SetTimeval(unsigned int timeval)
{
    timeval_ = timeval;
}

unsigned int WatchdogInner::GetCheckInterval() const
{
    return timeval_ * 500; // 0.5s = 500ms
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
