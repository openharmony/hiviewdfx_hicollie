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

#include "sample_stack_printer.h"

#include <iostream>
#include <sstream>

#include "dfx_frame_formatter.h"
#include "thread_sampler_utils.h"

namespace OHOS {
namespace HiviewDFX {
SampleStackItem* SampleStackPrinter::Insert(SampleStackItem* curNode, uintptr_t pc, int32_t count, uint64_t level)
{
    if (curNode == nullptr) {
        return nullptr;
    }

    if (curNode->pc == pc) {
        curNode->count += count;
        return curNode;
    }

    if (curNode->pc == 0) {
        curNode->pc = pc;
        curNode->count += count;
        curNode->current = std::make_shared<DfxFrame>();
        curNode->current->index = curNode->level;
        
        unwinder_->GetFrameByPc(pc, maps_, *(curNode->current));
        return curNode;
    }

    if (level > curNode->level) {
        if (curNode->child == nullptr) {
            curNode->child = new SampleStackItem;
            curNode->child->level = curNode->level + 1;
            allNodes_.push_back(curNode->child);
            return Insert(curNode->child, pc, count, level);
        }

        if (curNode->child->pc == pc) {
            curNode->child->count += count;
            return curNode->child;
        }

        curNode = curNode->child;
    }

    if (curNode->siblings == nullptr) {
        curNode->siblings = new SampleStackItem;
        curNode->siblings->level = curNode->level;
        allNodes_.push_back(curNode->siblings);
        return Insert(curNode->siblings, pc, count, level);
    }

    if (curNode->siblings->pc == pc) {
        curNode->siblings->count += count;
        return curNode->siblings;
    }

    return Insert(curNode->siblings, pc, count, level);
}

void SampleStackPrinter::Insert(std::vector<uintptr_t>& pcs, int32_t count)
{
    if (root_ == nullptr) {
        root_ = new SampleStackItem;
        root_->level = 0;
        allNodes_.push_back(root_);
    }

    SampleStackItem* curNode = root_;
    uint64_t level = 0;
    for (auto iter = pcs.rbegin(); iter != pcs.rend(); ++iter) {
        curNode = Insert(curNode, *iter, count, level);
        level++;
    }
}

std::string SampleStackPrinter::GetFullStack(std::vector<TimeAndFrames>& timeAndFrameList)
{
    std::string stack("");
    for (auto& taf : timeAndFrameList) {
        std::string requestTimeStr = TimeFormat(taf.requestTime);
        std::string snapshotTimeStr = TimeFormat(taf.snapshotTime);
        if (requestTimeStr == "" || snapshotTimeStr == "") {
            return stack;
        }
        stack += ("RequestTime:" + requestTimeStr + "\nSnapshotTime:" + snapshotTimeStr + "\n");
        std::vector<DfxFrame> frames = taf.frameList;
        for (auto& frame : frames) {
            unwinder_->FillFrame(frame);
            auto frameStr = DfxFrameFormatter::GetFrameStr(frame);
            stack += frameStr;
        }
        stack += "\n";
    }
    return stack;
}

std::string SampleStackPrinter::GetTreeStack(std::map<uint64_t, std::vector<uint64_t>>& stackIdTimeMap,
    std::unique_ptr<UniqueStackTable>& uniqueStackTable)
{
    std::vector<std::pair<uint64_t, uint64_t>> sortedStackId;
    for (auto it = stackIdTimeMap.begin(); it != stackIdTimeMap.end(); it++) {
        uint64_t stackId = it->first;
        sortedStackId.emplace_back(std::make_pair<uint64_t, uint64_t>(std::move(stackId), it->second.size()));
        std::sort(sortedStackId.begin(), sortedStackId.end(), [](auto& a, auto& b) {
            return a.second > b.second;
        });
    }
    for (auto it = sortedStackId.begin(); it != sortedStackId.end(); it++) {
        std::vector<uintptr_t> pcs;
        OHOS::HiviewDFX::StackId stackId;
        stackId.value = it->first;
        if (uniqueStackTable->GetPcsByStackId(stackId, pcs)) {
            Insert(pcs, it->second);
        }
    }
    std::string stack = Print();
    return stack;
}

std::string SampleStackPrinter::Print()
{
    if (root_ == nullptr) {
        return std::string("");
    }

    std::stringstream ret;
    std::vector<SampleStackItem*> nodes;
    nodes.push_back(root_);
    const int indent = 2;
    while (!nodes.empty()) {
        SampleStackItem* back = nodes.back();
        SampleStackItem* sibling = back->siblings;
        SampleStackItem* child = back->child;
        std::string space(indent * back->level, ' ');
        nodes.pop_back();
        
        ret << space << back->count << " " << DfxFrameFormatter::GetFrameStr(back->current);
        if (sibling != nullptr) {
            nodes.push_back(sibling);
        }

        if (child != nullptr) {
            nodes.push_back(child);
        }
    }
    return ret.str();
}

void SampleStackPrinter::FreeNodes()
{
    for (auto node : allNodes_) {
        if (node != nullptr) {
            delete node;
        }
    }
    allNodes_.clear();
    allNodes_.shrink_to_fit();
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
