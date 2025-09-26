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
#include "thread_sampler_utils.h"

#include <cstdio>
#include <ctime>

#include "dfx_frame_formatter.h"
#include "unique_stack_table.h"

namespace OHOS {
namespace HiviewDFX {
constexpr uint64_t NANOSEC_PER_SEC = 1000 * 1000 * 1000;
constexpr uint64_t NANOSEC_PER_MICROSEC = 1000;
constexpr int FORMAT_TIME_LEN = 20;
constexpr int MICROSEC_LEN = 6;

uint64_t GetCurrentTimeNanoseconds()
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = 0;
    clock_gettime(CLOCK_REALTIME, &t);
    return static_cast<uint64_t>(t.tv_sec) * NANOSEC_PER_SEC + static_cast<uint64_t>(t.tv_nsec);
}

std::string TimeFormat(uint64_t time)
{
    uint64_t nsec = time % NANOSEC_PER_SEC;
    time_t sec = static_cast<time_t>(time / NANOSEC_PER_SEC);
    char timeChars[FORMAT_TIME_LEN];
    struct tm localTime;
    if (localtime_r(&sec, &localTime) == nullptr) {
        return "";
    }
    size_t sz = strftime(timeChars, FORMAT_TIME_LEN, "%Y-%m-%d-%H-%M-%S", &localTime);
    if (sz == 0) {
        return "";
    }
    std::string s = timeChars;
    uint64_t usec = nsec / NANOSEC_PER_MICROSEC;
    std::string usecStr = std::to_string(usec);
    while (usecStr.size() < MICROSEC_LEN) {
        usecStr = "0" + usecStr;
    }
    s = s + "." + usecStr;
    return s;
}

void DoUnwind(const std::shared_ptr<Unwinder>& unwinder, UnwindInfo& unwindInfo)
{
#if defined(__aarch64__)
    static std::shared_ptr<DfxRegs> regs = std::make_shared<DfxRegsArm64>();
    regs->SetSp(unwindInfo.context->sp);
    regs->SetPc(unwindInfo.context->pc);
    regs->SetFp(unwindInfo.context->fp);
    regs->SetReg(REG_LR, &(unwindInfo.context->lr));
    unwinder->SetRegs(regs);
    unwinder->Unwind(&unwindInfo);
#endif  // #if defined(__aarch64__)
#if defined(__loongarch_lp64)
    static std::shared_ptr<DfxRegs> regs = std::make_shared<DfxRegsLoongArch64>();
    regs->SetSp(unwindInfo.context->sp);
    regs->SetPc(unwindInfo.context->pc);
    regs->SetReg(REG_LOONGARCH64_R22, &(unwindInfo.context->fp));
    regs->SetReg(REG_LOONGARCH64_R1, &(unwindInfo.context->lr));
    unwinder->SetRegs(regs);
    unwinder->Unwind(&unwindInfo);
#endif  // #if defined(__loongarch_lp64)
}

std::vector<uintptr_t> GetAsyncStackPcsByStackId(uint64_t stackId)
{
    std::vector<uintptr_t> pcVec;
    StackId stkId;
    stkId.value = stackId;
    if (!UniqueStackTable::Instance()->GetPcsByStackId(stkId, pcVec)) {
        XCOLLIE_LOGE("GetStackPcsByStackId failed, stackId %{public}" PRIu64 " not exist", stackId);
    }
    return pcVec;
}

std::string GetStackByPcs(const std::vector<uintptr_t>& pcVec, const std::shared_ptr<Unwinder>& unwinder,
                          const std::shared_ptr<DfxMaps>& maps, uint64_t snapshotTime)
{
    std::string stack;
    if (unwinder == nullptr || maps == nullptr) {
        XCOLLIE_LOGE("GetStackByPcs failed, unwinder or maps is nullptr");
        return stack;
    }
    if (snapshotTime != 0) {
        stack += "Timestamp:";
        stack += TimeFormat(snapshotTime) + "\n";
    }
    for (size_t i = 0; i < pcVec.size(); i++) {
        DfxFrame frame;
        unwinder->GetFrameByPc(pcVec[i], maps, frame);
        frame.index = i;
        auto frameStr = DfxFrameFormatter::GetFrameStr(frame);
        stack += frameStr;
    }
    return stack;
}
}  // end of namespace HiviewDFX
}  // end of namespace OHOS
