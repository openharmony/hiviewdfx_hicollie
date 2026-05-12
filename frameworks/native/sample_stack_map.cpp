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
constexpr size_t SAMPLE_STACK_MAP_MAX_SIZE = 5;
constexpr uint64_t SAMPLE_STACK_MAP_EXPIRE_MS = 180000;
constexpr size_t SAMPLE_STACK_MAP_VALUE_MAX_SIZE = 256 * 1024;
}
 
SampleStackMap::SampleStackMap() {}
 
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
    if (map_.size() >= SAMPLE_STACK_MAP_MAX_SIZE) {
        auto oldest = order_.begin();
        map_.erase(*oldest);
        order_.erase(oldest);
    }
    auto orderIt = order_.insert(order_.end(), key);
    map_[key] = {value, orderIt, GetCurrentTickMillseconds()};
}
 
std::string SampleStackMap::GetAndRemove(const std::string& key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ExpireOldEntries();
    auto it = map_.find(key);
    if (it == map_.end()) {
        return "";
    }
    auto orderIt = it->second.orderIt;
    std::string value = std::move(it->second.value);
    map_.erase(it);
    order_.erase(orderIt);
    return value;
}
 
void SampleStackMap::ExpireOldEntries()
{
    uint64_t now = GetCurrentTickMillseconds();
    for (auto it = order_.begin(); it != order_.end();) {
        auto mapIt = map_.find(*it);
        if (mapIt != map_.end() && (now - mapIt->second.timestamp) > SAMPLE_STACK_MAP_EXPIRE_MS) {
            order_.erase(it++);
            map_.erase(mapIt);
        } else {
            ++it;
        }
    }
}
} // end of namespace HiviewDFX
} // end of namespace OHOS