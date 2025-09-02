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

#include "watchdog_inner.h"

#include <cerrno>
#include <climits>
#include <cstdio>
#include <mutex>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <csignal>
#include <string>

#include <securec.h>
#include <dlfcn.h>
#include "musl_preinit_common.h"
#include "backtrace_local.h"
#ifdef HISYSEVENT_ENABLE
#include "hisysevent.h"
#endif
#include "ipc_skeleton.h"
#include "xcollie_utils.h"
#include "dfx_define.h"
#include "dfx_dumprequest.h"
#include "parameter.h"
#include "file_ex.h"

typedef void(*ThreadInfoCallBack)(char* buf, size_t len, void* ucontext);
extern "C" void SetThreadInfoCallback(ThreadInfoCallBack func) __attribute__((weak));
namespace OHOS {
namespace HiviewDFX {
namespace {
enum DumpStackState {
    DEFAULT = 0,
    COMPLETE = 1,
    SAMPLE_COMPLETE = 2
};
enum CatchLogType {
    LOGTYPE_DEFAULT = -1,
    LOGTYPE_NONE = 0,
    LOGTYPE_SAMPLE_STACK = 1,
    LOGTYPE_COLLECT_TRACE = 2
};
constexpr char STACK_CHECKER[] = "ThreadSampler";
constexpr char TRACE_CHECKER[] = "TraceCollector";
constexpr const char* const FREEZE_SAMPLE = "FreezeSampler";
constexpr int ONE_DAY_LIMIT = 24 * 60 * 60 * 1000;
constexpr int ONE_HOUR_LIMIT = 60 * 60 * 1000;
constexpr int MILLISEC_TO_NANOSEC = 1000 * 1000;
const int FFRT_BUFFER_SIZE = 512 * 1024;
const int DETECT_STACK_COUNT = 2;
const int COLLECT_STACK_COUNT = 10;
const int COLLECT_TRACE_MIN = 1;
const int COLLECT_TRACE_MAX = 20;
const int DURATION_TIME = 150;
const int DISTRIBUTE_TIME = 2000;
const int DUMPTRACE_TIME = 450;
constexpr const char* const KEY_SCB_STATE = "com.ohos.sceneboard";
constexpr uint64_t DEFAULT_TIMEOUT = 60 * 1000;
constexpr uint32_t FFRT_CALLBACK_TIME = 30 * 1000;
constexpr uint32_t TIME_MS_TO_S = 1000;
constexpr int INTERVAL_KICK_TIME = 6 * 1000;
constexpr uint32_t AUDIO_SERVER_UID = 1041;
constexpr uint32_t DATA_MANAGE_SERVICE_UID = 3012;
constexpr uint32_t FOUNDATION_UID = 5523;
constexpr uint32_t RENDER_SERVICE_UID = 1003;
constexpr int SERVICE_WARNING = 1;
const char* SYS_KERNEL_HUNGTASK_USERLIST = "/sys/kernel/hungtask/userlist";
const char* HUNGTASK_USERLIST = "/proc/sys/hguard/user_list";
const char* ON_KICK_TIME = "on,72";
const char* ON_KICK_TIME_EXTRA = "on,10,foundation";
const char* KICK_TIME = "kick";
const char* KICK_TIME_EXTRA = "kick,foundation";
const int32_t NOT_OPEN = -1;
const char* LIB_THREAD_SAMPLER_PATH = "libthread_sampler.z.so";
constexpr size_t STACK_LENGTH = 128 * 1024;
constexpr uint64_t DEFAULE_SLEEP_TIME = 2 * 1000;
constexpr uint32_t JOIN_IPC_FULL_UIDS[] = {
    AUDIO_SERVER_UID, DATA_MANAGE_SERVICE_UID,
    FOUNDATION_UID, RENDER_SERVICE_UID
};
constexpr uint64_t MIN_IPC_CHECK_INTERVAL = 10;
constexpr uint64_t MAX_IPC_CHECK_INTERVAL = 30;
constexpr uint64_t SAMPLE_STACK_MAP_SIZE = 5;
constexpr uint64_t SAMPLE_TRACE_MAP_SIZE = 1;
constexpr int MAX_SAMPLE_STACK_TIMES = 2500; // 2.5s
constexpr int SAMPLE_INTERVAL_MIN = 50; // 50ms
constexpr int SAMPLE_INTERVAL_MAX = 500; // 500ms
constexpr int SAMPLE_COUNT_MIN = 1;
constexpr int SAMPLE_REPORT_TIMES_MIN = 1;
constexpr int SAMPLE_REPORT_TIMES_MAX = 3;
constexpr int SAMPLE_EXTRA_COUNT = 4;
constexpr int IGNORE_STARTUP_TIME_MIN = 3; // 3s
constexpr int SCROLL_INTERVAL = 50; // 50ms
constexpr int DEFAULT_SAMPLE_VALUE = 1;
constexpr int CPU_FREQ_DECIMAL_BASE = 10;
constexpr const char* const SCROLL_JANK = "SCROLL_JANK";
constexpr const char* const MAIN_THREAD_JANK = "MAIN_THREAD_JANK";
constexpr const char* const BUSSINESS_THREAD_JANK = "BUSSINESS_THREAD_JANK";
}

std::mutex WatchdogInner::lockFfrt_;
static uint64_t g_lastKickTime = GetCurrentTickMillseconds();
static int32_t g_fd = NOT_OPEN;
static bool g_existFile = true;

static std::atomic_int g_scrollSampleCount {0};
static std::atomic_int g_freezeSampleCount {0};
static std::atomic_bool g_freezeTaskFinished {false};
static std::atomic_bool g_isReuseStack {false};
static std::atomic_bool g_isDumpStack {false};

SigActionType WatchdogInner::threadSamplerSigHandler_ = nullptr;
std::mutex WatchdogInner::threadSamplerSignalMutex_;

namespace {
void ThreadInfo(char *buf  __attribute__((unused)),
                size_t len  __attribute__((unused)),
                void* ucontext  __attribute__((unused)))
{
    if (ucontext == nullptr) {
        return;
    }

    auto ret = memcpy_s(buf, len, WatchdogInner::GetInstance().currentScene_.c_str(),
        WatchdogInner::GetInstance().currentScene_.size());
    if (ret != 0) {
        return;
    }
}

void SetThreadSignalMask(int signo, bool isAddSignal, bool isBlock)
{
    sigset_t set;
    sigemptyset(&set);
    pthread_sigmask(SIG_SETMASK, nullptr, &set);
    if (isAddSignal) {
        sigaddset(&set, signo);
    } else {
        sigdelset(&set, signo);
    }
    if (isBlock) {
        pthread_sigmask(SIG_BLOCK, &set, nullptr);
    } else {
        pthread_sigmask(SIG_UNBLOCK, &set, nullptr);
    }
}

static const int CRASH_SIGNAL_LIST[] = {
    SIGILL, SIGABRT, SIGBUS, SIGFPE,
    SIGSEGV, SIGSTKFLT, SIGSYS, SIGTRAP
};
}

WatchdogInner::WatchdogInner()
    : cntCallback_(0), timeCallback_(0)
{
    currentScene_ = "thread DfxWatchdog: Current scenario is hicollie.\n";
}

WatchdogInner::~WatchdogInner()
{
    Stop();
}

static bool IsInAppspwan()
{
    if (getuid() == 0 && GetSelfProcName().find("appspawn") != std::string::npos) {
        return true;
    }

    if (getuid() == 0 && GetSelfProcName().find("nativespawn") != std::string::npos) {
        return true;
    }

    if (getuid() == 0 && GetSelfProcName().find("hybridspawn") != std::string::npos) {
        return true;
    }

    return false;
}

void WatchdogInner::SetBundleInfo(const std::string& bundleName, const std::string& bundleVersion)
{
    bundleName_ = bundleName;
    bundleVersion_ = bundleVersion;
}

void WatchdogInner::SetForeground(const bool& isForeground)
{
    isForeground_ = isForeground;
}

bool WatchdogInner::GetForeground()
{
    return isForeground_;
}

bool WatchdogInner::ReportMainThreadEvent(int64_t tid, std::string eventName, bool isScroll,
    bool appStart)
{
    std::string stack = "";
    std::string heaviestStack = "";
    if (!CollectStack(stack, heaviestStack)) {
        stack = "";
        heaviestStack = "";
    }

    std::string path = "";
    int32_t pid = getprocpid();
    bool isOverLimit = false;
    if (!WriteStackToFd(pid, path, stack, eventName, isOverLimit)) {
        XCOLLIE_LOGI("MainThread WriteStackToFd Failed");
        return false;
    }
#ifdef HISYSEVENT_ENABLE
    int result = -1;
    if (appStart) {
        isScroll ? scrollSlowContent_.reportTimes-- : startSlowContent_.reportTimes--;
        result = HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, eventName,
            HiSysEvent::EventType::FAULT, "PROCESS_NAME", GetSelfProcName(),
            "STACK", stack);
        XCOLLIE_LOGI("AppStart HiSysEventWrite result=%{public}d, isScroll=%{public}d, "
            "eventName=%{public}s", result, isScroll, eventName.c_str());
        return result >= 0;
    }
    if (!isScroll) {
        result = HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, "MAIN_THREAD_JANK",
            HiSysEvent::EventType::FAULT,
            "BUNDLE_VERSION", bundleVersion_, "BUNDLE_NAME", bundleName_,
            "BEGIN_TIME", stackContent_.reportBegin / MILLISEC_TO_NANOSEC,
            "END_TIME", stackContent_.reportEnd / MILLISEC_TO_NANOSEC,
            "EXTERNAL_LOG", path, "STACK", stack, "JANK_LEVEL", 0,
            "THREAD_NAME", GetSelfProcName(), "FOREGROUND", isForeground_,
            "LOG_TIME", GetTimeStamp() / MILLISEC_TO_NANOSEC,
            "APP_START_JIFFIES_TIME", GetAppStartTime(pid, tid), "HEAVIEST_STACK", heaviestStack,
            "LOG_OVER_LIMIT", isOverLimit);
    } else {
        result = HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, "SCROLL_TIMEOUT",
            HiSysEvent::EventType::FAULT, "PROCESS_NAME", GetSelfProcName(),
            "EXTERNAL_LOG", path, "LOG_OVER_LIMIT", isOverLimit);
    }

    XCOLLIE_LOGI("MainThread HiSysEventWrite result=%{public}d, isScroll=%{public}d", result, isScroll);
    return result >= 0;
#else
    XCOLLIE_LOGI("hisysevent not exists");
#endif
}

bool WatchdogInner::CheckEventTimer(int64_t currentTime, int64_t reportBegin, int64_t reportEnd, int interval)
{
    if (reportBegin == timeContent_.curBegin &&
        reportEnd == timeContent_.curEnd) {
        return false;
    }
    return (timeContent_.curEnd <= timeContent_.curBegin &&
        (currentTime - timeContent_.curBegin >= interval * MILLISEC_TO_NANOSEC)) ||
        (timeContent_.curEnd - timeContent_.curBegin > interval * MILLISEC_TO_NANOSEC);
}

void WatchdogInner::ThreadSamplerSigHandler(int sig, siginfo_t* si, void* context)
{
    std::lock_guard<std::mutex> lock(threadSamplerSignalMutex_);
    if (WatchdogInner::threadSamplerSigHandler_ == nullptr) {
        return;
    }
    WatchdogInner::threadSamplerSigHandler_(sig, si, context);
}

bool WatchdogInner::InstallThreadSamplerSignal()
{
    struct sigaction action {};
    sigfillset(&action.sa_mask);
    for (size_t i = 0; i < sizeof(CRASH_SIGNAL_LIST) / sizeof(CRASH_SIGNAL_LIST[0]); i++) {
        sigdelset(&action.sa_mask, CRASH_SIGNAL_LIST[i]);
    }
    action.sa_sigaction = WatchdogInner::ThreadSamplerSigHandler;
    action.sa_flags = SA_RESTART | SA_SIGINFO;
    if (sigaction(MUSL_SIGNAL_SAMPLE_STACK, &action, nullptr) != 0) {
        XCOLLIE_LOGE("Failed to register signal(%{public}d:%{public}d)", MUSL_SIGNAL_SAMPLE_STACK, errno);
        return false;
    }
    return true;
}

void WatchdogInner::UninstallThreadSamplerSignal()
{
    std::lock_guard<std::mutex> lock(threadSamplerSignalMutex_);
    threadSamplerSigHandler_ = nullptr;
}

bool WatchdogInner::CheckThreadSampler()
{
    XCOLLIE_LOGD("ThreadSampler 1st in ThreadSamplerTask.\n");
    if (!InitThreadSamplerFuncs()) {
        XCOLLIE_LOGE("ThreadSampler initialize failed.\n");
        return false;
    }

    if (!InstallThreadSamplerSignal()) {
        XCOLLIE_LOGE("ThreadSampler install signal failed.\n");
        return false;
    }

    int initThreadSamplerRet = threadSamplerInitFunc_(COLLECT_STACK_COUNT);
    if (initThreadSamplerRet != 0) {
        XCOLLIE_LOGE("Thread sampler init failed. ret %{public}d\n", initThreadSamplerRet);
        return false;
    }
    return true;
}

bool WatchdogInner::InitThreadSamplerFuncs()
{
    threadSamplerFuncHandler_ = dlopen(LIB_THREAD_SAMPLER_PATH, RTLD_LAZY);
    if (threadSamplerFuncHandler_ == nullptr) {
        XCOLLIE_LOGE("dlopen failed, funcHandler is nullptr.\n");
        return false;
    }

    threadSamplerInitFunc_ =
        reinterpret_cast<ThreadSamplerInitFunc>(FunctionOpen(threadSamplerFuncHandler_, "ThreadSamplerInit"));
    threadSamplerSampleFunc_ =
        reinterpret_cast<ThreadSamplerSampleFunc>(FunctionOpen(threadSamplerFuncHandler_, "ThreadSamplerSample"));
    threadSamplerCollectFunc_ =
        reinterpret_cast<ThreadSamplerCollectFunc>(FunctionOpen(threadSamplerFuncHandler_, "ThreadSamplerCollect"));
    threadSamplerDeinitFunc_ =
        reinterpret_cast<ThreadSamplerDeinitFunc>(FunctionOpen(threadSamplerFuncHandler_, "ThreadSamplerDeinit"));
    threadSamplerSigHandler_ =
        reinterpret_cast<SigActionType>(FunctionOpen(threadSamplerFuncHandler_, "ThreadSamplerSigHandler"));
    if (threadSamplerInitFunc_ == nullptr || threadSamplerSampleFunc_ == nullptr ||
        threadSamplerCollectFunc_ == nullptr || threadSamplerDeinitFunc_ == nullptr ||
        threadSamplerSigHandler_ == nullptr) {
        ResetThreadSamplerFuncs();
        XCOLLIE_LOGE("ThreadSampler dlsym some function failed.\n");
        return false;
    }
    XCOLLIE_LOGI("ThreadSampler has been successfully loaded.\n");
    return true;
}

void WatchdogInner::ResetThreadSamplerFuncs()
{
    threadSamplerInitFunc_ = nullptr;
    threadSamplerSampleFunc_ = nullptr;
    threadSamplerCollectFunc_ = nullptr;
    threadSamplerDeinitFunc_ = nullptr;
    threadSamplerSigHandler_ = nullptr;
    dlclose(threadSamplerFuncHandler_);
    threadSamplerFuncHandler_ = nullptr;
}

void WatchdogInner::UpdateTime(int64_t& reportBegin, int64_t& reportEnd,
    TimePoint& lastEndTime, const TimePoint& endTime)
{
    reportBegin = timeContent_.curBegin;
    reportEnd = timeContent_.curEnd;
    lastEndTime = endTime;
}

bool WatchdogInner::SampleStackDetect(const TimePoint& endTime, int& reportTimes,
    int updateTimes, int ignoreTime, bool isScroll)
{
    uint64_t startUpTime = static_cast<uint64_t>(ignoreTime) * TIME_MS_TO_S;
    if (GetCurrentTickMillseconds() - watchdogStartTime_ < startUpTime) {
        XCOLLIE_LOGI("Application is in starting period.\n");
        return false;
    }
    if (!stackContent_.isStartSampleEnabled) {
        XCOLLIE_LOGI("Current sample detection task is being executed.\n");
        return false;
    }
    if (reportTimes <= 0) {
        int32_t checkTimer = ONE_DAY_LIMIT;
        int32_t checkInterval = jankParamsMap[KEY_CHECKER_INTERVAL];
        if (checkInterval > 0) {
            checkTimer = checkInterval;
        } else if (!isScroll && (IsDeveloperOpen() ||
            (IsBetaVersion() && GetProcessNameFromProcCmdline(getpid()) == KEY_SCB_STATE))) {
            checkTimer = ONE_HOUR_LIMIT;
        }
        auto diff = endTime - stackContent_.lastEndTime;
        int64_t intervalTime = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
        if (intervalTime < checkTimer) {
            return false;
        }
        reportTimes = updateTimes;
        XCOLLIE_LOGI("Update the currentThread's reportTimes: %{public}d", reportTimes);
    }
    stackContent_.isStartSampleEnabled = false;
    UpdateTime(stackContent_.reportBegin, stackContent_.reportEnd, stackContent_.lastEndTime, endTime);
    return true;
}

bool WatchdogInner::AppStartSample(bool isScroll, AppStartContent& startContent)
{
    int64_t tid = getproctid();
    startContent.collectCount.store(0);
    startContent.isStartSampleEnabled = false;
    auto sampleTask = [this, tid, isScroll, &startContent]() {
        if (startContent.collectCount.load() == 0 && g_isDumpStack) {
            startContent.isFinishStartSample = true;
            return;
        }
        if ((startContent.collectCount.load() == 0 && !CheckThreadSampler()) || threadSamplerSampleFunc_ == nullptr) {
            g_isDumpStack.store(false);
            startContent.isFinishStartSample = true;
            return;
        }
        if (startContent.collectCount.load() < startContent.targetCount) {
            g_isDumpStack.store(true);
            threadSamplerSampleFunc_();
        } else {
            std::string eventName;
            if (isScroll) {
                eventName = EVENT_APP_START_SCROLL_JANK;
            } else {
                eventName = EVENT_APP_START_JANK;
            }
            ReportMainThreadEvent(tid, eventName, isScroll, true);
            g_isDumpStack.store(false);
            startContent.isFinishStartSample = true;
            return;
        }
        startContent.collectCount.fetch_add(DEFAULT_SAMPLE_VALUE);
    };
    WatchdogTask task(APP_START_SAMPLE, sampleTask, 0, startContent.sampleInterval, false);
    std::unique_lock<std::mutex> lock(lock_);
    if (!InsertWatchdogTaskLocked(APP_START_SAMPLE, std::move(task))) {
        return false;
    }
    return true;
}

bool WatchdogInner::EnableAppStartSample(AppStartContent& startContent, int64_t durationTime, bool isScroll)
{
    if (!startContent.enableStartSample.load()) {
        return false;
    }
    if (!OHOS::FileExists(APP_START_CONFIG)) {
        startContent.enableStartSample.store(false);
        XCOLLIE_LOGD("file:%{public}s not exist, errno:%{public}d", APP_START_CONFIG, errno);
        return false;
    }
    if (!isScroll && (GetCurrentTickMillseconds() - watchdogStartTime_) >
        static_cast<uint64_t>(startContent.startUpDuration)) {
        startContent.enableStartSample.store(false);
        XCOLLIE_LOGD("CurrentThread is not in starting period. startUpDuration:%{public}" PRId64,
            startContent.startUpDuration);
        return false;
    }
    int64_t curTime = GetTimeStamp() / SEC_TO_MICROSEC;
    if (curTime - startContent.startTime > APP_START_LIMIT) {
        startContent.enableStartSample.store(false);
        XCOLLIE_LOGD("The time for detecting the slow startup of an application exceeds the limit, "
            "curTime:%{public}" PRId64, curTime);
        return false;
    }
    if (startContent.reportTimes <= 0) {
        if (!isScroll) {
            startContent.enableStartSample.store(false);
        }
        return isScroll;
    }
    if (!startContent.isStartSampleEnabled) {
        XCOLLIE_LOGD("Current app start detection task is being executed.");
        return true;
    }
    if (durationTime < startContent.threshold) {
        return false;
    }
    return AppStartSample(isScroll, startContent);
}

bool WatchdogInner::CheckSample(const TimePoint& endTime, int64_t durationTime)
{
    bool isScroll = isScroll_;
    if (!isScroll) {
        return EnableAppStartSample(startSlowContent_, durationTime, isScroll);
    }
    if (!EnableAppStartSample(scrollSlowContent_, durationTime, isScroll) &&
        durationTime > SCROLL_INTERVAL && !scrollSlowContent_.enableStartSample.load()) {
        return StartScrollProfile(endTime, durationTime, SCROLL_INTERVAL);
    }
    return false;
}

bool WatchdogInner::StartScrollProfile(const TimePoint& endTime, int64_t durationTime, int sampleInterval)
{
    bool isScroll = true;
    if (!SampleStackDetect(endTime, stackContent_.scrollTimes, SAMPLE_DEFULE_REPORT_TIMES,
        DEFAULT_IGNORE_STARTUP_TIME, isScroll)) {
        return false;
    }
    XCOLLIE_LOGI("StartScrollProfile durationTime: %{public}" PRId64 " ms, sampleInterval: %{public}d "
        "isScroll: %{public}d.", durationTime, sampleInterval, isScroll);
    int64_t tid = getproctid();
    g_scrollSampleCount.store(0);
    auto sampleTask = [this, sampleInterval, tid, isScroll]() {
        if (g_scrollSampleCount.load() == 0 && (g_isDumpStack || !CheckThreadSampler())) {
            isMainThreadStackEnabled_ = true;
            return;
        }
        if (threadSamplerSampleFunc_ == nullptr) {
            isMainThreadStackEnabled_ = true;
            return;
        }
        if (g_scrollSampleCount.load() == 0) {
            g_isDumpStack.store(true);
            threadSamplerSampleFunc_();
            g_scrollSampleCount.fetch_add(DEFAULT_SAMPLE_VALUE);
        } else {
            ReportMainThreadEvent(tid, SCROLL_JANK, isScroll);
            stackContent_.scrollTimes--;
            g_isDumpStack.store(false);
            isMainThreadStackEnabled_ = true;
        }
    };
    WatchdogTask task("ThreadSampler", sampleTask, 0, sampleInterval, true);
    std::unique_lock<std::mutex> lock(lock_);
    InsertWatchdogTaskLocked("ThreadSampler", std::move(task));
    return true;
}

void WatchdogInner::StartProfileMainThread(const TimePoint& endTime, int64_t durationTime, int sampleInterval)
{
    std::unique_lock<std::mutex> lock(lock_);
    if (!SampleStackDetect(endTime, stackContent_.reportTimes,
        jankParamsMap[KEY_SAMPLE_REPORT_TIMES], jankParamsMap[KEY_IGNORE_STARTUP_TIME])) {
        return;
    }
    XCOLLIE_LOGI("StartProfileMainThread durationTime: %{public}" PRId64 " ms, sampleInterval: %{public}d.",
        durationTime, sampleInterval);
    stackContent_.detectorCount = 0;
    stackContent_.collectCount = 0;
    int sampleCount = jankParamsMap[KEY_SAMPLE_COUNT];
    int64_t tid = getproctid();
    auto sampleTask = [this, sampleInterval, sampleCount, tid]() {
        if ((stackContent_.detectorCount == 0 && stackContent_.collectCount == 0 &&
            (g_isDumpStack || !CheckThreadSampler())) || threadSamplerSampleFunc_ == nullptr) {
            isMainThreadStackEnabled_ = true;
            return;
        }
        if (stackContent_.collectCount > DumpStackState::DEFAULT &&
            stackContent_.collectCount < sampleCount) {
            g_isDumpStack.store(true);
            threadSamplerSampleFunc_();
            stackContent_.collectCount++;
        } else if (stackContent_.collectCount == sampleCount) {
            g_isDumpStack.store(false);
            std::string eventName = buissnessThreadInfo_.empty() ? MAIN_THREAD_JANK : BUSSINESS_THREAD_JANK;
            ReportMainThreadEvent(tid, eventName);
            stackContent_.reportTimes--;
            isMainThreadStackEnabled_ = true;
            return;
        } else {
            if (CheckEventTimer(GetTimeStamp(), stackContent_.reportBegin,
                stackContent_.reportEnd, sampleInterval)) {
                threadSamplerSampleFunc_();
                stackContent_.collectCount++;
            } else {
                stackContent_.detectorCount++;
            }
        }
        if (stackContent_.detectorCount == DETECT_STACK_COUNT) {
            isMainThreadStackEnabled_ = true;
        }
    };
    WatchdogTask task("ThreadSampler", sampleTask, 0, sampleInterval, true);
    InsertWatchdogTaskLocked("ThreadSampler", std::move(task));
}

void WatchdogInner::SaveFreezeStackToFile(const std::string& outFile, int32_t pid)
{
    if (!CreateDir(FREEZE_DIR)) {
        XCOLLIE_LOGE("Path to realPath failed.");
        return;
    }
    std::string stack;
    std::string heaviestStack;
    CollectStack(stack, heaviestStack, 0);
    std::string info = "#ThreadInfos Tid: " + std::to_string(pid) + ", Name: " + bundleName_ + "\n";
    if (g_isReuseStack) {
        info += "The current thread is collecting the stack, which conflicts with the main thread jank event."
            " Reuse the current stack.";
        g_isReuseStack.store(false);
    }
    info += stack;
    ClearFreezeFileIfNeed(info.size());
    bool saveRet = SaveStringToFile(FREEZE_DIR + outFile, info);
    g_isDumpStack.store(false);
    g_freezeTaskFinished.store(true);
    XCOLLIE_LOGI("Save freeze stack to file, ret:%{public}d, isReuseStack:%{public}d.",
        saveRet, g_isReuseStack.load());
}

void WatchdogInner::StartSample(int duration, int interval, std::string& outFile)
{
    std::unique_lock<std::mutex> lock(lock_);
    if (IsTaskExistLocked(FREEZE_SAMPLE)) {
        XCOLLIE_LOGW("StartSample task already exit, skip this task.");
        return;
    }
    if (duration <= 0 || interval <= 0) {
        XCOLLIE_LOGW("StartSample failed, duration=%{public}d, interval=%{public}d.",
            duration, interval);
        return;
    }
    int targetCount = duration / interval;
    if (targetCount < DEFAULT_SAMPLE_VALUE) {
        XCOLLIE_LOGW("StartSample failed, sampleCount=%{public}d", targetCount);
        return;
    }
    int32_t pid = getpid();
    g_freezeSampleCount.store(0);
    outFile = "freeze_" + GetFormatDate() + "_" + std::to_string(pid) + ".txt";
    auto sampleTask = [this, pid, targetCount, outFile]() {
        if ((g_freezeSampleCount.load() == 0 && !CheckThreadSampler()) || threadSamplerSampleFunc_ == nullptr) {
            g_freezeTaskFinished.store(true);
            return;
        }
        if (g_freezeSampleCount.load() == 0 && g_isDumpStack) {
            g_isReuseStack.store(true);
        }
        if (g_isReuseStack) {
            if (g_isDumpStack) {
                return;
            }
            SaveFreezeStackToFile(outFile, pid);
            return;
        }
        if (g_freezeSampleCount.load() < targetCount) {
            g_isDumpStack.store(true);
            threadSamplerSampleFunc_();
        } else {
            SaveFreezeStackToFile(outFile, pid);
        }
        g_freezeSampleCount.fetch_add(DEFAULT_SAMPLE_VALUE);
    };
    WatchdogTask task(FREEZE_SAMPLE, sampleTask, 0, interval, false);
    if (!InsertWatchdogTaskLocked(FREEZE_SAMPLE, std::move(task))) {
        outFile = "";
    }
}

bool WatchdogInner::CollectStack(std::string& stack, std::string& heaviestStack, int treeFormat)
{
    if (threadSamplerCollectFunc_ == nullptr) {
        return false;
    }
    char* stk = new char[STACK_LENGTH]();
    char* heaviest = new char[STACK_LENGTH]();
    int collectRet = threadSamplerCollectFunc_(stk, heaviest, STACK_LENGTH, STACK_LENGTH, treeFormat);
    if (collectRet != 0) {
        XCOLLIE_LOGE("threadSampler collect stack failed.");
        delete[] stk;
        delete[] heaviest;
        return false;
    }
    stack = stk;
    heaviestStack = heaviest;
    delete[] stk;
    delete[] heaviest;
    return true;
}

bool WatchdogInner::Deinit()
{
    if (threadSamplerDeinitFunc_ == nullptr) {
        return false;
    }
    UninstallThreadSamplerSignal();
    int ret = threadSamplerDeinitFunc_();
    return ret == 0;
}

void WatchdogInner::DumpTraceProfile(int32_t interval)
{
    traceContent_.dumpCount = 0;
    traceContent_.traceCount = 0;
    auto traceTask = [this, interval]() {
        if (traceContent_.traceCount == 0) {
            uint64_t beforeTime = GetCurrentTickMillseconds();
            appCaller_.actionId = UCollectClient::ACTION_ID_START_TRACE;
            auto result = traceCollector_->CaptureDurationTrace(appCaller_);
            XCOLLIE_LOGI("MainThread collect trace result: %{public}d interval: %{public}llu", result.retCode,
                static_cast<unsigned long long>(GetCurrentTickMillseconds() - beforeTime));
            if (result.retCode != 0) {
                isMainThreadTraceEnabled_ = true;
                traceContent_.traceState = DumpStackState::DEFAULT;
                return;
            }
        }
        traceContent_.traceCount++;
        if (CheckEventTimer(GetTimeStamp(), traceContent_.reportBegin,
            traceContent_.reportEnd, interval)) {
            traceContent_.dumpCount++;
        }
        if (traceContent_.traceCount >= COLLECT_TRACE_MAX) {
            if (traceContent_.dumpCount >= COLLECT_TRACE_MIN) {
                CreateDir(WATCHDOG_DIR);
                appCaller_.actionId = UCollectClient::ACTION_ID_DUMP_TRACE;
                appCaller_.isBusinessJank = !buissnessThreadInfo_.empty();
                auto result = traceCollector_->CaptureDurationTrace(appCaller_);
                XCOLLIE_LOGI("MainThread TraceCollector Dump result: %{public}d", result.retCode);
            }
            isMainThreadTraceEnabled_ = true;
        }
    };
    WatchdogTask task("TraceCollector", traceTask, 0, interval, true);
    std::unique_lock<std::mutex> lock(lock_);
    InsertWatchdogTaskLocked("TraceCollector", std::move(task));
}

int32_t WatchdogInner::StartTraceProfile()
{
    traceCollector_ = UCollectClient::TraceCollector::Create();
    if (traceCollector_ == nullptr) {
        traceContent_.traceState = DumpStackState::DEFAULT;
        XCOLLIE_LOGE("Create traceCollector failed.");
        return -1;
    }
    appCaller_.bundleName = bundleName_;
    appCaller_.bundleVersion = bundleVersion_;
    appCaller_.uid = static_cast<int64_t>(getuid());
    appCaller_.pid = getprocpid();
    appCaller_.threadName = GetSelfProcName();
    appCaller_.foreground = isForeground_;
    appCaller_.happenTime = GetTimeStamp() / MILLISEC_TO_NANOSEC;
    appCaller_.beginTime = traceContent_.reportBegin / MILLISEC_TO_NANOSEC;
    appCaller_.endTime = traceContent_.reportEnd / MILLISEC_TO_NANOSEC;
    DumpTraceProfile(DURATION_TIME);
    return 0;
}

void WatchdogInner::CollectTraceDetect(const TimePoint& endTime, int64_t durationTime)
{
    if (IsBetaVersion()) {
        return;
    }
    if (traceContent_.traceState == DumpStackState::COMPLETE) {
        auto diff = endTime - traceContent_.lastEndTime;
        int64_t intervalTime = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
        if (intervalTime < ONE_DAY_LIMIT) {
            return;
        }
    }
    traceContent_.traceState = DumpStackState::COMPLETE;
    UpdateTime(traceContent_.reportBegin, traceContent_.reportEnd, traceContent_.lastEndTime, endTime);
    int32_t result = StartTraceProfile();
    XCOLLIE_LOGI("MainThread TraceCollector Start result: %{public}d, Duration Time: %{public}" PRId64 " ms",
        result, durationTime);
}

static TimePoint DistributeStart(const std::string& name)
{
    WatchdogInner::GetInstance().timeContent_.curBegin = GetTimeStamp();
    return std::chrono::steady_clock::now();
}

static void DistributeEnd(const std::string& name, const TimePoint& startTime)
{
    TimePoint endTime = std::chrono::steady_clock::now();
    auto duration = endTime - startTime;
    int64_t durationTime = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
#ifdef HICOLLIE_JANK_ENABLE
    WatchdogInner::GetInstance().timeContent_.curEnd = GetTimeStamp();
    if (WatchdogInner::GetInstance().CheckSample(endTime, durationTime)) {
        return;
    }
    int sampleInterval = WatchdogInner::GetInstance().jankParamsMap[KEY_SAMPLE_INTERVAL];
    if (duration > std::chrono::milliseconds(sampleInterval)) {
        int logType = WatchdogInner::GetInstance().jankParamsMap[KEY_LOG_TYPE];
        switch (logType) {
            case CatchLogType::LOGTYPE_SAMPLE_STACK: {
                WatchdogInner::GetInstance().StartProfileMainThread(endTime, durationTime, sampleInterval);
                break;
            }
            case CatchLogType::LOGTYPE_COLLECT_TRACE: {
                if (duration > std::chrono::milliseconds(DUMPTRACE_TIME)) {
                    WatchdogInner::GetInstance().CollectTraceDetect(endTime, durationTime);
                }
                break;
            }
            case CatchLogType::LOGTYPE_NONE: {
                if (duration < std::chrono::milliseconds(DUMPTRACE_TIME)) {
                    WatchdogInner::GetInstance().StartProfileMainThread(endTime, durationTime, sampleInterval);
                } else {
                    WatchdogInner::GetInstance().CollectTraceDetect(endTime, durationTime);
                }
                break;
            }
            default:
                break;
        }
    }
#endif // HICOLLIE_JANK_ENABLE
    if (duration > std::chrono::milliseconds(DISTRIBUTE_TIME)) {
        XCOLLIE_LOGI("BlockMonitor event name: %{public}s, Duration Time: %{public}" PRId64 " ms",
            name.c_str(), durationTime);
    }
}

int WatchdogInner::AddThread(const std::string &name,
    std::shared_ptr<AppExecFwk::EventHandler> handler, TimeOutCallback timeOutCallback, uint64_t interval)
{
    if (name.empty() || handler == nullptr) {
        XCOLLIE_LOGE("Add thread fail, invalid args!");
        return -1;
    }

    if (IsInAppspwan()) {
        return -1;
    }

    std::string limitedName = GetLimitedSizeName(name);
    XCOLLIE_LOGI("Add thread %{public}s to watchdog.", limitedName.c_str());
    std::unique_lock<std::mutex> lock(lock_);

    IpcCheck();

    if (!InsertWatchdogTaskLocked(limitedName, WatchdogTask(limitedName, handler, timeOutCallback, interval))) {
        return -1;
    }
    return 0;
}

void WatchdogInner::RunOneShotTask(const std::string& name, Task&& task, uint64_t delay)
{
    if (name.empty() || task == nullptr) {
        XCOLLIE_LOGE("Add task fail, invalid args!");
        return;
    }

    if (IsInAppspwan()) {
        return;
    }

    std::unique_lock<std::mutex> lock(lock_);
    std::string limitedName = GetLimitedSizeName(name);
    InsertWatchdogTaskLocked(limitedName, WatchdogTask(limitedName, std::move(task), delay, 0, true));
}

int64_t WatchdogInner::RunXCollieTask(const std::string& name, uint64_t timeout, XCollieCallback func,
    void *arg, unsigned int flag)
{
    if (name.empty() || timeout == 0) {
        XCOLLIE_LOGE("Add XCollieTask fail, invalid args!");
        return INVALID_ID;
    }

    if (IsInAppspwan()) {
        return INVALID_ID;
    }

    std::unique_lock<std::mutex> lock(lock_);
    IpcCheck();
    std::string limitedName = GetLimitedSizeName(name);
    return InsertWatchdogTaskLocked(limitedName, WatchdogTask(limitedName, timeout, func, arg, flag));
}

void WatchdogInner::RemoveXCollieTask(int64_t id)
{
    std::priority_queue<WatchdogTask> tmpQueue;
    std::unique_lock<std::mutex> lock(lock_);
    size_t size = checkerQueue_.size();
    if (size == 0) {
        XCOLLIE_LOGE("Remove XCollieTask %{public}lld fail, empty queue!", static_cast<long long>(id));
        return;
    }
    while (!checkerQueue_.empty()) {
        const WatchdogTask& task = checkerQueue_.top();
        if (task.id != id || task.timeout == 0) {
            tmpQueue.push(task);
        }
        checkerQueue_.pop();
    }
    if (tmpQueue.size() == size) {
        XCOLLIE_LOGE("Remove XCollieTask fail, can not find timer %{public}lld, size=%{public}zu!",
            static_cast<long long>(id), size);
    }
    tmpQueue.swap(checkerQueue_);
}

void WatchdogInner::RunPeriodicalTask(const std::string& name, Task&& task, uint64_t interval, uint64_t delay)
{
    if (name.empty() || task == nullptr) {
        XCOLLIE_LOGE("Add task fail, invalid args!");
        return;
    }

    if (IsInAppspwan()) {
        return;
    }

    std::string limitedName = GetLimitedSizeName(name);
    XCOLLIE_LOGD("Add periodical task %{public}s to watchdog.", name.c_str());
    std::unique_lock<std::mutex> lock(lock_);
    InsertWatchdogTaskLocked(limitedName, WatchdogTask(limitedName, std::move(task), delay, interval, false));
}

int64_t WatchdogInner::SetTimerCountTask(const std::string &name, uint64_t timeLimit, int countLimit)
{
    if (name.empty() || timeLimit == 0 || countLimit <= 0) {
        XCOLLIE_LOGE("SetTimerCountTask fail, invalid args!");
        return INVALID_ID;
    }

    if (IsInAppspwan()) {
        return INVALID_ID;
    }
    std::string limitedName = GetLimitedSizeName(name);
    XCOLLIE_LOGD("SetTimerCountTask name : %{public}s", name.c_str());
    std::unique_lock<std::mutex> lock(lock_);
    return InsertWatchdogTaskLocked(limitedName, WatchdogTask(limitedName, timeLimit, countLimit));
}

void WatchdogInner::TriggerTimerCountTask(const std::string &name, bool bTrigger, const std::string &message)
{
    std::unique_lock<std::mutex> lock(lock_);

    if (checkerQueue_.empty()) {
        XCOLLIE_LOGE("TriggerTimerCountTask name : %{public}s fail, empty queue!", name.c_str());
        return;
    }

    bool isTaskExist = false;
    uint64_t now = GetCurrentTickMillseconds();
    std::priority_queue<WatchdogTask> tmpQueue;
    while (!checkerQueue_.empty()) {
        WatchdogTask task = checkerQueue_.top();
        if (task.name == name) {
            isTaskExist = true;
            if (bTrigger) {
                task.triggerTimes.push_back(now);
                task.message = message;
            } else {
                task.triggerTimes.clear();
            }
        }
        tmpQueue.push(task);
        checkerQueue_.pop();
    }
    tmpQueue.swap(checkerQueue_);

    if (!isTaskExist) {
        XCOLLIE_LOGE("TriggerTimerCount name : %{public}s does not exist!", name.c_str());
    }
}

bool WatchdogInner::IsTaskExistLocked(const std::string& name)
{
    return (taskNameSet_.find(name) != taskNameSet_.end());
}

bool WatchdogInner::IsExceedMaxTaskLocked()
{
    if (checkerQueue_.size() >= MAX_WATCH_NUM) {
        XCOLLIE_LOGE("Exceed max watchdog task!");
        return true;
    }

    return false;
}

int64_t WatchdogInner::InsertWatchdogTaskLocked(const std::string& name, WatchdogTask&& task)
{
    if (!task.isOneshotTask && IsTaskExistLocked(name)) {
        XCOLLIE_LOGI("Task with %{public}s already exist, failed to insert.", name.c_str());
        return 0;
    }

    if (IsExceedMaxTaskLocked()) {
        XCOLLIE_LOGE("Exceed max watchdog task, failed to insert.");
        return 0;
    }
    int64_t id = task.id;
    checkerQueue_.push(std::move(task));
    if (!task.isOneshotTask) {
        taskNameSet_.insert(name);
    }
    CreateWatchdogThreadIfNeed();
    if (task.nextTickTime < nextWeakUpTime_) {
        condition_.notify_all();
    }

    return id;
}

void WatchdogInner::StopWatchdog()
{
    Stop();
}

bool WatchdogInner::IsCallbackLimit(unsigned int flag)
{
    bool ret = false;
    time_t startTime = time(nullptr);
    if (!(flag & XCOLLIE_FLAG_LOG)) {
        return ret;
    }
    if (timeCallback_ + XCOLLIE_CALLBACK_TIMEWIN_MAX < startTime) {
        timeCallback_ = startTime;
    } else {
        if (++cntCallback_ > XCOLLIE_CALLBACK_HISTORY_MAX) {
            ret = true;
        }
    }
    return ret;
}

void IPCProxyLimitCallback(uint64_t num)
{
    XCOLLIE_LOGE("ipc proxy num %{public}" PRIu64 " exceed limit", num);
    if (getuid() >= MIN_APP_UID && IsBetaVersion()) {
        XCOLLIE_LOGI("Process is going to exit, reason: ipc proxy num exceed limit");
        _exit(0);
    }
}

void WatchdogInner::UpdateAppStartContent(const std::map<std::string, int64_t>& paramsMap,
    AppStartContent& startContent)
{
    auto it = paramsMap.find(KEY_THRESHOLD);
    if (it == paramsMap.end() || it->second <= 0) {
        XCOLLIE_LOGE("Set %{public}s param error, value=%{public}" PRId64".", KEY_THRESHOLD, it->second);
        return;
    }
    startContent.threshold = it->second;

    it = paramsMap.find(KEY_TRIGGER_INTERVAL);
    if (it == paramsMap.end() || it->second <= 0) {
        XCOLLIE_LOGE("Set %{public}s param error, value=%{public}" PRId64".", KEY_TRIGGER_INTERVAL, it->second);
        return;
    }
    startContent.sampleInterval = it->second;

    it = paramsMap.find(KEY_COLLECT_TIMES);
    if (it == paramsMap.end() || it->second <= 0) {
        XCOLLIE_LOGE("Set %{public}s param error, value=%{public}" PRId64".", KEY_COLLECT_TIMES, it->second);
        return;
    }
    startContent.targetCount = it->second;

    it = paramsMap.find(KEY_REPORT_TIMES);
    if (it == paramsMap.end() || it->second <= 0) {
        XCOLLIE_LOGE("Set %{public}s param error, value=%{public}" PRId64".", KEY_REPORT_TIMES, it->second);
        return;
    }
    startContent.reportTimes = it->second;

    it = paramsMap.find(KEY_START_TIME);
    if (it == paramsMap.end() || it->second <= 0) {
        XCOLLIE_LOGE("Set %{public}s param error, value=%{public}" PRId64".", KEY_START_TIME, it->second);
        return;
    }
    startContent.startTime = it->second;

    it = paramsMap.find(KEY_STARTUP_DURATION);
    if (it != paramsMap.end() && it->second > 0) {
        startContent.startUpDuration = it->second;
    }
    startContent.enableStartSample.store(true);
    XCOLLIE_LOGW("UpdateAppStartContent threshold=%{public}" PRId64", sampleInterval=%{public}" PRId64
        ", targetCount=%{public}d, ""reportTimes=%{public}d, startTime=%{public}" PRId64", "
        "enableStartSample=%{public}d, startUpDuration=%{public}" PRId64".",
        startContent.threshold, startContent.sampleInterval, startContent.targetCount, startContent.reportTimes,
        startContent.startTime, startContent.enableStartSample.load(), startContent.startUpDuration);
}

void WatchdogInner::ParseAppStartParams(const std::string& line, const std::string& eventName)
{
    std::map<std::string, int64_t> keyValueMap;
    std::stringstream iss(line);
    std::string tokens;
    while (getline(iss, tokens, ',') && !tokens.empty()) {
        std::string key;
        std::string value;
        if (value.size() > std::to_string(INT64_MAX).length() ||
            !GetKeyValueByStr(tokens, key, value, ':')) {
            XCOLLIE_LOGE("ParseAppStartParams failed, key:%{public}s value:%{public}s",
                key.c_str(), value.c_str());
            continue;
        }
        keyValueMap[key] = static_cast<int64_t>(strtoull(value.c_str(), nullptr, CPU_FREQ_DECIMAL_BASE));
    }
    if (keyValueMap.size() < APP_START_PARAM_SIZE) {
        XCOLLIE_LOGE("ParseAppStartParams eventName:%{public}s keyValueMap size:%{public}zu",
            eventName.c_str(), keyValueMap.size());
        return;
    }
    if (eventName == EVENT_APP_START_SLOW) {
        UpdateAppStartContent(keyValueMap, startSlowContent_);
    } else if (eventName == EVENT_SLIDING_JANK) {
        UpdateAppStartContent(keyValueMap, scrollSlowContent_);
    }
}

void WatchdogInner::ReadAppStartConfig(const std::string& filePath)
{
    if (!OHOS::FileExists(filePath)) {
        XCOLLIE_LOGD("file:%{public}s not exist, errno:%{public}d", filePath.c_str(), errno);
        return;
    }
    std::string str;
    if (!OHOS::LoadStringFromFile(APP_START_CONFIG, str)) {
        XCOLLIE_LOGE("get content from file:%{public}s failed, errno:%{public}d", APP_START_CONFIG, errno);
        return;
    }
    std::stringstream iss(str);
    std::string line;
    std::string eventName;
    while (std::getline(iss, line)) {
        if (line.empty() || line.find(KEY_EVENT_NAME) == std::string::npos) {
            continue;
        }
        if (line.find(EVENT_SLIDING_JANK) != std::string::npos) {
            eventName = EVENT_SLIDING_JANK;
        } else if (line.find(EVENT_APP_START_SLOW) != std::string::npos) {
            eventName = EVENT_APP_START_SLOW;
        } else {
            continue;
        }
        ParseAppStartParams(line, eventName);
    }
}

void WatchdogInner::CreateWatchdogThreadIfNeed()
{
    std::call_once(flag_, [this] {
        if (threadLoop_ == nullptr) {
            if (mainRunner_ == nullptr) {
                mainRunner_ = AppExecFwk::EventRunner::GetMainEventRunner();
            }
            mainRunner_->SetMainLooperWatcher(DistributeStart, DistributeEnd);
            if (getuid() >= MIN_APP_UID) {
                ReadAppStartConfig(APP_START_CONFIG);
            }
            const uint64_t limitNum = 20000;
            IPCDfx::SetIPCProxyLimit(limitNum, IPCProxyLimitCallback);
            threadLoop_ = std::make_unique<std::thread>(&WatchdogInner::Start, this);
            if (getpid() == gettid()) {
                // Enable kernel snapshot, and disable it on DFX unwind success
                DFX_EnableNativeCrashKernelSnapshot();
                SetThreadSignalMask(SIGDUMP, true, true);
            }
            XCOLLIE_LOGD("Watchdog is running!");
        }
    });
}


bool WatchdogInner::IsInSleep(const WatchdogTask& queuedTaskCheck)
{
    if (IsInAppspwan() || queuedTaskCheck.bootTimeStart <= 0 || queuedTaskCheck.monoTimeStart <= 0) {
        return false;
    }

    uint64_t bootTimeStart = 0;
    uint64_t monoTimeStart = 0;
    CalculateTimes(bootTimeStart, monoTimeStart);
    uint64_t bootTimeDetal = GetNumsDiffAbs(bootTimeStart, queuedTaskCheck.bootTimeStart);
    uint64_t monoTimeDetal = GetNumsDiffAbs(monoTimeStart, queuedTaskCheck.monoTimeStart);
    if (GetNumsDiffAbs(bootTimeDetal, monoTimeDetal) >= DEFAULE_SLEEP_TIME) {
        XCOLLIE_LOGI("Current Thread has been sleep, pid: %{public}d", getprocpid());
        return true;
    }
    return false;
}

void WatchdogInner::CheckKickWatchdog(uint64_t now, const WatchdogTask& queuedTask)
{
    if (g_existFile && queuedTask.name == IPC_FULL_TASK && getuid() == FOUNDATION_UID &&
        now - g_lastKickTime > INTERVAL_KICK_TIME) {
        if (KickWatchdog()) {
            g_lastKickTime = now;
        }
    }
}

bool WatchdogInner::CheckCurrentTaskLocked(const WatchdogTask& queuedTaskCheck)
{
    if (queuedTaskCheck.name.empty()) {
        checkerQueue_.pop();
        XCOLLIE_LOGW("queuedTask name is empty.");
    } else if (queuedTaskCheck.name == STACK_CHECKER && isMainThreadStackEnabled_) {
        checkerQueue_.pop();
        taskNameSet_.erase("ThreadSampler");
        if (!g_isDumpStack && !g_isReuseStack && Deinit()) {
            ResetThreadSamplerFuncs();
        }
        stackContent_.isStartSampleEnabled = true;
        isMainThreadStackEnabled_ = false;
        XCOLLIE_LOGI("Detect sample stack task complete.");
    } else if (queuedTaskCheck.name == APP_START_SAMPLE && ((startSlowContent_.isFinishStartSample) ||
        scrollSlowContent_.isFinishStartSample)) {
        checkerQueue_.pop();
        taskNameSet_.erase(APP_START_SAMPLE);
        if (!g_isDumpStack && !g_isReuseStack && Deinit()) {
            ResetThreadSamplerFuncs();
        }
        startSlowContent_.isFinishStartSample = false;
        startSlowContent_.isStartSampleEnabled = true;
        scrollSlowContent_.isFinishStartSample = false;
        scrollSlowContent_.isStartSampleEnabled = true;
        XCOLLIE_LOGI("Detect app start sample task complete.");
    } else if (queuedTaskCheck.name == TRACE_CHECKER && isMainThreadTraceEnabled_) {
        checkerQueue_.pop();
        taskNameSet_.erase("TraceCollector");
        isMainThreadTraceEnabled_ = false;
        if (traceContent_.dumpCount < COLLECT_TRACE_MIN) {
            traceContent_.traceState = DumpStackState::DEFAULT;
        }
        XCOLLIE_LOGI("Detect collect trace task complete.");
    } else if (queuedTaskCheck.name == FREEZE_SAMPLE && g_freezeTaskFinished) {
        checkerQueue_.pop();
        if (!g_isDumpStack && Deinit()) {
            ResetThreadSamplerFuncs();
        }
        taskNameSet_.erase(FREEZE_SAMPLE);
        g_isReuseStack.store(false);
        g_freezeTaskFinished.store(false);
        XCOLLIE_LOGI("Freeze collect stack task complete.");
    } else {
        return false;
    }
    return true;
}

uint64_t WatchdogInner::FetchNextTask(uint64_t now, WatchdogTask& task)
{
    if (isNeedStop_) {
        while (!checkerQueue_.empty()) {
            checkerQueue_.pop();
        }
        return DEFAULT_TIMEOUT;
    }

    if (checkerQueue_.empty()) {
        return DEFAULT_TIMEOUT;
    }

    const WatchdogTask& queuedTaskCheck = checkerQueue_.top();
    if (CheckCurrentTaskLocked(queuedTaskCheck) && checkerQueue_.empty()) {
        return DEFAULT_TIMEOUT;
    }

    const WatchdogTask& queuedTask = checkerQueue_.top();
    CheckKickWatchdog(now, queuedTask);
    if (queuedTask.nextTickTime > now) {
        return queuedTask.nextTickTime - now;
    }

    currentScene_ = "thread DfxWatchdog: Current scenario is task name: " + queuedTask.name + "\n";
    task = queuedTask;
    checkerQueue_.pop();
    return 0;
}

void WatchdogInner::ReInsertTaskIfNeed(WatchdogTask& task)
{
    if (task.checkInterval == 0) {
        return;
    }

    std::unique_lock<std::mutex> lock(lock_);
    task.nextTickTime = task.nextTickTime + task.checkInterval;
    checkerQueue_.push(task);
}

bool WatchdogInner::Start()
{
    if (pthread_setname_np(pthread_self(), "OS_DfxWatchdog") != 0) {
        XCOLLIE_LOGW("Failed to set threadName for watchdog, errno:%d.", errno);
    }
    SetThreadSignalMask(SIGDUMP, false, false);
    watchdogStartTime_ = GetCurrentTickMillseconds();
    XCOLLIE_LOGD("Watchdog is running in thread(%{public}d)!", getproctid());
    if (SetThreadInfoCallback != nullptr) {
        SetThreadInfoCallback(ThreadInfo);
        XCOLLIE_LOGD("Watchdog Set Thread Info Callback");
    }
    while (!isNeedStop_) {
        if (__get_global_hook_flag() && __get_hook_flag()) {
            __set_hook_flag(false);
        }
        uint64_t now = GetCurrentTickMillseconds();
        WatchdogTask task;
        uint64_t leftTimeMill;
        {
            std::unique_lock<std::mutex> lock(lock_);
            leftTimeMill = FetchNextTask(now, task);
            nextWeakUpTime_ = now + leftTimeMill;
        }
        if (leftTimeMill == 0) {
            if (!IsInSleep(task)) {
                task.Run(now);
                ReInsertTaskIfNeed(task);
                currentScene_ = "thread DfxWatchdog: Current scenario is hicollie.\n";
            }
            continue;
        } else if (isNeedStop_) {
            break;
        } else {
            std::unique_lock<std::mutex> lock(lock_);
            condition_.wait_for(lock, std::chrono::milliseconds(leftTimeMill));
        }
    }
    if (SetThreadInfoCallback != nullptr) {
        SetThreadInfoCallback(nullptr);
    }
    return true;
}

bool WatchdogInner::SendMsgToHungtask(const std::string& msg)
{
    if (g_fd == NOT_OPEN) {
        return false;
    }

    ssize_t watchdogWrite = write(g_fd, msg.c_str(), msg.size());
    if (watchdogWrite < 0 || watchdogWrite != static_cast<ssize_t>(msg.size())) {
        XCOLLIE_KLOGE("watchdog write msg failed, errno:%{public}d", errno);
        close(g_fd);
        g_fd = NOT_OPEN;
        return false;
    }
    XCOLLIE_KLOGI("Send %{public}s to hungtask Successful\n", msg.c_str());
    return true;
}

bool WatchdogInner::KickWatchdog()
{
    if (g_fd == NOT_OPEN) {
        g_fd = open(SYS_KERNEL_HUNGTASK_USERLIST, O_WRONLY);
        if (g_fd < 0) {
            g_fd = open(HUNGTASK_USERLIST, O_WRONLY);
            if (g_fd < 0) {
                XCOLLIE_KLOGE("can't open hungtask file, errno:%{public}d", errno);
                g_existFile = false;
                return false;
            }
            XCOLLIE_KLOGE("hmos kernel");
            isHmos = true;
        } else {
            XCOLLIE_KLOGE("linux kernel");
        }

        if (!SendMsgToHungtask(isHmos ? ON_KICK_TIME_EXTRA : ON_KICK_TIME)) {
            XCOLLIE_KLOGI("kick watchdog send msg to hungtask fail");
            return false;
        }
    }
    return SendMsgToHungtask(isHmos ? KICK_TIME_EXTRA : KICK_TIME);
}

bool WatchdogInner::AddIpcFull(uint64_t interval, unsigned int flag, IpcFullCallback func, void *arg)
{
    if (interval < MIN_IPC_CHECK_INTERVAL || interval > MAX_IPC_CHECK_INTERVAL) {
        XCOLLIE_KLOGE("add ipc full failed, interval is invalid");
        return false;
    }
    if (IsInAppspwan()) {
        return false;
    }

    std::unique_lock<std::mutex> lock(lock_);
    return IpcCheck(interval, flag, func, arg, false);
}

bool WatchdogInner::IpcCheck(uint64_t interval, unsigned int flag, IpcFullCallback func, void *arg, bool defaultType)
{
    if (defaultType) {
        static bool ipcCheckInit = false;
        if (ipcCheckInit) {
            return false;
        }
        ipcCheckInit = true;
        uint32_t uid = getuid();
        bool isJoinIpcFullUid = std::any_of(std::begin(JOIN_IPC_FULL_UIDS), std::end(JOIN_IPC_FULL_UIDS),
            [uid](const uint32_t joinIpcFullUid) { return uid == joinIpcFullUid; });
        if (!isJoinIpcFullUid && GetSelfProcName() != KEY_SCB_STATE) {
            return false;
        }
    }

    if (IsTaskExistLocked(IPC_FULL_TASK)) {
        XCOLLIE_LOGE("ipc full task aleady exists");
        return false;
    }

    bool result = InsertWatchdogTaskLocked(IPC_FULL_TASK, WatchdogTask(interval * TO_MILLISECOND_MULTPLE, func, arg,
        flag)) > 0;
    if (result) {
        XCOLLIE_LOGI("add ipc full task success");
    } else {
        XCOLLIE_LOGE("add ipc full task falied");
    }
    return result;
}

bool WatchdogInner::WriteStringToFile(int32_t pid, const char *str)
{
    char file[PATH_LEN] = {0};
    if (snprintf_s(file, PATH_LEN, PATH_LEN - 1, "/proc/%d/unexpected_die_catch", pid) == -1) {
        XCOLLIE_LOGE("failed to build path for %{public}d.", pid);
        return false;
    }
    FILE* fp = fopen(file, "wb");
    if (fp == nullptr) {
        XCOLLIE_LOGI("failed to open file %{public}s, errno: %{public}d", file, errno);
        return false;
    }
    bool writeResult = true;
    if (fwrite(str, sizeof(char), strlen(str), fp) != strlen(str)) {
        XCOLLIE_LOGI("failed to write file %{public}s, errno: %{public}d", file, errno);
        writeResult = false;
    }
    if (fclose(fp)) {
        XCOLLIE_LOGE("fclose is failed");
    }
    fp = nullptr;
    return writeResult;
}

void WatchdogInner::FfrtCallback(uint64_t taskId, const char *taskInfo, uint32_t delayedTaskCount)
{
    std::string faultTimeStr = "\nFault time:" + FormatTime("%Y/%m/%d-%H:%M:%S") + "\n";
    std::string description = "FfrtCallback: task(";
    description += taskInfo;
    description += ") blocked " + std::to_string(FFRT_CALLBACK_TIME / TIME_MS_TO_S) + "s";
    std::string info(taskInfo);
    if (info.find("Queue_Schedule_Timeout") != std::string::npos) {
        WatchdogInner::SendFfrtEvent(description, "SERVICE_WARNING", taskInfo, faultTimeStr, false);
        description += ", report twice instead of exiting process.";
        WatchdogInner::SendFfrtEvent(description, "SERVICE_BLOCK", taskInfo, faultTimeStr);
        WatchdogInner::KillPeerBinderProcess(description);
        return;
    }
    bool isExist = false;
    {
        std::unique_lock<std::mutex> lock(lockFfrt_);
        auto &map = WatchdogInner::GetInstance().taskIdCnt;
        auto search = map.find(taskId);
        if (search != map.end()) {
            isExist = true;
        } else {
            map[taskId] = SERVICE_WARNING;
        }
    }

    if (isExist) {
        description += ", report twice instead of exiting process."; // 1s = 1000ms
        WatchdogInner::SendFfrtEvent(description, "SERVICE_BLOCK", taskInfo, faultTimeStr);
        WatchdogInner::GetInstance().taskIdCnt.erase(taskId);
        WatchdogInner::KillPeerBinderProcess(description);
    } else {
        WatchdogInner::SendFfrtEvent(description, "SERVICE_WARNING", taskInfo, faultTimeStr);
    }
}

void WatchdogInner::InitFfrtWatchdog()
{
    CreateWatchdogThreadIfNeed();
    ffrt_task_timeout_set_cb(FfrtCallback);
    ffrt_task_timeout_set_threshold(FFRT_CALLBACK_TIME);
    std::unique_lock<std::mutex> lock(lock_);
    IpcCheck();
}

void WatchdogInner::SendFfrtEvent(const std::string &msg, const std::string &eventName, const char * taskInfo,
    const std::string& faultTimeStr, const bool isDumpStack)
{
    int32_t pid = getprocpid();
    if (IsProcessDebug(pid)) {
        XCOLLIE_LOGI("heap dump or debug for %{public}d, don't report.", pid);
        return;
    }
    uint32_t gid = getgid();
    uint32_t uid = getuid();
    time_t curTime = time(nullptr);
    std::string sendMsg = std::string((ctime(&curTime) == nullptr) ? "" : ctime(&curTime)) +
        "\n" + msg + "\n";
    char* buffer = new char[FFRT_BUFFER_SIZE + 1]();
    buffer[FFRT_BUFFER_SIZE] = 0;
    ffrt_dump(DUMP_INFO_ALL, buffer, FFRT_BUFFER_SIZE);
    sendMsg += buffer;
    delete[] buffer;
    int32_t tid = pid;
    GetFfrtTaskTid(tid, sendMsg);
    sendMsg += faultTimeStr;
#ifdef HISYSEVENT_ENABLE
    int ret = HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, eventName, HiSysEvent::EventType::FAULT,
        "PID", pid, "TID", tid, "TGID", gid, "UID", uid, "MODULE_NAME", taskInfo, "PROCESS_NAME", GetSelfProcName(),
        "MSG", sendMsg, "STACK", isDumpStack ? GetProcessStacktrace() : "");
    if (ret == ERR_OVER_SIZE) {
        std::string stack = "";
        if (isDumpStack) {
            GetBacktraceStringByTid(stack, tid, 0, true);
        }
        ret = HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, eventName, HiSysEvent::EventType::FAULT,
            "PID", pid, "TID", tid, "TGID", gid, "UID", uid, "MODULE_NAME", taskInfo,
            "PROCESS_NAME", GetSelfProcName(), "MSG", sendMsg, "STACK", stack);
    }

    XCOLLIE_LOGI("hisysevent write result=%{public}d, send event [FRAMEWORK,%{public}s], "
        "msg=%{public}s", ret, eventName.c_str(), msg.c_str());
#else
    XCOLLIE_LOGI("hisysevent not exists");
#endif
}

void WatchdogInner::GetFfrtTaskTid(int32_t& tid, const std::string& msg)
{
    std::string queueNameFrontStr = "us. queue name [";
    size_t queueNameFrontPos = msg.find(queueNameFrontStr);
    if (queueNameFrontPos == std::string::npos) {
        return;
    }
    size_t queueNameRearPos = msg.find("], remaining tasks count=");
    size_t queueStartPos = queueNameFrontPos + queueNameFrontStr.length();
    if (queueNameRearPos == std::string::npos || queueNameRearPos <= queueStartPos) {
        return;
    }
    size_t queueNameLength = queueNameRearPos - queueStartPos;
    std::string workerTidFrontStr = " worker tid ";
    std::string taskIdFrontStr = " is running, task id ";
    std::string queueNameStr = " name " + msg.substr(queueStartPos, queueNameLength);
    std::istringstream issMsg(msg);
    std::string line;
    while (std::getline(issMsg, line, '\n')) {
        size_t workerTidFrontPos = line.find(workerTidFrontStr);
        size_t taskIdFrontPos = line.find(taskIdFrontStr);
        size_t queueNamePos = line.find(queueNameStr);
        size_t workerStartPos = workerTidFrontPos + workerTidFrontStr.length();
        if (workerTidFrontPos == std::string::npos || taskIdFrontPos == std::string::npos ||
            queueNamePos == std::string::npos || taskIdFrontPos <= workerStartPos) {
            continue;
        }
        size_t tidLength = taskIdFrontPos - workerStartPos;
        if (tidLength < std::to_string(INT32_MAX).length()) {
            std::string tidStr = line.substr(workerStartPos, tidLength);
            if (std::all_of(std::begin(tidStr), std::end(tidStr), [] (const char& c) {
                return isdigit(c);
            })) {
                tid = std::stoi(tidStr);
                return;
            }
        }
    }
}

void WatchdogInner::LeftTimeExitProcess(const std::string &description)
{
    int32_t pid = getprocpid();
    if (IsProcessDebug(pid)) {
        XCOLLIE_LOGI("heap dump or debug for %{public}d, don't exit.", pid);
        return;
    }
    DelayBeforeExit(10); // sleep 10s for hiview dump
    bool result = WatchdogInner::WriteStringToFile(pid, "0");
    XCOLLIE_LOGI("Process is going to exit, reason:%{public}s. write to file: %{public}d.",
        description.c_str(), result);

    _exit(0);
}

bool WatchdogInner::Stop()
{
    IPCDfx::SetIPCProxyLimit(0, nullptr);
    if (mainRunner_ != nullptr) {
        mainRunner_->SetMainLooperWatcher(nullptr, nullptr);
    }
    isNeedStop_.store(true);
    condition_.notify_all();
    if (threadLoop_ != nullptr && threadLoop_->joinable()) {
        threadLoop_->join();
        threadLoop_ = nullptr;
    }
    if (g_fd != NOT_OPEN) {
        close(g_fd);
        g_fd = NOT_OPEN;
    }
    return true;
}

void WatchdogInner::KillPeerBinderProcess(const std::string &description)
{
    bool result = false;
    if (getuid() == FOUNDATION_UID) {
        result = KillProcessByPid(getprocpid());
    }
    if (!result) {
        WatchdogInner::LeftTimeExitProcess(description);
    }
}

void WatchdogInner::RemoveInnerTask(const std::string& name)
{
    if (name.empty()) {
        XCOLLIE_LOGI("RemoveInnerTask fail, cname is null");
        return;
    }
    std::priority_queue<WatchdogTask> tmpQueue;
    std::unique_lock<std::mutex> lock(lock_);
    size_t size = checkerQueue_.size();
    if (size == 0) {
        XCOLLIE_LOGE("RemoveInnerTask %{public}s fail, empty queue!", name.c_str());
        return;
    }
    while (!checkerQueue_.empty()) {
        const WatchdogTask& task = checkerQueue_.top();
        if (task.name != name) {
            tmpQueue.push(task);
        } else {
            size_t nameSize = taskNameSet_.size();
            if (nameSize != 0 && !task.isOneshotTask) {
                taskNameSet_.erase(name);
                XCOLLIE_LOGD("RemoveInnerTask name %{public}s, remove result=%{public}d",
                    name.c_str(), nameSize > taskNameSet_.size());
            }
        }
        checkerQueue_.pop();
    }
    if (tmpQueue.size() == size) {
        XCOLLIE_LOGE("RemoveInnerTask fail, can not find name %{public}s, size=%{public}zu!",
            name.c_str(), size);
    }
    tmpQueue.swap(checkerQueue_);
}

void InitBeginFunc(const char* name)
{
    std::string nameStr(name);
    WatchdogInner::GetInstance().bussinessBeginTime_ = DistributeStart(nameStr);
}

void InitEndFunc(const char* name)
{
    std::string nameStr(name);
    DistributeEnd(nameStr, WatchdogInner::GetInstance().bussinessBeginTime_);
}

void WatchdogInner::InitMainLooperWatcher(WatchdogInnerBeginFunc* beginFunc,
    WatchdogInnerEndFunc* endFunc)
{
    int64_t tid = getproctid();
    if (beginFunc && endFunc) {
        if (buissnessThreadInfo_.find(tid) != buissnessThreadInfo_.end()) {
            XCOLLIE_LOGI("Tid =%{public}" PRId64 "already exits, "
                "no repeated initialization.", tid);
            return;
        }
        if (mainRunner_ != nullptr) {
            mainRunner_->SetMainLooperWatcher(nullptr, nullptr);
        }
        *beginFunc = InitBeginFunc;
        *endFunc = InitEndFunc;
        buissnessThreadInfo_.insert(tid);
    } else {
        if (buissnessThreadInfo_.find(tid) != buissnessThreadInfo_.end()) {
            XCOLLIE_LOGI("Remove already init tid=%{public}." PRId64, tid);
            mainRunner_->SetMainLooperWatcher(DistributeStart, DistributeEnd);
            buissnessThreadInfo_.erase(tid);
        }
    }
}

void WatchdogInner::SetAppDebug(bool isAppDebug)
{
    isAppDebug_ = isAppDebug;
}

bool WatchdogInner::GetAppDebug()
{
    return isAppDebug_;
}

void WatchdogInner::UpdateJankParam(int sampleInterval, int ignoreStartUpTime, int sampleCount,
    int logType, int reportTimes)
{
    jankParamsMap[KEY_LOG_TYPE] = logType;
    jankParamsMap[KEY_SAMPLE_INTERVAL] = sampleInterval;
    jankParamsMap[KEY_IGNORE_STARTUP_TIME] = ignoreStartUpTime;
    jankParamsMap[KEY_SAMPLE_COUNT] = sampleCount;
    if (logType == CatchLogType::LOGTYPE_COLLECT_TRACE) {
        XCOLLIE_LOGI("Set thread only dump trace success.");
        return;
    }
    if (jankParamsMap[KEY_SET_TIMES_FLAG] == SET_TIMES_FLAG) {
        UpdateReportTimes(bundleName_, reportTimes, jankParamsMap[KEY_CHECKER_INTERVAL]);
        jankParamsMap[KEY_SAMPLE_REPORT_TIMES] = reportTimes;
        stackContent_.reportTimes = reportTimes;
        jankParamsMap[KEY_SET_TIMES_FLAG] = 0;
    }
    XCOLLIE_LOGI("Set thread sampler params success. logType: %{public}d, sample interval: %{public}d, "
        "ignore startUp interval: %{public}d, count: %{public}d, reportTimes: %{public}d.",
        logType, sampleInterval, ignoreStartUpTime, sampleCount, stackContent_.reportTimes);
}

int WatchdogInner::ConvertStrToNum(std::map<std::string, std::string> paramsMap, const std::string& key)
{
    int num = -1;
    auto it = paramsMap.find(key);
    if (it == paramsMap.end()) {
        XCOLLIE_LOGE("Set the thread sampler param error, %{public}s is not exist.", key.c_str());
        return num;
    }
    std::string str = it->second;
    if (!str.empty() && str.size() < std::to_string(INT32_MAX).length()) {
        if (std::all_of(std::begin(str), std::end(str), [] (const char &c) {
            return isdigit(c);
        })) {
            num = std::stoi(str);
        }
    }
    if (num < 0) {
        XCOLLIE_LOGE("Set param error, %{public}s: %{public}s should be a number, "
            "and greater than 0 and less than INT32_MAX.", key.c_str(), str.c_str());
    }
    return num;
}

bool WatchdogInner::CheckSampleParam(std::map<std::string, std::string> paramsMap)
{
    int sampleInterval = ConvertStrToNum(paramsMap, KEY_SAMPLE_INTERVAL);
    if (sampleInterval < 0) {
        return false;
    } else if (sampleInterval < SAMPLE_INTERVAL_MIN || sampleInterval > SAMPLE_INTERVAL_MAX) {
        XCOLLIE_LOGE("Set the range of sample stack is from %{public}d to %{public}d, "
            "interval: %{public}d.", SAMPLE_INTERVAL_MIN, SAMPLE_INTERVAL_MAX, sampleInterval);
        return false;
    }

    int ignoreStartUpTime = ConvertStrToNum(paramsMap, KEY_IGNORE_STARTUP_TIME);
    if (ignoreStartUpTime < 0) {
        return false;
    } else if (ignoreStartUpTime < IGNORE_STARTUP_TIME_MIN) {
        XCOLLIE_LOGE("Set the minimum of ignore startup interval is %{public}d s, "
            "interval: %{public}d.", IGNORE_STARTUP_TIME_MIN, ignoreStartUpTime);
        return false;
    }

    int sampleCount = ConvertStrToNum(paramsMap, KEY_SAMPLE_COUNT);
    if (sampleCount < 0) {
        return false;
    }
    int maxSampleCount = MAX_SAMPLE_STACK_TIMES / sampleInterval - SAMPLE_EXTRA_COUNT;
    if (sampleCount < SAMPLE_COUNT_MIN || sampleCount > maxSampleCount) {
        XCOLLIE_LOGE("Set the range of sample count, min value: %{public}d max value: %{public}d, count: %{public}d.",
            SAMPLE_COUNT_MIN, maxSampleCount, sampleCount);
        return false;
    }

    int reportTimes = ConvertStrToNum(paramsMap, KEY_SAMPLE_REPORT_TIMES);
    if (reportTimes < 0) {
        return false;
    } else if (reportTimes < SAMPLE_REPORT_TIMES_MIN || reportTimes > SAMPLE_REPORT_TIMES_MAX) {
        XCOLLIE_LOGE("Set the range of sample reportTimes is from %{public}d to %{public}d, "
            "reportTimes: %{public}d", SAMPLE_REPORT_TIMES_MIN, SAMPLE_REPORT_TIMES_MAX, reportTimes);
        return false;
    }
    UpdateJankParam(sampleInterval, ignoreStartUpTime, sampleCount, CatchLogType::LOGTYPE_SAMPLE_STACK, reportTimes);
    return true;
}

int WatchdogInner::SetEventConfig(std::map<std::string, std::string> paramsMap)
{
    if (paramsMap.empty()) {
        XCOLLIE_LOGE("Set the thread sampler param map is null.");
        return -1;
    }
    int logType = ConvertStrToNum(paramsMap, KEY_LOG_TYPE);
    size_t size = paramsMap.size();
    switch (logType) {
        case CatchLogType::LOGTYPE_DEFAULT:
            return -1;
        case CatchLogType::LOGTYPE_NONE:
        case CatchLogType::LOGTYPE_COLLECT_TRACE: {
            if (size != SAMPLE_TRACE_MAP_SIZE) {
                XCOLLIE_LOGE("Set the thread sampler param map size error, can only set log_type. "
                    "map size: %{public}zu", size);
                return -1;
            }
            UpdateJankParam(SAMPLE_DEFULE_INTERVAL, DEFAULT_IGNORE_STARTUP_TIME, SAMPLE_DEFULE_COUNT,
                logType, SAMPLE_REPORT_TIMES_MIN);
            break;
        }
        case CatchLogType::LOGTYPE_SAMPLE_STACK: {
            if (size != SAMPLE_STACK_MAP_SIZE) {
                XCOLLIE_LOGE("Set the thread sampler param map size error, current map size: %{public}zu", size);
                return -1;
            }
            if (!CheckSampleParam(paramsMap)) {
                return -1;
            }
            break;
        }
        default: {
            XCOLLIE_LOGE("Set the log_type can only be 0 1 2, logType: %{public}d", logType);
            return -1;
        }
    };
    return 0;
}

void WatchdogInner::SetSpecifiedProcessName(const std::string& name)
{
    specifiedProcessName_ = name;
}

std::string WatchdogInner::GetSpecifiedProcessName()
{
    return specifiedProcessName_;
}

void WatchdogInner::SetScrollState(bool isScroll)
{
    isScroll_ = isScroll;
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
