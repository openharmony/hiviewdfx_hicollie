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

#ifndef RELIABILITY_SAMPLE_STACK_PRINTER_H
#define RELIABILITY_SAMPLE_STACK_PRINTER_H

#include <map>
#include <vector>

#include "unwinder.h"
#include "dfx_frame.h"
#include "thread_sampler.h"
#include "unique_stack_table.h"

namespace OHOS {
namespace HiviewDFX {
struct SampleStackItem {
    uintptr_t pc;
    int32_t count;
    uint64_t level;
    std::shared_ptr<DfxFrame> current;
    std::shared_ptr<SampleStackItem> child;
    std::shared_ptr<SampleStackItem> siblings;

    SampleStackItem() : pc(0),
        count(0),
        level(0),
        current(nullptr),
        child(nullptr),
        siblings(nullptr)
        {};
};
class SampleStackPrinter {
public:
    SampleStackPrinter(std::shared_ptr<Unwinder> unwinder, std::shared_ptr<DfxMaps> maps) : unwinder_(unwinder),
        maps_(maps)
    {
        root_ = nullptr;
    };
    SampleStackPrinter(const SampleStackPrinter& other) = delete;
    SampleStackPrinter& operator=(const SampleStackPrinter& other) = delete;
    ~SampleStackPrinter()
    {};

    void Insert(std::vector<uintptr_t>& pcs, int32_t count);
    std::string GetFullStack(const std::vector<TimeAndFrames>& timeAndFrameList);
    std::string GetTreeStack(std::vector<StackIdAndCount>& stackIdCount,
        std::unique_ptr<UniqueStackTable>& uniqueStackTable, std::string& heaviestStack);
    std::string Print();

private:
    std::shared_ptr<SampleStackItem> Insert(std::shared_ptr<SampleStackItem> curNode,
        uintptr_t pc, int32_t count, uint64_t level, std::shared_ptr<SampleStackItem> acientNode);
    std::shared_ptr<SampleStackItem> AdjustSiblings(std::shared_ptr<SampleStackItem> acient,
        std::shared_ptr<SampleStackItem> cur, std::shared_ptr<SampleStackItem> node);
    std::shared_ptr<SampleStackItem> root_;
    std::shared_ptr<Unwinder> unwinder_;
    std::shared_ptr<DfxMaps> maps_;
};
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif
