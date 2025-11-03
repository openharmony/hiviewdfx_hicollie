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
};
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif

