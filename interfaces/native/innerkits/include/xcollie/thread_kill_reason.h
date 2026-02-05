/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef RELIABILITY_THREAD_KILL_REASON_H
#define RELIABILITY_THREAD_KILL_REASON_H

#include <map>

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr const char* INVALID_KILL_ID = "InvalidKillId";
}
namespace ThreadKillReason {

enum kevent_id {
    REASON_MIN = 0,
    // crash: 0~1000
    REASON_NORMAL = REASON_MIN,
    REASON_CPP_CRASH,
    REASON_JS_ERROR,
    REASON_APP_FREEZE,
    REASON_RESOURCE_LEAK_CES,
    REASON_RESOURCE_LEAK_PSSSOFTLEAK,
    REASON_RESOURCE_LEAK_PSSLEAK,
    REASON_RESOURCE_LEAK_IONLEAK,
    REASON_RESOURCE_LEAK_ASHMEMLEAK,
    REASON_RESOURCE_LEAK_GPURSLEAK,
    REASON_RESOURCE_LEAK_GPULEAK,
    REASON_RESOURCE_LEAK_VMALEAK,
    REASON_RESOURCE_LEAK_FDLEAK,
    REASON_RESOURCE_LEAK_THREADLEAK,
    REASON_RESOURCE_LEAK_KERNELZONELEAK,
    REASON_RSS_KILLER,
    REASON_OOM_KILLER,
    REASON_CPA_KILLER,
    REASON_ASHMEM_KILLER,
    REASON_GPU_KILLER,
    REASON_DMA_KILLER,
    REASON_THREAD_KILLER,
    REASON_IO_HIGHLOAD,
    REASON_CPU_HIGHLOAD,
    REASON_CPU_HIGHLOAD_NOTIFY,
    REASON_CPU_HIGHLOAD_USER_REQUEST,
    REASON_ILLEGAL_AUDIO_RENDERER_BY_SUSPEND,
    REASON_ILLEGAL_AUDIO_CAPTURER_BY_SUSPEND,

    // app exit: 1000 ~ 2000
    REASON_KILLAPPLICATION = 1000,
    REASON_UNKNOWN,
    REASON_RESTART,

    // user exit: 2000 ~ 3000
    REASON_USER_REQUEST = 2000,
    REASON_UNINSTALL,
    REASON_UPGRADE,
    REASON_LOGOUT,
    REASON_UNINSTALL_STORAGE,

    // sys exit: 3000 ~ 4000
    REASON_POWER_SAVE_CLEAN = 3000,
    REASON_LOW_MEMORY_KILL,
    REASON_SWAP_KILL,
    REASON_HIGH_TEMPERATURE,
    REASON_TRANSIENT_TASK_TIMEOUT,

    REASON_MAX = REASON_TRANSIENT_TASK_TIMEOUT,
};

inline std::map<int32_t, const char* const> killReasonConfig {
    {REASON_NORMAL, "Normal"},
    {REASON_CPP_CRASH, "CppCrash"},
    {REASON_JS_ERROR, "JsError"},
    {REASON_APP_FREEZE, "Appfreeze"},
    {REASON_RESOURCE_LEAK_CES, "ResourceLeak(CES)"},
    {REASON_RESOURCE_LEAK_PSSSOFTLEAK, "ResourceLeak(PSSSoftLeak)"},
    {REASON_RESOURCE_LEAK_PSSLEAK, "ResourceLeak(PSSLeak)"},
    {REASON_RESOURCE_LEAK_IONLEAK, "ResourceLeak(IONLeak)"},
    {REASON_RESOURCE_LEAK_ASHMEMLEAK, "ResourceLeak(AshmemLeak)"},
    {REASON_RESOURCE_LEAK_GPURSLEAK, "ResourceLeak(GPURSLeak)"},
    {REASON_RESOURCE_LEAK_GPULEAK, "ResourceLeak(GPULeak)"},
    {REASON_RESOURCE_LEAK_VMALEAK, "ResourceLeak(VMLeak)"},
    {REASON_RESOURCE_LEAK_FDLEAK, "ResourceLeak(FdLeak)"},
    {REASON_RESOURCE_LEAK_THREADLEAK, "ResourceLeak(ThreadLeak)"},
    {REASON_RESOURCE_LEAK_KERNELZONELEAK, "ResourceLeak(KernelZoneLeak)"},
    {REASON_RSS_KILLER, "RSSKiller"},
    {REASON_OOM_KILLER, "OOMKiler"},
    {REASON_CPA_KILLER, "CPAKiller"},
    {REASON_ASHMEM_KILLER, "AshmemKiller"},
    {REASON_GPU_KILLER, "GpuKiller"},
    {REASON_DMA_KILLER, "DmaKiller"},
    {REASON_THREAD_KILLER, "ThreadKiller"},
    {REASON_IO_HIGHLOAD, "IoHighLoad"},
    {REASON_CPU_HIGHLOAD, "CpuHighLoad"},
    {REASON_CPU_HIGHLOAD_NOTIFY, "CpuHighLoadUserNotify"},
    {REASON_CPU_HIGHLOAD_USER_REQUEST, "CpuHighLoadUserRequest"},
    {REASON_ILLEGAL_AUDIO_RENDERER_BY_SUSPEND, "IllegalRendererBySuspend"},
    {REASON_ILLEGAL_AUDIO_CAPTURER_BY_SUSPEND, "IllegalAudioCapturerBySuspend"},
    {REASON_KILLAPPLICATION, "KillApplication"},
    {REASON_UNKNOWN, "UnKnown"},
    {REASON_RESTART, "Restart"},
    {REASON_USER_REQUEST, "UserRequest"},
    {REASON_UNINSTALL, "Uninstall"},
    {REASON_UPGRADE, "Upgrade"},
    {REASON_LOGOUT, "Logout"},
    {REASON_UNINSTALL_STORAGE, "UninstallStopage"},
    {REASON_POWER_SAVE_CLEAN, "PowerSaveClean"},
    {REASON_LOW_MEMORY_KILL, "LowMemoryKill"},
    {REASON_SWAP_KILL, "SwapFull"},
    {REASON_HIGH_TEMPERATURE, "HighTemperature"},
    {REASON_TRANSIENT_TASK_TIMEOUT, "TransientTaskTimeout"},
};

std::string GetKillReason(int killId)
{
    auto it = killReasonConfig.find(killId);
    return (it != killReasonConfig.end()) ? it->second : INVALID_KILL_ID;
}
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif // RELIABILITY_THREAD_KILL_REASON_H

