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

#ifndef RELIABILITY_APP_WATCHDOG_INNER_H
#define RELIABILITY_APP_WATCHDOG_INNER_H

#include <atomic>
#include <string>
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
    constexpr int DEFAULT_RESERVED_TIME = 3500; // 3.5s
    constexpr int BETA_RESERVED_TIME = 6000; // 6s
}
class AppWatchdogInner : public Singleton<AppWatchdogInner> {
    DECLARE_SINGLETON(AppWatchdogInner);
public:
    int32_t GetReservedTimeForLogging();
    void SetBundleInfo(const std::string& bundleName, const std::string& bundleVersion);
    void SetSystemApp(bool isSystemApp);
    void SetForeground(bool isForeground);
    void SetAppDebug(bool isAppDebug);
    void SetSpecifiedProcessName(const std::string& name);
    void SetScrollState(bool isScroll);

    bool GetForeground();
    bool GetSystemApp();
    bool GetAppDebug();
    bool GetScrollState();
    std::string GetSpecifiedProcessName();
    std::string GetBundleName();

private:
    int reservedTime_ {DEFAULT_RESERVED_TIME};
    std::string specifiedProcessName_;
    std::string bundleName_;
    std::string bundleVersion_;
    std::atomic_bool isSystemApp_ { false };
    std::atomic_bool isForeground_ {false};
    std::atomic_bool isAppDebug_ {false};
    std::atomic_bool isScroll_ {false};
};
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif
