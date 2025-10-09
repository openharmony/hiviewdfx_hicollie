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

void Watchdog::InitFfrtWatchdog()
{
    return WatchdogInner::GetInstance().InitFfrtWatchdog();
}

void Watchdog::SetBundleInfo(const std::string& bundleName, const std::string& bundleVersion)
{
    return WatchdogInner::GetInstance().SetBundleInfo(bundleName, bundleVersion);
}

void Watchdog::SetSystemApp(bool isSystemApp)
{
    return WatchdogInner::GetInstance().SetSystemApp(isSystemApp);
}

void Watchdog::SetForeground(const bool& isForeground)
{
    return WatchdogInner::GetInstance().SetForeground(isForeground);
}

bool Watchdog::GetForeground()
{
    return WatchdogInner::GetInstance().GetForeground();
}

void Watchdog::RemovePeriodicalTask(const std::string& name)
{
    WatchdogInner::GetInstance().RemoveInnerTask(name);
}

void Watchdog::RemoveThread(const std::string& name)
{
    WatchdogInner::GetInstance().RemoveInnerTask(name);
}

void Watchdog::InitMainLooperWatcher(WatchdogBeginFunc* beginFunc, WatchdogEndFunc* endFunc)
{
    WatchdogInner::GetInstance().InitMainLooperWatcher(beginFunc, endFunc);
}

void Watchdog::SetAppDebug(bool isAppDebug)
{
    WatchdogInner::GetInstance().SetAppDebug(isAppDebug);
}

bool Watchdog::GetAppDebug()
{
    return WatchdogInner::GetInstance().GetAppDebug();
}

int Watchdog::SetEventConfig(std::map<std::string, std::string> paramsMap)
{
    return WatchdogInner::GetInstance().SetEventConfig(paramsMap);
}

int Watchdog::ConfigEventPolicy(std::map<std::string, std::string> paramsMap)
{
    return WatchdogInner::GetInstance().ConfigEventPolicy(paramsMap);
}

void Watchdog::SetSpecifiedProcessName(const std::string& name)
{
    WatchdogInner::GetInstance().SetSpecifiedProcessName(name);
}

void Watchdog::SetScrollState(bool isScroll)
{
    WatchdogInner::GetInstance().SetScrollState(isScroll);
}

void Watchdog::StartSample(int duration, int interval)
{
    WatchdogInner::GetInstance().StartSample(duration, interval);
}

std::string Watchdog::StopSample(int sampleCount)
{
    return WatchdogInner::GetInstance().StopSample(sampleCount);
}

void Watchdog::GetSamplerResult(uint64_t &samplerStartTime, uint64_t &samplerFinishTime, int32_t &samplerCount)
{
    SamplerResult result = WatchdogInner::GetInstance().GetSamplerResult();
    samplerStartTime = result.samplerStartTime;
    samplerFinishTime = result.samplerFinishTime;
    samplerCount = result.samplerCount;
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
