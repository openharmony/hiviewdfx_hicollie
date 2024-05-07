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

#include <string>
#include <map>
#include <vector>

#include "thread_sampler.h"

namespace OHOS {
namespace HiviewDFX {

uint64_t GetCurrentTimeNanoseconds();
std::string TimeFormat(uint64_t time);
void PutTimeInMap(std::map<uint64_t, std::vector<uint64_t>>& stackIdTimeMap, uint64_t stackId, uint64_t timestamp);
void DoUnwind(ThreadUnwindContext* context, std::shared_ptr<Unwinder> unwinder, UnwindInfo& unwindInfo);
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif
