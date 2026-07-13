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

#ifndef RELIABILITY_SAMPLE_STACK_MAP_H
#define RELIABILITY_SAMPLE_STACK_MAP_H

#include <mutex>
#include <string>

#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {

class SampleStackMap : public Singleton<SampleStackMap> {
    DECLARE_SINGLETON(SampleStackMap);

public:
    void Set(const std::string& key, const std::string& value);
    std::string GetAndRemove(const std::string& key);

private:
    struct Entry {
        std::string key;
        std::string value;
        uint64_t timestamp = 0;
        bool valid = false;
    };

    void ExpireOldEntries();
    Entry entries_[3];
    mutable std::mutex mutex_;
};

} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif