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

#include "watchdog.h"

#include "watchdog_inner.h"

namespace OHOS {
namespace HiviewDFX {
Watchdog::Watchdog()
{
}

Watchdog::~Watchdog()
{
}

int Watchdog::AddThread(const std::string &name, std::shared_ptr<AppExecFwk::EventHandler> handler,
    TimeOutCallback timeOutCallback, uint64_t interval)
{
    return WatchdogInner::GetInstance().AddThread(name, handler, timeOutCallback, interval);
}

int Watchdog::AddThread(const std::string &name, std::shared_ptr<AppExecFwk::EventHandler> handler,  uint64_t interval)
{
    return WatchdogInner::GetInstance().AddThread(name, handler, nullptr, interval);
}

void Watchdog::RunOneShotTask(const std::string& name, Task&& task, uint64_t delay)
{
    return WatchdogInner::GetInstance().RunOneShotTask(name, std::move(task), delay);
}

void Watchdog::RunPeriodicalTask(const std::string& name, Task&& task, uint64_t interval, uint64_t delay)
{
    return WatchdogInner::GetInstance().RunPeriodicalTask(name, std::move(task), interval, delay);
}

void Watchdog::StopWatchdog()
{
    return WatchdogInner::GetInstance().StopWatchdog();
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
