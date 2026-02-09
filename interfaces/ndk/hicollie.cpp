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

#include "hicollie.h"

#include <atomic>
#include <unistd.h>
#include <string>
#include <sys/syscall.h>
#include "watchdog.h"
#include "xcollie.h"
#include "xcollie_define.h"
#include "xcollie_utils.h"
#include "fault_data.h"
#include "app_mgr_client.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
#ifdef SUPPORT_ASAN
    constexpr uint32_t CHECK_INTERVAL_TIME = 45000;
#else
    constexpr uint32_t CHECK_INTERVAL_TIME = 3000;
    constexpr uint32_t TIME_S_TO_MS = 1000;
    constexpr uint32_t MAX_TIMEOUT = 15000;
#endif
    constexpr uint32_t INI_TIMER_FIRST_SECOND = 10000;
    constexpr uint32_t RATIO = 2;
    constexpr int32_t BACKGROUND_REPORT_COUNT_MAX = 5;
}

static int32_t g_bussinessTid = 0;
static uint32_t g_stuckTimeout = 0;
static int64_t g_lastWatchTime = 0;
static std::atomic_int g_backgroundReportCount = 0;
static int g_pid = getpid();

bool IsAppMainThread()
{
    static uint64_t uid = getuid();
    if (g_pid == gettid() && uid >= MIN_APP_UID) {
        return true;
    }
    return false;
}

bool CheckInBackGround(bool* isSixSecond)
{
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::
        system_clock::now().time_since_epoch()).count();
    if ((now - g_lastWatchTime) > (RATIO * g_stuckTimeout)) {
        XCOLLIE_LOGI("Update backgroundCount, currentTime: %{public}llu, lastTime: %{public}llu",
            static_cast<unsigned long long>(now), static_cast<unsigned long long>(g_lastWatchTime));
        g_backgroundReportCount.store(0);
    }
    bool inBackground = !OHOS::HiviewDFX::Watchdog::GetInstance().GetForeground();
    XCOLLIE_LOGD("In Background, thread is background: %{public}d", inBackground);
    if (inBackground && g_backgroundReportCount.load() < BACKGROUND_REPORT_COUNT_MAX) {
        XCOLLIE_LOGI("In Background, not report event: g_backgroundReportCount: %{public}d, "
            "currentTime: %{public}llu, lastTime: %{public}llu", g_backgroundReportCount.load(),
            static_cast<unsigned long long>(now), static_cast<unsigned long long>(g_lastWatchTime));
        g_backgroundReportCount++;
        g_lastWatchTime = now;
        return true;
    }
    return false;
}

int Report(bool* isSixSecond)
{
    if (CheckInBackGround(isSixSecond)) {
        return 0;
    }
    g_backgroundReportCount++;

    AppExecFwk::FaultData faultData;
    faultData.faultType = OHOS::AppExecFwk::FaultDataType::APP_FREEZE;
    int stuckTimeout = g_stuckTimeout;
    if (*isSixSecond) {
        faultData.errorObject.name = "BUSSINESS_THREAD_BLOCK_6S";
        faultData.forceExit = true;
        *isSixSecond = false;
        stuckTimeout = g_stuckTimeout * RATIO;
        std::ifstream statmStream("/proc/" + std::to_string(g_pid) + "/statm");
        if (statmStream) {
            std::string procStatm;
            std::getline(statmStream, procStatm);
            statmStream.close();
            faultData.procStatm = procStatm;
        }
    } else {
        faultData.errorObject.name = "BUSSINESS_THREAD_BLOCK_3S";
        faultData.forceExit = false;
        *isSixSecond = true;
    }
    std::string timeStamp = "\nFaultTime:" + FormatTime("%Y-%m-%d %H:%M:%S") + "\n";
    faultData.errorObject.message = timeStamp +
        "Bussiness thread is not response, timeout " + std::to_string(stuckTimeout) + "ms.\n";
    faultData.timeoutMarkers = "";
    faultData.errorObject.stack = "";
    faultData.notifyApp = false;
    faultData.waitSaveState = false;
    faultData.tid = g_bussinessTid > 0 ? g_bussinessTid : g_pid;
    faultData.stuckTimeout = g_stuckTimeout;
    auto result = DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance()->NotifyAppFault(faultData);
    XCOLLIE_LOGI("OH_HiCollie_Report result: %{public}d, current tid: %{public}d, timeout: %{public}u",
        result, faultData.tid, faultData.stuckTimeout);
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::
        system_clock::now().time_since_epoch()).count();
    if ((now - g_lastWatchTime) < 0 || (now - g_lastWatchTime) >= (g_stuckTimeout / RATIO)) {
        XCOLLIE_LOGI("Update backgroundCount, currentTime: %{public}llu, lastTime: %{public}llu",
            static_cast<unsigned long long>(now), static_cast<unsigned long long>(g_lastWatchTime));
        g_lastWatchTime = now;
    }
    return result;
}
} // end of namespace HiviewDFX
} // end of namespace OHOS

inline HiCollie_ErrorCode InitStuckDetection(OH_HiCollie_Task task, uint32_t timeout)
{
    if (OHOS::HiviewDFX::IsAppMainThread()) {
        return HICOLLIE_WRONG_THREAD_CONTEXT;
    }
    if (!task) {
        OHOS::HiviewDFX::g_stuckTimeout = 0;
        OHOS::HiviewDFX::g_bussinessTid = 0;
        OHOS::HiviewDFX::Watchdog::GetInstance().RemovePeriodicalTask("BussinessWatchdog");
    } else {
        if (OHOS::HiviewDFX::g_stuckTimeout != 0) {
            OHOS::HiviewDFX::Watchdog::GetInstance().RemovePeriodicalTask("BussinessWatchdog");
        }
        OHOS::HiviewDFX::g_stuckTimeout = timeout;
        OHOS::HiviewDFX::g_lastWatchTime = 0;
        OHOS::HiviewDFX::g_backgroundReportCount.store(0);
        OHOS::HiviewDFX::g_bussinessTid = syscall(SYS_gettid);
        OHOS::HiviewDFX::Watchdog::GetInstance().RunPeriodicalTask("BussinessWatchdog", task,
            timeout, OHOS::HiviewDFX::INI_TIMER_FIRST_SECOND);
        OHOS::HiviewDFX::Watchdog::GetInstance().RemovePeriodicalTask("AppkitWatchdog");
    }
    return HICOLLIE_SUCCESS;
}

HiCollie_ErrorCode OH_HiCollie_Init_StuckDetection(OH_HiCollie_Task task)
{
    return InitStuckDetection(task, OHOS::HiviewDFX::CHECK_INTERVAL_TIME);
}

HiCollie_ErrorCode OH_HiCollie_Init_StuckDetectionWithTimeout(OH_HiCollie_Task task, uint32_t timeout)
{
#ifdef SUPPORT_ASAN
    timeout = OHOS::HiviewDFX::CHECK_INTERVAL_TIME;
#else
    timeout = timeout * OHOS::HiviewDFX::TIME_S_TO_MS;
    if (timeout < OHOS::HiviewDFX::CHECK_INTERVAL_TIME || timeout > OHOS::HiviewDFX::MAX_TIMEOUT) {
        return HICOLLIE_INVALID_ARGUMENT;
    }
#endif
    return InitStuckDetection(task, timeout);
}

HiCollie_ErrorCode OH_HiCollie_Init_JankDetection(OH_HiCollie_BeginFunc* beginFunc,
    OH_HiCollie_EndFunc* endFunc, HiCollie_DetectionParam param)
{
    if (OHOS::HiviewDFX::IsAppMainThread()) {
        return HICOLLIE_WRONG_THREAD_CONTEXT;
    }
    if ((!beginFunc && endFunc) || (beginFunc && !endFunc)) {
        return HICOLLIE_INVALID_ARGUMENT;
    }
    OHOS::HiviewDFX::Watchdog::GetInstance().InitMainLooperWatcher(beginFunc, endFunc);
    return HICOLLIE_SUCCESS;
}

HiCollie_ErrorCode OH_HiCollie_Report(bool* isSixSecond)
{
    if (OHOS::HiviewDFX::IsAppMainThread()) {
        return HICOLLIE_WRONG_THREAD_CONTEXT;
    }
    if (isSixSecond == nullptr) {
        return HICOLLIE_INVALID_ARGUMENT;
    }
    if (OHOS::HiviewDFX::Watchdog::GetInstance().GetAppDebug()) {
        XCOLLIE_LOGD("Bussiness: Get appDebug state is true");
        return HICOLLIE_SUCCESS;
    }
    if (OHOS::HiviewDFX::Report(isSixSecond) != 0) {
        return HICOLLIE_REMOTE_FAILED;
    }
    return HICOLLIE_SUCCESS;
}

HiCollie_ErrorCode OH_HiCollie_SetTimer(HiCollie_SetTimerParam param, int *id)
{
    if (param.name == nullptr) {
        XCOLLIE_LOGE("timer name is nullptr");
        return HICOLLIE_INVALID_TIMER_NAME;
    }
    std::string timerName = param.name;
    if (timerName.empty()) {
        XCOLLIE_LOGE("timer name is empty");
        return HICOLLIE_INVALID_TIMER_NAME;
    }
    if (param.timeout == 0) {
        XCOLLIE_LOGE("invalid timeout value");
        return HICOLLIE_INVALID_TIMEOUT_VALUE;
    }
    if (id == nullptr) {
        XCOLLIE_LOGE("wrong timer id output param");
        return HICOLLIE_WRONG_TIMER_ID_OUTPUT_PARAM;
    }
    
    int timerId = OHOS::HiviewDFX::XCollie::GetInstance().SetTimer(timerName, param.timeout, param.func, param.arg,
        param.flag);
    if (timerId == OHOS::HiviewDFX::INVALID_ID) {
        XCOLLIE_LOGE("wrong process context, process is in appspawn or nativespawn");
        return HICOLLIE_WRONG_PROCESS_CONTEXT;
    }
    if (timerId == 0) {
        XCOLLIE_LOGE("task queue size exceed max");
        return HICOLLIE_WRONG_TIMER_ID_OUTPUT_PARAM;
    }

    *id = timerId;
    return HICOLLIE_SUCCESS;
}

void OH_HiCollie_CancelTimer(int id)
{
    if (id <= 0) {
        XCOLLIE_LOGE("invalid timer id, cancel timer failed");
        return;
    }
    OHOS::HiviewDFX::XCollie::GetInstance().CancelTimer(id);
}
