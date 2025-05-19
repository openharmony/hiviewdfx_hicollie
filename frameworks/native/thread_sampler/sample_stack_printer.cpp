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
std::shared_ptr<SampleStackItem> SampleStackPrinter::Insert(std::shared_ptr<SampleStackItem> curNode,
    uintptr_t pc, int32_t count, uint64_t level, std::shared_ptr<SampleStackItem> acientNode)
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
        return curNode;
    }

    if (level > curNode->level) {
        acientNode = curNode;

        if (curNode->child == nullptr) {
            curNode->child = std::make_shared<SampleStackItem>();
            curNode->child->level = curNode->level + 1;
            return Insert(curNode->child, pc, count, level, acientNode);
        }

        if (curNode->child->pc == pc) {
            curNode->child->count += count;
            return curNode->child;
        }

        curNode = curNode->child;
    }

    if (curNode->siblings == nullptr) {
        curNode->siblings = std::make_shared<SampleStackItem>();
        curNode->siblings->level = curNode->level;
        std::shared_ptr<SampleStackItem> node = Insert(curNode->siblings, pc, count, level, acientNode);
        curNode = AdjustSiblings(acientNode, curNode, node);
        return curNode;
    }

    if (curNode->siblings->pc == pc) {
        curNode->siblings->count += count;
        curNode = AdjustSiblings(acientNode, curNode, curNode->siblings);
        return curNode;
    }

    return Insert(curNode->siblings, pc, count, level, acientNode);
}

void SampleStackPrinter::Insert(std::vector<uintptr_t>& pcs, int32_t count)
{
    if (root_ == nullptr) {
        root_ = std::make_shared<SampleStackItem>();
        root_->level = 0;
    }

    std::shared_ptr<SampleStackItem> dummyNode = std::make_shared<SampleStackItem>();
    std::shared_ptr<SampleStackItem> acientNode = dummyNode;
    acientNode->child = root_;
    std::shared_ptr<SampleStackItem> curNode = root_;
    uint64_t level = 0;
    for (auto iter = pcs.rbegin(); iter != pcs.rend(); ++iter) {
        curNode = Insert(curNode, *iter, count, level, acientNode);
        level++;
    }
}

std::shared_ptr<SampleStackItem> SampleStackPrinter::AdjustSiblings(std::shared_ptr<SampleStackItem> acient,
    std::shared_ptr<SampleStackItem> cur, std::shared_ptr<SampleStackItem> node)
{
    std::shared_ptr<SampleStackItem> dummy = std::make_shared<SampleStackItem>();
    dummy->siblings = acient->child;
    std::shared_ptr<SampleStackItem> p = dummy;
    while (p->siblings != node && p->siblings->count >= node->count) {
        p = p->siblings;
    }
    if (p->siblings != node) {
        cur->siblings = node->siblings;
        node->siblings = p->siblings;
        if (p == dummy) {
            acient->child = node;
        }
        p->siblings = node;
    }
    return node;
}

std::string SampleStackPrinter::GetFullStack(const std::vector<TimeAndFrames>& timeAndFrameList)
{
    std::string stack("");
    for (const auto& taf : timeAndFrameList) {
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

std::string SampleStackPrinter::GetTreeStack(std::vector<StackIdAndCount>& stackIdCount,
    std::unique_ptr<UniqueStackTable>& uniqueStackTable, std::string& heaviestStack)
{
    std::sort(stackIdCount.begin(), stackIdCount.end(), [](const auto& a, const auto& b) {
        return a.count > b.count;
    });
    unwinder_->EnableFillFrames(true);
    std::string stack("");
    for (auto it = stackIdCount.begin(); it != stackIdCount.end(); it++) {
        std::vector<uintptr_t> pcs;
        OHOS::HiviewDFX::StackId stackId;
        stackId.value = it->stackId;
        if (uniqueStackTable->GetPcsByStackId(stackId, pcs)) {
            Insert(pcs, it->count);
        }
        if (it == stackIdCount.begin()) {
            heaviestStack = "heaviest stack: \nstack counts: " + std::to_string(it->count) + "\n";
            for (auto i = pcs.size(); i > 0; i--) {
                DfxFrame frame;
                unwinder_->GetFrameByPc(pcs[i - 1], maps_, frame);
                frame.index = pcs.size() - i;
                auto frameStr = DfxFrameFormatter::GetFrameStr(frame);
                heaviestStack += frameStr;
            }
        }
    }
    stack += Print();
    return stack;
}

std::string SampleStackPrinter::Print()
{
    if (root_ == nullptr) {
        return std::string("");
    }

    std::stringstream ret;
    std::vector<std::shared_ptr<SampleStackItem>> nodes;
    nodes.push_back(root_);
    const int indent = 2;
    while (!nodes.empty()) {
        std::shared_ptr<SampleStackItem> back = nodes.back();
        back->current = std::make_shared<DfxFrame>();
        back->current->index = back->level;
        unwinder_->GetFrameByPc(back->pc, maps_, *(back->current));
        std::shared_ptr<SampleStackItem> sibling = back->siblings;
        std::shared_ptr<SampleStackItem> child = back->child;
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
} // end of namespace HiviewDFX
} // end of namespace OHOS
