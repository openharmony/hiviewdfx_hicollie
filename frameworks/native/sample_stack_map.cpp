/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "sample_stack_map.h"

#include "xcollie_utils.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr size_t SAMPLE_STACK_MAP_MAX_SIZE = 3;
constexpr uint64_t SAMPLE_STACK_MAP_EXPIRE_MS = 120000;
constexpr size_t SAMPLE_STACK_MAP_VALUE_MAX_SIZE = 256 * 1024;
}

SampleStackMap::SampleStackMap()
{
    for (size_t i = 0; i < SAMPLE_STACK_MAP_MAX_SIZE; i++) {
        entries_[i].timestamp = 0;
        entries_[i].valid = false;
    }
}

SampleStackMap::~SampleStackMap() {}

void SampleStackMap::Set(const std::string& key, const std::string& value)
{
    if (value.size() > SAMPLE_STACK_MAP_VALUE_MAX_SIZE) {
        XCOLLIE_LOGW("SampleStackMap value size too large, key: %{public}s, size: %{public}zu",
            key.c_str(), value.size());
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    ExpireOldEntries();

    size_t oldestIdx = 0;
    uint64_t oldestTime = UINT64_MAX;
    size_t emptyIdx = SAMPLE_STACK_MAP_MAX_SIZE;

    for (size_t i = 0; i < SAMPLE_STACK_MAP_MAX_SIZE; i++) {
        if (entries_[i].valid && entries_[i].key == key) {
            entries_[i].value = value;
            entries_[i].timestamp = GetCurrentTickMillseconds();
            return;
        }
        if (!entries_[i].valid) {
            emptyIdx = i;
        } else if (entries_[i].timestamp < oldestTime) {
            oldestTime = entries_[i].timestamp;
            oldestIdx = i;
        }
    }

    size_t idx = (emptyIdx < SAMPLE_STACK_MAP_MAX_SIZE) ? emptyIdx : oldestIdx;
    entries_[idx].key = key;
    entries_[idx].value = value;
    entries_[idx].timestamp = GetCurrentTickMillseconds();
    entries_[idx].valid = true;
}

std::string SampleStackMap::GetAndRemove(const std::string& key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ExpireOldEntries();
    for (size_t i = 0; i < SAMPLE_STACK_MAP_MAX_SIZE; i++) {
        if (entries_[i].valid && entries_[i].key == key) {
            entries_[i].valid = false;
            return std::move(entries_[i].value);
        }
    }
    return "";
}

void SampleStackMap::ExpireOldEntries()
{
    uint64_t now = GetCurrentTickMillseconds();
    for (size_t i = 0; i < SAMPLE_STACK_MAP_MAX_SIZE; i++) {
        if (entries_[i].valid &&
            (now - entries_[i].timestamp) > SAMPLE_STACK_MAP_EXPIRE_MS) {
            entries_[i].valid = false;
        }
    }
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
