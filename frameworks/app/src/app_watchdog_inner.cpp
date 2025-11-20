/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "app_watchdog_inner.h"
#include "app_watchdog_utils.h"

namespace OHOS {
namespace HiviewDFX {
AppWatchdogInner::AppWatchdogInner()
{
}

AppWatchdogInner::~AppWatchdogInner()
{
}

int32_t AppWatchdogInner::GetReservedTimeForLogging()
{
    bool version = AppWatchdogUtils::IsBetaVersion();
    XCOLLIE_LOGD("Set reserved time, is betaVersion: %{public}d", version);
    if (version) {
        reservedTime_ = BETA_RESERVED_TIME;
    }
    return reservedTime_;
}

void AppWatchdogInner::SetBundleInfo(const std::string& bundleName, const std::string& bundleVersion)
{
    bundleName_ = bundleName;
    bundleVersion_ = bundleVersion;
}

void AppWatchdogInner::SetSystemApp(bool isSystemApp)
{
    isSystemApp_.store(isSystemApp);
}

void AppWatchdogInner::SetAppDebug(bool isAppDebug)
{
    isAppDebug_.store(isAppDebug);
}

void AppWatchdogInner::SetForeground(bool isForeground)
{
    isForeground_.store(isForeground);
}

void AppWatchdogInner::SetScrollState(bool isScroll)
{
    isScroll_.store(isScroll);
}

void AppWatchdogInner::SetSpecifiedProcessName(const std::string& name)
{
    specifiedProcessName_ = name;
}

bool AppWatchdogInner::GetSystemApp()
{
    return isSystemApp_;
}

std::string AppWatchdogInner::GetBundleName()
{
    return bundleName_;
}

bool AppWatchdogInner::GetForeground()
{
    return isForeground_.load();
}

std::string AppWatchdogInner::GetSpecifiedProcessName()
{
    return specifiedProcessName_;
}

bool AppWatchdogInner::GetScrollState()
{
    return isScroll_.load();
}

bool AppWatchdogInner::GetAppDebug()
{
    return isAppDebug_.load();
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
