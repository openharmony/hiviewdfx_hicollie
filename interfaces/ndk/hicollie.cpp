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
#include "report_data.h"
#include "xcollie.h"
#include "xcollie_define.h"
#include "xcollie_utils.h"
#include "iservice_registry.h"
#include "iremote_object.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
DECLARE_INTERFACE_DESCRIPTOR(u"ohos.appexecfwk.AppMgr");
#ifdef SUPPORT_ASAN
    constexpr uint32_t CHECK_INTERVAL_TIME = 45000;
#else
    constexpr uint32_t CHECK_INTERVAL_TIME = 3000;
    constexpr uint32_t TIME_S_TO_MS = 1000;
    constexpr uint32_t MAX_TIMEOUT = 15000;
#endif
    constexpr uint32_t INI_TIMER_FIRST_SECOND = 10000;
    constexpr uint32_t NOTIFY_APP_FAULT = 38;
    constexpr uint32_t APP_MGR_SERVICE_ID = 501;
    constexpr uint32_t RATIO = 2;
    constexpr int32_t BACKGROUND_REPORT_COUNT_MAX = 5;
}
static int32_t g_bussinessTid = 0;
static uint32_t g_stuckTimeout = 0;
static int64_t g_lastWatchTime = 0;
static std::atomic_int g_backgroundReportCount = 0;

bool IsAppMainThread()
{
    static int pid = getpid();
    static uint64_t uid = getuid();
    if (pid == gettid() && uid >= MIN_APP_UID) {
        return true;
    }
    return false;
}

int32_t NotifyAppFault(const OHOS::HiviewDFX::ReportData &reportData)
{
    XCOLLIE_LOGD("called.");
    auto systemAbilityMgr = OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityMgr == nullptr) {
        XCOLLIE_LOGE("ReportData failed to get system ability manager.");
        return -1;
    }
    auto appMgrService = systemAbilityMgr->GetSystemAbility(APP_MGR_SERVICE_ID);
    if (appMgrService == nullptr) {
        XCOLLIE_LOGE("ReportData failed to find APP_MGR_SERVICE_ID.");
        return -1;
    }
    OHOS::MessageParcel data;
    OHOS::MessageParcel reply;
    OHOS::MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        XCOLLIE_LOGE("ReportData failed to WriteInterfaceToken.");
        return -1;
    }
    if (!data.WriteParcelable(&reportData)) {
        XCOLLIE_LOGE("ReportData write reportData failed.");
        return -1;
    }
    auto ret = appMgrService->SendRequest(NOTIFY_APP_FAULT, data, reply, option);
    if (ret != OHOS::NO_ERROR) {
        XCOLLIE_LOGE("ReportData Send request failed with error code: %{public}d", ret);
        return -1;
    }
    return reply.ReadInt32();
}

bool CheckBackGroundStatus(bool* isSixSecond)
{
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::
        system_clock::now().time_since_epoch()).count();
    if ((now - g_lastWatchTime) > (RATIO * g_stuckTimeout)) {
        XCOLLIE_LOGI("Update backgroundCount, currTime: %{public}ld, lastTime: %{public}ld",
            now, g_lastWatchTime);
        g_backgroundReportCount.store(0);
    }
    bool inBackground = !OHOS::HiviewDFX::Watchdog::GetInstance().GetForeground();
    XCOLLIE_LOGD("In Background, thread is background: %{public}d", inBackground);
    if (inBackground) {
        if (g_backgroundReportCount.load() == BACKGROUND_REPORT_COUNT_MAX) {
            XCOLLIE_LOGI("In Background, thread has been blocked, need to report event: "
                "currTime: %{public}" PRId64 ", lastTime: %{public}" PRId64 ".", now, g_lastWatchTime);
            *isSixSecond = false;
            return false;
        } else if (g_backgroundReportCount.load() < BACKGROUND_REPORT_COUNT_MAX) {
            XCOLLIE_LOGI("In Background, not report event: g_backgroundReportCount: %{public}d"
                "currentTime: %{public}" PRId64 ", lastTime: %{public}" PRId64 ".",
                g_backgroundReportCount.load(), now, g_lastWatchTime);
            g_backgroundReportCount++;
            g_lastWatchTime = now;
            return true;
        }
    }
    return false;
}

int Report(bool* isSixSecond)
{
    if (CheckBackGroundStatus(isSixSecond, g_stuckTimeout)) {
        return 0;
    }
    g_backgroundReportCount++;

    OHOS::HiviewDFX::ReportData reportData;
    reportData.faultType = OHOS::HiviewDFX::FaultDataType::APP_FREEZE;
    int stuckTimeout = g_stuckTimeout;
    if (*isSixSecond) {
        reportData.errorObject.name = "BUSSINESS_THREAD_BLOCK_6S";
        reportData.forceExit = true;
        *isSixSecond = false;
        stuckTimeout = g_stuckTimeout * RATIO;
    } else {
        reportData.errorObject.name = "BUSSINESS_THREAD_BLOCK_3S";
        reportData.forceExit = false;
        *isSixSecond = true;
    }
    std::string timeStamp = "\nFaultTime:" + FormatTime("%Y-%m-%d %H:%M:%S") + "\n";
    reportData.errorObject.message = timeStamp +
        "Bussiness thread is not response, timeout " + std::to_string(stuckTimeout) + "ms.\n";
    reportData.timeoutMarkers = "";
    reportData.errorObject.stack = "";
    reportData.notifyApp = false;
    reportData.waitSaveState = false;
    reportData.stuckTimeout = g_stuckTimeout;
    reportData.tid = g_bussinessTid > 0 ? g_bussinessTid : getpid();
    auto result = NotifyAppFault(reportData);
    XCOLLIE_LOGI("OH_HiCollie_Report result: %{public}d, current tid: %{public}d, timeout: %{public}u",
        result, reportData.tid, reportData.stuckTimeout);
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::
        system_clock::now().time_since_epoch()).count();
    if ((now - g_lastWatchTime) < 0 || (now - g_lastWatchTime) >= (g_stuckTimeout / RATIO)) {
        XCOLLIE_LOGI("Update backgroundCount, currTime: %{public}ld, lastTime: %{public}ld",
            now, g_lastWatchTime);
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
        XCOLLIE_LOGE("task quque size exceed max");
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