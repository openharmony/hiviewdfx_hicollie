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
#include "app_watchdog_utils.h"

#include <string>
#include "parameter.h"
#include "parameters.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
    constexpr const char* const KEY_BETA_TYPE = "const.logsystem.versiontype";
    constexpr const char* const ENABLE_BETA_VAULE = "beta";
}

bool AppWatchdogUtils::IsBetaVersion()
{
    static std::string isBetaVersion;
    if (!isBetaVersion.empty()) {
        return (isBetaVersion.find(ENABLE_BETA_VAULE) != std::string::npos);
    }
    isBetaVersion = system::GetParameter(KEY_BETA_TYPE, "");
    return (isBetaVersion.find(ENABLE_BETA_VAULE) != std::string::npos);
}
}  // end of namespace HiviewDFX
}  // end of namespace OHOS
