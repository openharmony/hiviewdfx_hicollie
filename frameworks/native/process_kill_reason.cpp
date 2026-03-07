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

#include "process_kill_reason.h"

#include <unordered_map>

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr const char* INVALID_KILL_ID = "InvalidKillId";
}
namespace ProcessKillReason {
struct KillReasonInfo {
    std::string faultReason;
    std::string appReason;
};

inline std::unordered_map<int32_t, KillReasonInfo> killReasonConfig = {
    {REASON_KILL_APPLICATION_BY_BUNDLE_NAME, {"KillApplicationByBundleName", "KillApplication"}},
    {REASON_ON_REMOTE_DIED, {"OnRemoteDied", "OnRemoteDied"}},
    {REASON_RESTART, {"RestartApp", "Restart"}},
    {REASON_APP_EXIT, {"app exit", "KillApplication"}},
    {REASON_KILL_APPLICATION_SELF, {"KillApplicationSelf", "KillApplication"}},

    {REASON_USER_REQUEST, {"User Request", "UserRequest"}},
    {REASON_CLEAR_SESSION, {"ClearSession", "UserRequest"}},
    {REASON_UNINSTALL_APP, {"UninstallApp", "Uninstall"}},
    {REASON_UPGRADE_APP, {"UpgradeApp", "Upgrade"}},

    {REASON_THREAD_BLOCK_6S, {"THREAD_BLOCK_6S", "ThreadBlock6S"}},
    {REASON_APP_INPUT_BLOCK, {"APP_INPUT_BLOCK", "AppInputBlock"}},
    {REASON_LIFECYCLE_TIMEOUT, {"LIFECYCLE_TIMEOUT", "LifecycleTimeout"}},
    {REASON_JS_ERROR, {"Js Error", "JsError"}},
    {REASON_CPP_CRASH, {"Cpp Crash", "CppCrash"}},
    {REASON_CES_REGISTER_EXCEED_LIMIT, {"CES Register exceed limit", "ResourceLeak(CES)"}},
    {REASON_RESOURCE_LEAK_PSS_SOFT_KILL, {"ResourceLeak:Pss Soft Kill", "ResourceLeak(PSSSoftLeak)"}},
    {REASON_RESOURCE_LEAK_PSS_KILL, {"ResourceLeak:Pss Kill", "ResourceLeak(PSSLeak)"}},
    {REASON_RESOURCE_LEAK_ION_LEAK, {"ResourceLeak:Ion Leak", "ResourceLeak(IonLeak)"}},
    {REASON_RESOURCE_LEAK_ASHMEM_LEAK, {"ResourceLeak:Ashmem Leak", "ResourceLeak(AshmemLeak)"}},
    {REASON_RESOURCE_LEAK_GPU_RS_LEAK, {"ResourceLeak:Gpu_rs Leak", "ResourceLeak(GpuRsLeak)"}},
    {REASON_RESOURCE_LEAK_GPU_LEAK, {"ResourceLeak:Gpu Leak", "ResourceLeak(GpuLeak)"}},
    {REASON_RESOURCE_LEAK_VMA_LEAK, {"ResourceLeak:Vma Leak", "ResourceLeak(VMALeak)"}},
    {REASON_RESOURCE_LEAK_FD_LEAK, {"ResourceLeak:Fd Leak", "ResourceLeak(FDLeak)"}},
    {REASON_RESOURCE_LEAK_THREAD_LEAK, {"ResourceLeak:Thread Leak", "ResourceLeak(ThreadLeak)"}},
    {REASON_FD_EXCEEDS_LIMIT, {"The number of fd exceeds the limit", "FdRs"}},
    {REASON_APP_HIGH_POWER_NOTIFY_KILL, {"App High Power Notify Kill", "CPUHighloadNotify"}},
    {REASON_HIGH_POWER_CONSUMPTION_KILL, {"High Power Consumption Kill", "CPUHighloadUserRequest"}},
    {REASON_ILLEGAL_AUDIO_RENDERER_BY_SUSPEND, {"ILLEGAL_AUDIO_RENDERER_BY_SUSPEND", "IllegalAudioRendererBySuspend"}},
    {REASON_ILLEGAL_AUDIO_CAPTURER_BY_SUSPEND, {"ILLEGAL_AUDIO_CAPTURER_BY_SUSPEND", "IllegalAudioCapturerBySuspend"}},
    {REASON_IO_MANAGE_CONTROL, {"IO Manage Control", "IOHighload"}},
    {REASON_BUSINESS_THREAD_BLOCK_6S, {"BUSINESS_THREAD_BLOCK_6S", "AppFreeze"}},
    {REASON_BUSINESS_INPUT_BLOCK, {"BUSINESS_INPUT_BLOCK", "AppFreeze"}},

    {REASON_CONTINUOUSLY_WAKEUP_ABNORMAL, {"ContinuouslyWakeupAbnormal", "PowerSaveClean"}},
    {REASON_BACKGROUND_FREEZE_ABNORMAL, {"BackgroundFreezeAbnormal", "PowerSaveClean"}},
    {REASON_GNSS_WORK_TIME_ABNORMAL, {"GnssWorkTimeAbnormal", "PowerSaveClean"}},
    {REASON_BLUETOOTH_WORK_TIME_ABNORMAL, {"BluetoothWorkTimeAbnormal", "PowerSaveClean"}},
    {REASON_RUNNING_LOCK_ABNORMAL, {"RunningLockAbnormal", "PowerSaveClean"}},
    {REASON_KERNEL_WAKELOCK_ABNORMAL, {"KernelWakelockAbnormal", "PowerSaveClean"}},
    {REASON_POWER_SAVE_CLEAN_EXTREME_POWER_SAVE_CLEAN, {"PowerSaveClean ExtremePowerSaveClean", "PowerSaveClean"}},
    {REASON_MODULE_POWER_CONSUMPTION_ABNORMAL, {"Module Power Consumption Abnormal", "PowerSaveClean"}},
    {REASON_POWER_ABNORMAL, {"Power Abnormal", "PowerSaveClean"}},
    {REASON_MEMORY_PRESSURE, {"Memory Pressure", "LowMemoryKill"}},
    {REASON_STABILITY_CHECK_KILL, {"StabilityCheckKill", "LowMemoryKill"}},
    {REASON_APP_LIMIT_KILL, {"APP_LIMIT_KILL", "LowMemoryKill"}},
    {REASON_SWAP_FULL, {"SWAP_FULL", "SwapFull"}},
    {REASON_SYSTEM_CAMERA_QUICK_KILL, {"SYSTEM_CAMERA_QUICK_KILL", "LowMemoryKill"}},
    {REASON_OTHER_CAMERA_QUICK_KILL, {"OTHER_CAMERA_QUICK_KILL", "LowMemoryKill"}},
    {REASON_ALIVE_APP_COUNT_KILL, {"AliveAppCountKill", "LowMemoryKill"}},
    {REASON_LOW_FREE_KILL, {"LowFreeKill", "LowMemoryKill"}},
    {REASON_ZSWAPD_PRESSURE, {"ZSWAPD_PRESSURE", "LowMemoryKill"}},
    {REASON_BIG_MEMORY_KILL, {"BIG_MEMORY_KILL", "LowMemoryKill"}},
    {REASON_GPU_LEAK_KILL, {"GPU_LEAK_KILL", "LowMemoryKill"}},
    {REASON_MEMORY_LIMIT_KILL, {"MemoryLimitKill", "LowMemoryKill"}},
    {REASON_PC_BIG_MEMORY_KILL, {"PcBigMemoryKill", "LowMemoryKill"}},
    {REASON_GPU_TEXTURE_MEM_LEAK_KILL, {"GpuTextureMemLeakKill", "LowMemoryKill"}},
    {REASON_ANCO_START_QUICK_KILL, {"AncoStartQuickKill", "LowMemoryKill"}},
    {REASON_BG_DEPDED_MINI_SYS_KILL, {"BgDepdedMiniSysKill", "LowMemoryKill"}},
    {REASON_FORE_UNDEPD_MINI_SYS_KILL, {"ForeUndepdMiniSysKill", "LowMemoryKill"}},
    {REASON_TEMPERATURE_CONTROL, {"Temperature Control", "HighTemperature"}},
    {REASON_CPU_EXT_HIGHLOAD, {"CPU_EXT Highload", "PowerSaveClean"}},
    {REASON_SCREEN_OFF_CLEAN, {"Screen Off Clean", "PowerSaveClean"}},
    {REASON_SLEEP_CLEAN, {"Sleep Clean", "PowerSaveClean"}},
    {REASON_INACTIVE_CLEAN, {"Inactive Clean", "PowerSaveClean"}},
    {REASON_STANDBY_FAILED_CONTINUOUSLY_WAKEUP_ABNORMAL, {"StandbyFailedContinuouslyWakeupAbnormal", "PowerSaveClean"}},
    {REASON_SWITCH_TO_PENGLAI_MODE, {"Switch To Penglai Mode", "PowerSaveClean"}},
    {REASON_HEAVY_LOAD_MUTEX_KILL, {"HeavyLoad Mutex Kill", "PowerSaveClean"}},
    {REASON_TRANSIENT_TASK_TIMEOUT, {"TRANSIENT_TASK_TIMEOUT", "TransientTaskTimeout"}},
    {REASON_RESOURCE_OVERLIMIT, {"Resource Overlimit", "LowMemoryKill"}},
    {REASON_RESOURCE_LEAK_STANDBY_CLEAN, {"ResourceLeak:Standby Clean", "PowerSaveClean"}},
    {REASON_SYSFREEZE_KILL_PEER_BINDER, {"SysfreezeKillPeerBinder", "LowMemoryKill"}},
    {REASON_TOO_MANY_READY_THREADS, {"TooManyReadyThreads", "TooManyReadyThreads"}},
    {REASON_JS_HEAP_SLEEP_CLEAN_KILL, {"Js_Heap_Sleep_Clean_Kill", "REASON_RESOURCE_CONTROL"}},
    {REASON_HARDWARE_DECODING_RESOURCES_LIMIT, {"HARDWARE_DECODING_RESOURCES_LIMIT", "HardwareDecodingResourcesLimit"}},

    {REASON_OOM_KILLER, {"OOM_KILLER", "OomKiller"}},
    {REASON_CPA_KILLER, {"CPA_KILLER", "CpaKiller"}},
    {REASON_LOW_MEMORY_KILLER, {"LowMemoryKill", "LowMemoryKill"}},
    {REASON_CPU_HIGHLOAD_KILLER, {"CPU Highload", "CPUHighload"}},
    {REASON_RSS_THRESHOLD_KILLER, {"RSS_THRESHOLD_KILLER", "RssThresholdKiller"}},
    {REASON_ASHMEM_THRESHOLD_KILLER, {"ASHMEM_THRESHOLD_KILLER", "AshmemKiller"}},
    {REASON_GPU_THRESHOLD_KILLER, {"GPU_THRESHOLD_KILLER", "GpuKiller"}},
    {REASON_DMA_THRESHOLD_KILLER, {"DMA_THRESHOLD_KILLER", "DmaKiller"}},
    {REASON_THREAD_THRESHOLD_KILLER, {"THREAD_THRESHOLD_KILLER", "ThreadKiller"}},
    {REASON_KERNEL_ZONE_LOW, {"Kernel Zone Low", "ResourceLeak(KernelZoneLeak)"}},
    {REASON_APP_SANDBOX_UNINSTALL, {"VolumeLogout", "Logout"}},
    {REASON_STORAGE_CARD_UNINSTALL, {"VolumeUmount", "UninstallStorage"}},
};

std::string GetKillReason(int killId)
{
    auto it = killReasonConfig.find(killId);
    return (it != killReasonConfig.end()) ? it->second.faultReason : INVALID_KILL_ID;
}

std::string GetAppExitReason(int killId)
{
    auto it = killReasonConfig.find(killId);
    return (it != killReasonConfig.end()) ? it->second.appReason : INVALID_KILL_ID;
}
} // end of namespace ProcessKillReason
} // end of namespace HiviewDFX
} // end of namespace OHOS
