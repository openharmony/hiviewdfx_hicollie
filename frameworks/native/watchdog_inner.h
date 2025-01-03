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

namespace OHOS {
namespace HiviewDFX {

using TimePoint = AppExecFwk::InnerEvent::TimePoint;
struct TimeContent {
    int64_t reportBegin;
    int64_t reportEnd;
    int64_t curBegin;
    int64_t curEnd;
};

struct StackContent {
    int stackState;
    int detectorCount;
    int collectCount;
};

struct TraceContent {
    int traceState;
    int traceCount;
    int dumpCount;
};

typedef void (*WatchdogInnerBeginFunc)(const char* eventName);
typedef void (*WatchdogInnerEndFunc)(const char* eventName);

typedef int (*ThreadSamplerInitFunc)(int);
typedef int32_t (*ThreadSamplerSampleFunc)();
typedef int (*ThreadSamplerCollectFunc)(char*, size_t, int);
typedef int (*ThreadSamplerDeinitFunc)();
typedef void (*SigActionType)(int, siginfo_t*, void*);

class WatchdogInner : public Singleton<WatchdogInner> {
    DECLARE_SINGLETON(WatchdogInner);
public:
    static const int XCOLLIE_CALLBACK_HISTORY_MAX = 5;
    static const int XCOLLIE_CALLBACK_TIMEWIN_MAX = 60;
    std::map<int64_t, int> taskIdCnt;
    int AddThread(const std::string &name, std::shared_ptr<AppExecFwk::EventHandler> handler, uint64_t interval);
    int AddThread(const std::string &name, std::shared_ptr<AppExecFwk::EventHandler> handler,
        TimeOutCallback timeOutCallback, uint64_t interval);
    void RunOneShotTask(const std::string& name, Task&& task, uint64_t delay);
    void RunPeriodicalTask(const std::string& name, Task&& task, uint64_t interval, uint64_t delay);
    int64_t RunXCollieTask(const std::string& name, uint64_t timeout, XCollieCallback func, void *arg,
        unsigned int flag);
    void RemoveXCollieTask(int64_t id);
    int64_t SetTimerCountTask(const std::string &name, uint64_t timeLimit, int countLimit);
    void TriggerTimerCountTask(const std::string &name, bool bTrigger, const std::string &message);
    void StopWatchdog();
    bool IsCallbackLimit(unsigned int flag);
    void IpcCheck();
    void InitFfrtWatchdog();
    static void WriteStringToFile(uint32_t pid, const char *str);
    static void FfrtCallback(uint64_t taskId, const char *taskInfo, uint32_t delayedTaskCount);
    static void SendFfrtEvent(const std::string &msg, const std::string &eventName, const char *taskInfo);
    static void LeftTimeExitProcess(const std::string &description);
    static void KillPeerBinderProcess(const std::string &description);
    int32_t StartProfileMainThread(int32_t interval);
    bool CollectStack(std::string& stack);
    void CollectTrace();
    bool Deinit();
    void SetBundleInfo(const std::string& bundleName, const std::string& bundleVersion);
    void SetForeground(const bool& isForeground);
    void ChangeState(int& state, int targetState);
    void DayChecker(int& state, TimePoint currenTime, TimePoint lastEndTime, int64_t checkTimer);
    void RemoveInnerTask(const std::string& name);
    void InitMainLooperWatcher(WatchdogInnerBeginFunc* beginFunc, WatchdogInnerEndFunc* endFunc);
    void SetAppDebug(bool isAppDebug);
    bool GetAppDebug();
    std::string currentScene_;
    TimePoint lastTraceTime_;
    TimePoint lastStackTime_;
    TimePoint bussinessBeginTime_;
    TimeContent timeContent_ {0};
    StackContent stackContent_ {0};
    TraceContent traceContent_ {0};

private:
    bool Start();
    bool Stop();
    bool SendMsgToHungtask(const std::string& msg);
    bool KickWatchdog();
    bool IsTaskExistLocked(const std::string& name);
    bool IsExceedMaxTaskLocked();
    int64_t InsertWatchdogTaskLocked(const std::string& name, WatchdogTask&& task);
    bool IsInSleep(const WatchdogTask& queuedTaskCheck);
    void CheckIpcFull(uint64_t now, const WatchdogTask& queuedTask);
    uint64_t FetchNextTask(uint64_t now, WatchdogTask& task);
    void ReInsertTaskIfNeed(WatchdogTask& task);
    void CreateWatchdogThreadIfNeed();
    bool ReportMainThreadEvent();
    bool CheckEventTimer(const int64_t& currentTime);
    void StartTraceProfile(int32_t interval);
    void ThreadSampleTask();
    bool InitThreadSamplerFuncs();
    void ResetThreadSamplerFuncs();
    static void GetFfrtTaskTid(int32_t& tid, const std::string& msg);

    static void ThreadSamplerSigHandler(int sig, siginfo_t* si, void* context);
    bool InstallThreadSamplerSignal();
    void UninstallThreadSamplerSignal();

    static SigActionType threadSamplerSigHandler_;

    static const unsigned int MAX_WATCH_NUM = 128; // 128: max handler thread
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
    std::shared_ptr<AppExecFwk::EventHandler> binderCheckHander_;
    int cntCallback_;
    time_t timeCallback_;
    bool isHmos = false;
    void* threadSamplerFuncHandler_  {nullptr};
    ThreadSamplerInitFunc threadSamplerInitFunc_ {nullptr};
    ThreadSamplerSampleFunc threadSamplerSampleFunc_ {nullptr};
    ThreadSamplerCollectFunc threadSamplerCollectFunc_ {nullptr};
    ThreadSamplerDeinitFunc threadSamplerDeinitFunc_ {nullptr};
    uint64_t watchdogStartTime_ {0};
    static std::mutex threadSamplerSignalMutex_;

    bool isMainThreadProfileTaskEnabled_ {false};
    bool isMainThreadTraceEnabled_ {false};
    std::string bundleName_;
    std::string bundleVersion_;
    bool isForeground_ {false};
    bool isAppDebug_ {false};
    int sampleTaskState_;
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
};
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif
