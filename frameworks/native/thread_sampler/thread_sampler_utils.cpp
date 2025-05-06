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

uint64_t GetCurrentTimeNanoseconds()
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = 0;
    clock_gettime(CLOCK_REALTIME, &t);
    return static_cast<uint64_t>(t.tv_sec) * SEC_TO_NANOSEC + static_cast<uint64_t>(t.tv_nsec);
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
} // end of namespace HiviewDFX
} // end of namespace OHOS
