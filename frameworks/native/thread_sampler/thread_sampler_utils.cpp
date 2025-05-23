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

#include <ctime>
#include <cstdio>

namespace OHOS {
namespace HiviewDFX {
constexpr uint64_t SEC_TO_NANOSEC = 1000000000;
constexpr uint64_t MICROSEC_TO_NANOSEC = 1000;
constexpr int FORMAT_TIME_LEN = 20;
constexpr int MICROSEC_LEN = 6;

uint64_t GetCurrentTimeNanoseconds()
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = 0;
    clock_gettime(CLOCK_REALTIME, &t);
    return static_cast<uint64_t>(t.tv_sec) * SEC_TO_NANOSEC + static_cast<uint64_t>(t.tv_nsec);
}

std::string TimeFormat(uint64_t time)
{
    uint64_t nsec = time % SEC_TO_NANOSEC;
    time_t sec = static_cast<time_t>(time / SEC_TO_NANOSEC);
    char timeChars[FORMAT_TIME_LEN];
    struct tm* localTime = localtime(&sec);
    if (localTime == nullptr) {
        return "";
    }
    size_t sz = strftime(timeChars, FORMAT_TIME_LEN, "%Y-%m-%d-%H-%M-%S", localTime);
    if (sz == 0) {
        return "";
    }
    std::string s = timeChars;
    uint64_t usec = nsec / MICROSEC_TO_NANOSEC;
    std::string usecStr = std::to_string(usec);
    while (usecStr.size() < MICROSEC_LEN) {
        usecStr = "0" + usecStr;
    }
    s = s + "." + usecStr;
    return s;
}

void PutStackId(std::vector<StackIdAndCount>& stackIdCount, uint64_t stackId)
{
    auto it = std::find_if(stackIdCount.begin(), stackIdCount.end(), [&stackId](const auto& stackIdCnt) {
        return stackIdCnt.stackId == stackId;
    });
    if (it == stackIdCount.end()) {
        StackIdAndCount sac = {
            .stackId = stackId,
            .count = 1,
        };
        stackIdCount.emplace_back(sac);
    } else {
        it->count++;
    }
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
    unwinder->EnableFillFrames(false);
    unwinder->Unwind(&unwindInfo);
#endif  // #if defined(__aarch64__)
#if defined(__loongarch_lp64)
    static std::shared_ptr<DfxRegs> regs = std::make_shared<DfxRegsLoongArch64>();
    regs->SetSp(unwindInfo.context->sp);
    regs->SetPc(unwindInfo.context->pc);
    regs->SetReg(REG_LOONGARCH64_R22, &(unwindInfo.context->fp));
    regs->SetReg(REG_LOONGARCH64_R1, &(unwindInfo.context->lr));
    unwinder->SetRegs(regs);
    unwinder->EnableFillFrames(false);
    unwinder->Unwind(&unwindInfo);
#endif  // #if defined(__loongarch_lp64)
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
