/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef RELIABILITY_THREAD_SAMPLER_UTILS_H
#define RELIABILITY_THREAD_SAMPLER_UTILS_H

#include <map>
#include <string>
#include <vector>

#include "hilog/log.h"
#include "thread_sampler.h"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002D06

#undef LOG_TAG
#define LOG_TAG "XCollie"

namespace OHOS {
namespace HiviewDFX {

#define XCOLLIE_LOGF(...) HILOG_FATAL(LOG_CORE, ##__VA_ARGS__)
#define XCOLLIE_LOGE(...) HILOG_ERROR(LOG_CORE, ##__VA_ARGS__)
#define XCOLLIE_LOGW(...) HILOG_WARN(LOG_CORE, ##__VA_ARGS__)
#define XCOLLIE_LOGI(...) HILOG_INFO(LOG_CORE, ##__VA_ARGS__)
#define XCOLLIE_LOGD(...) HILOG_DEBUG(LOG_CORE, ##__VA_ARGS__)

uint64_t GetCurrentTimeNanoseconds();
std::string TimeFormat(uint64_t time);
void DoUnwind(const std::shared_ptr<Unwinder>& unwinder, UnwindInfo& unwindInfo);
std::vector<uintptr_t> GetAsyncStackPcsByStackId(uint64_t stackId);
std::string GetStackByPcs(const std::vector<uintptr_t>& pcVec, const std::shared_ptr<Unwinder>& unwinder,
                          const std::shared_ptr<DfxMaps>& maps, uint64_t snapshotTime);
}  // end of namespace HiviewDFX
}  // end of namespace OHOS
#endif
