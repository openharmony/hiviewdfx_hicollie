/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef RELIABILITY_WATCHDOG_INNER_H
#define RELIABILITY_WATCHDOG_INNER_H

#include <atomic>
#include <condition_variable>
#include <csignal>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>

#include "watchdog_task.h"
#include "c/ffrt_dump.h"
#include "singleton.h"
#include "client/trace_collector_client.h"
#include "xcollie_define.h"
#include "watchdog_inner_data.h"

namespace OHOS {
namespace HiviewDFX {

class WatchdogInner : public Singleton<WatchdogInner> {
    DECLARE_SINGLETON(WatchdogInner);
public:
    std::map<int64_t, int> taskIdCnt;
    int AddThread(const std::string &name, std::shared_ptr<AppExecFwk::EventHandler> handler, uint64_t interval);
    int AddThread(const std::string &name, std::shared_ptr<AppExecFwk::EventHandler> handler,
        TimeOutCallback timeOutCallback, uint64_t interval);
    void RunOneShotTask(const std::string& name, Task&& task, uint64_t delay);
    void RunPeriodicalTask(const std::string& name, Task&& task, uint64_t interval, uint64_t delay);
    int64_t RunXCollieTask(const std::string& name, uint64_t timeout, XCollieCallback func, void *arg,
        unsigned int flag);
    void RemoveXCollieTask(int64_t id);
    bool AddIpcFull(uint64_t interval, unsigned int flag, IpcFullCallback func, void *arg);
    int64_t SetTimerCountTask(const std::string &name, uint64_t timeLimit, int countLimit);
    void TriggerTimerCountTask(const std::string &name, bool bTrigger, const std::string &message);
    void StopWatchdog();
    bool IsCallbackLimit(unsigned int flag);
    bool IpcCheck(uint64_t interval = DEFAULT_IPC_FULL_INTERVAL, unsigned int flag = XCOLLIE_FLAG_DEFAULT,
        IpcFullCallback func = nullptr, void *arg = nullptr, bool defaultType = true);
    void InitFfrtWatchdog();
    static bool WriteStringToFile(int32_t pid, const char *str);
    static void FfrtCallback(uint64_t taskId, const char *taskInfo, uint32_t delayedTaskCount);
    static void SendFfrtEvent(const std::string &msg, const std::string &eventName, const char *taskInfo,
        const std::string& faultTimeStr, const bool isDumpStack = true);
    static void LeftTimeExitProcess(const std::string &description);
    static void KillPeerBinderProcess(const std::string &description);
    bool StartScrollProfile(const TimePoint& endTime, int64_t durationTime, int sampleInterval);
    void StartProfileMainThread(const TimePoint& endTime, int64_t durationTime, int sampleInterval);
    bool CollectStack(std::string& stack, std::string& heaviestStack, int treeFormat = ENABLE_TREE_FORMAT);
    bool Deinit();
    void SetBundleInfo(const std::string& bundleName, const std::string& bundleVersion);
    void SetSystemApp(bool isSystemApp);
    bool GetSystemApp();
    void SetForeground(const bool& isForeground);
    bool GetForeground();
    void RemoveInnerTask(const std::string& name);
    void InitMainLooperWatcher(WatchdogInnerBeginFunc* beginFunc, WatchdogInnerEndFunc* endFunc);
    void SetAppDebug(bool isAppDebug);
    bool GetAppDebug();
    int SetEventConfig(const std::map<std::string, std::string>& paramsMap);
    int ConfigEventPolicy(const std::map<std::string, std::string>& paramsMap);
    bool SampleStackDetect(const TimePoint& endTime, int& reportTimes, int updateTimes, int ignoreTime,
        bool isScroll = false);
    void CollectTraceDetect(const TimePoint& endTime, int64_t durationTime);
    void SetSpecifiedProcessName(const std::string& name);
    std::string GetSpecifiedProcessName();
    void SetScrollState(bool isScroll);
    void StartSample(int duration, int interval);
    std::string StopSample(int sampleCount);
    bool CheckSample(const TimePoint& endTime, int64_t durationTime);
    SamplerResult GetSamplerResult();

public:
    std::string currentScene_;
    TimePoint bussinessBeginTime_;
    TimeContent timeContent_ {0};
    StackContent stackContent_;
    TraceContent traceContent_;
    std::map<std::string, int> jankParamsMap = {
        {KEY_SAMPLE_INTERVAL, SAMPLE_DEFAULT_INTERVAL}, {KEY_IGNORE_STARTUP_TIME, DEFAULT_IGNORE_STARTUP_TIME},
        {KEY_SAMPLE_COUNT, SAMPLE_DEFAULT_COUNT}, {KEY_SAMPLE_REPORT_TIMES, SAMPLE_DEFAULT_REPORT_TIMES},
        {KEY_LOG_TYPE, 0}, {KEY_SET_TIMES_FLAG, SET_TIMES_FLAG}, {KEY_CHECKER_INTERVAL, 0}, {KEY_AUTO_STOP_SAMPLING, 0}
    };
    bool isScroll_ {false};

private:
    bool Start();
    bool Stop();
    bool SendMsgToHungtask(const std::string& msg);
    bool KickWatchdog();
    bool IsTaskExistLocked(const std::string& name);
    bool IsExceedMaxTaskLocked();
    int64_t InsertWatchdogTaskLocked(const std::string& name, WatchdogTask&& task);
#ifdef SUSPEND_CHECK_ENABLE
    bool IsInSleep(const WatchdogTask& queuedTaskCheck);
#endif
    void CheckKickWatchdog(uint64_t now, const WatchdogTask& queuedTask);
    bool CheckCurrentTaskLocked(const WatchdogTask& queuedTaskCheck);
    uint64_t FetchNextTask(uint64_t now, WatchdogTask& task);
    void ReInsertTaskIfNeed(WatchdogTask& task);
    void CreateWatchdogThreadIfNeed();
    bool ReportMainThreadEvent(int64_t tid, std::string eventName, bool isScroll = false, bool appStart = false);
    bool CheckEventTimer(int64_t currentTime, int64_t reportBegin, int64_t reportEnd, int interval);
    void DumpTraceTask(int32_t interval);
    int32_t StartTraceProfile();
    void UpdateTime(int64_t& reportBegin, int64_t& reportEnd, TimePoint& lastEndTime, const TimePoint& endTime);
    bool CheckThreadSampler(bool recordSubmitterStack);
    bool InitThreadSamplerFuncs();
    void ResetThreadSamplerFuncs();
    static void GetFfrtTaskTid(int32_t& tid, const std::string& msg);
    void UpdateJankParam(SampleJankParams& params);
    int ConvertStrToNum(const std::map<std::string, std::string>& paramsMap, const std::string& key,
        int defaultValue = -1);
    bool GetAutoStopSampling(const std::map<std::string, std::string>& paramsMap, int& autoStopSampling);
    bool CheckSampleParam(const std::map<std::string, std::string>& paramsMap, bool keyNeedExist = true);
    std::string SaveFreezeStackToFile(int32_t pid);
    bool AppStartSample(bool isScroll, AppStartContent& startContent);
    void ClearParam(bool& isFinished);
    void UpdateAppStartContent(const std::map<std::string, int64_t>& paramsMap, AppStartContent& startContent);
    void ParseAppStartParams(const std::string& line, const std::string& eventName);
    void ReadAppStartConfig(const std::string& filePath);
    bool EnableAppStartSample(AppStartContent& startContent, int64_t durationTime, bool isScroll);
    std::string GetBundleName();
#if defined(__aarch64__)
    void InitAsyncStackIfNeed();
    bool NeedOpenAsyncStack();
#endif

    static void ThreadSamplerSigHandler(int sig, siginfo_t* si, void* context);
    bool InstallThreadSamplerSignal();
    void UninstallThreadSamplerSignal();
    void ResetFreezeSampleFlags();

    static SigActionType threadSamplerSigHandler_;
    std::priority_queue<WatchdogTask> checkerQueue_; // protected by lock_
    std::unique_ptr<std::thread> threadLoop_;
    std::mutex lock_;
    static std::mutex lockFfrt_;
    std::condition_variable condition_;
    std::atomic_bool isNeedStop_ = false;
    std::once_flag flag_;
    std::set<std::string> taskNameSet_;
    std::set<int64_t> buissnessThreadInfo_;
    std::shared_ptr<AppExecFwk::EventRunner> mainRunner_;
    int cntCallback_;
    time_t timeCallback_;
    bool isHmos = false;
    void* threadSamplerFuncHandler_  {nullptr};
    ThreadSamplerInitFunc threadSamplerInitFunc_ {nullptr};
    ThreadSamplerSampleFunc threadSamplerSampleFunc_ {nullptr};
    ThreadSamplerCollectFunc threadSamplerCollectFunc_ {nullptr};
    ThreadSamplerDeinitFunc threadSamplerDeinitFunc_ {nullptr};
    ThreadSamplerGetResultFunc threadSamplerGetResultFunc_ {nullptr};
    SamplerResult samplerResult_ {0, 0, 0};
    uint64_t watchdogStartTime_ {0};
    static std::mutex threadSamplerSignalMutex_;

    bool isMainThreadStackEnabled_ {false};
    bool isMainThreadTraceEnabled_ {false};
    std::string bundleName_;
    std::string bundleVersion_;
    bool isSystemApp_ { false };
    bool isForeground_ {false};
    bool isAppDebug_ {false};
    std::shared_ptr<UCollectClient::TraceCollector> traceCollector_;
    UCollectClient::AppCaller appCaller_ {
        .actionId = 0,
        .foreground = 0,
        .uid = 0,
        .pid = 0,
        .happenTime = 0,
        .beginTime = 0,
        .endTime = 0,
        .isBusinessJank = false,
    };
    std::string specifiedProcessName_;
    uint64_t nextWeakUpTime_ {UINT64_MAX};
    AppStartContent startSlowContent_;
    AppStartContent scrollSlowContent_;
    SampleFreezeInfo sampleFreezeInfo_;
    bool initAsyncStack_ {false};
};
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif
