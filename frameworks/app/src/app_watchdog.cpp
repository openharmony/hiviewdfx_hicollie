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

#include "app_watchdog.h"

#include "app_watchdog_inner.h"

namespace OHOS {
namespace HiviewDFX {
AppWatchdog::AppWatchdog()
{
}

AppWatchdog::~AppWatchdog()
{
}

int32_t AppWatchdog::GetReservedTimeForLogging()
{
    return AppWatchdogInner::GetInstance().GetReservedTimeForLogging();
}

void AppWatchdog::SetBundleInfo(const std::string& bundleName, const std::string& bundleVersion)
{
    AppWatchdogInner::GetInstance().SetBundleInfo(bundleName, bundleVersion);
}

void AppWatchdog::SetSystemApp(bool isSystemApp)
{
    AppWatchdogInner::GetInstance().SetSystemApp(isSystemApp);
}

void AppWatchdog::SetAppDebug(bool isAppDebug)
{
    AppWatchdogInner::GetInstance().SetAppDebug(isAppDebug);
}

void AppWatchdog::SetForeground(bool isForeground)
{
    AppWatchdogInner::GetInstance().SetForeground(isForeground);
}

void AppWatchdog::SetScrollState(bool isScroll)
{
    AppWatchdogInner::GetInstance().SetScrollState(isScroll);
}

void AppWatchdog::SetSpecifiedProcessName(const std::string& name)
{
    AppWatchdogInner::GetInstance().SetSpecifiedProcessName(name);
}

bool AppWatchdog::GetSystemApp()
{
    return AppWatchdogInner::GetInstance().GetSystemApp();
}

std::string AppWatchdog::GetBundleName()
{
    return AppWatchdogInner::GetInstance().GetBundleName();
}

bool AppWatchdog::GetForeground()
{
    return AppWatchdogInner::GetInstance().GetForeground();
}

std::string AppWatchdog::GetSpecifiedProcessName()
{
    return AppWatchdogInner::GetInstance().GetSpecifiedProcessName();
}

bool AppWatchdog::GetScrollState()
{
    return AppWatchdogInner::GetInstance().GetScrollState();
}

bool AppWatchdog::GetAppDebug()
{
    return AppWatchdogInner::GetInstance().GetAppDebug();
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
