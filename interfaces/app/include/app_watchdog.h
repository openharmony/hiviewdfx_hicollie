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

#ifndef RELIABILITY_APPWATCHDOG_H
#define RELIABILITY_APPWATCHDOG_H

#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
class AppWatchdog : public Singleton<AppWatchdog> {
    DECLARE_SINGLETON(AppWatchdog);
public:
    /**
     * @brief Get sampler result.
     */
    int32_t GetReservedTimeForLogging();
    
    /**
     *
     * @brief Set bundle info.
     *
     */
    void SetBundleInfo(const std::string& bundleName, const std::string& bundleVersion);

    /**
     *
     * @brief Set system app.
     *
     */
    void SetSystemApp(bool isSystemApp);

    /**
     *
     * @brief Set foreground.
     *
     */
    void SetForeground(bool isForeground);

    /**
     * @brief Set isAppDebug.
     */
    void SetAppDebug(bool isAppDebug);

    /**
     * @brief Set specified process name.
     */
    void SetSpecifiedProcessName(const std::string& name);

    /**
     * @brief Set scroll param.
     */
    void SetScrollState(bool isScroll);

    /**
     *
     * @brief Get systemApp.
     *
     */
    bool GetSystemApp();

    /**
     *
     * @brief Get bundleName.
     *
     */
    std::string GetBundleName();

    /**
     *
     * @brief Get foreground.
     *
     */
    bool GetForeground();

    /**
     *
     * @brief Get specifiedProcessName.
     *
     */
    std::string GetSpecifiedProcessName();

    /**
     *
     * @brief Get scrollState.
     *
     */
    bool GetScrollState();

    /**
     *
     * @brief Get appDebug.
     *
     */
    bool GetAppDebug();
};
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif

