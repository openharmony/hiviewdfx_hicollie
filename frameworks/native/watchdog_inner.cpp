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
#include <mutex>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <csignal>
#include <string>

#include <securec.h>

#include "backtrace_local.h"
#include "hisysevent.h"
#include "xcollie_utils.h"
#include "xcollie_define.h"
#include "dfx_define.h"
#include "parameter.h"
#include "thread_sampler.h"

typedef void(*ThreadInfoCallBack)(char* buf, size_t len, void* ucontext);
extern "C" void SetThreadInfoCallback(ThreadInfoCallBack func) __attribute__((weak));
namespace OHOS {
namespace HiviewDFX {
constexpr uint64_t DEFAULT_TIMEOUT = 60 * 1000;
constexpr uint32_t FFRT_CALLBACK_TIME = 30 * 1000;
constexpr uint32_t IPC_CHECKER_TIME = 30 * 1000;
constexpr uint32_t TIME_MS_TO_S = 1000;
constexpr int INTERVAL_KICK_TIME = 6 * 1000;
constexpr int32_t WATCHED_UID = 5523;
constexpr int  SERVICE_WARNING = 1;
const char* SYS_KERNEL_HUNGTASK_USERLIST = "/sys/kernel/hungtask/userlist";
const char* HMOS_HUNGTASK_USERLIST = "/proc/sys/hguard/user_list";
const std::string ON_KICK_TIME = "on,72";
const std::string ON_KICK_TIME_HMOS = "on,63,foundation";
const std::string KICK_TIME = "kick";
const std::string KICK_TIME_HMOS = "kick,foundation";
const int32_t NOT_OPEN = -1;
std::mutex WatchdogInner::lockFfrt_;
static uint64_t g_nextKickTime = GetCurrentTickMillseconds();
static int32_t g_fd = -1;
static bool g_existFile = true;
namespace {
void ThreadInfo(char *buf  __attribute__((unused)),
                size_t len  __attribute__((unused)),
                void* ucontext  __attribute__((unused)))
{
    if (ucontext == nullptr) {
        XCOLLIE_LOGI("ThreadInfo ucontext == nullptr");
        return;
    }

    auto ret = memcpy_s(buf, len, WatchdogInner::GetInstance().currentScene_.c_str(),
        WatchdogInner::GetInstance().currentScene_.size());
    if (ret != 0) {
        XCOLLIE_LOGE("memcpy_s ret = %d!", ret);
    }
}
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

bool WatchdogInner::ReportMainThreadEvent()
{
    std::string stack = "";
    CollectStack(stack);
    Deinit();
    std::string path = "";
    if (!WriteStackToFd(getprocpid(), path, stack)) {
        XCOLLIE_LOGI("MainThread WriteStackToFd Failed");
        return false;
    }
    int result = HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, "MAIN_THREAD_JANK",
        HiSysEvent::EventType::FAULT,
        "BUNDLE_VERSION", bundleVersion_,
        "BUNDLE_NAME", bundleName_,
        "BEGIN_TIME", timeContent_.reportBegin / MILLISEC_TO_NANOSEC,
        "END_TIME", timeContent_.reportEnd / MILLISEC_TO_NANOSEC,
        "EXTERNAL_LOG", path,
        "STACK", stack,
        "JANK_LEVEL", 0,
        "THREAD_NAME", GetSelfProcName(),
        "FOREGROUND", isForeground_,
        "LOG_TIME", GetTimeStamp() / MILLISEC_TO_NANOSEC);
    XCOLLIE_LOGI("MainThread HiSysEventWrite result=%{public}d", result);
    return result >= 0;
}

bool WatchdogInner::CheckEventTimer(const int64_t& currentTime)
{
    if (timeContent_.reportBegin == timeContent_.curBegin &&
        timeContent_.reportEnd == timeContent_.curEnd) {
        return false;
    }
    return (timeContent_.curEnd <= timeContent_.curBegin &&
        (currentTime - timeContent_.curBegin >= DURATION_TIME * MILLISEC_TO_NANOSEC)) ||
        (timeContent_.curEnd - timeContent_.curBegin > DURATION_TIME * MILLISEC_TO_NANOSEC);
}

int32_t WatchdogInner::StartProfileMainThread(int32_t interval)
{
    std::unique_lock<std::mutex> lock(lock_);

    if (!ThreadSampler::GetInstance().Init()) {
        return -1;
    }

    sampleTaskState_ = 0;
    stackContent_.detectorCount = 0;
    stackContent_.collectCount = 0;
    auto sampleTask = [this]() {
        if (sampleTaskState_ == DumpStackState::DEFAULT) {
            sampleTaskState_++;
            return;
        }
        int64_t currentTime = GetTimeStamp();
        if (stackContent_.collectCount > DumpStackState::DEFAULT &&
            stackContent_.collectCount < COLLECT_STACK_COUNT) {
            ThreadSampler::GetInstance().Sample();
            stackContent_.collectCount++;
        } else if (stackContent_.collectCount == COLLECT_STACK_COUNT) {
            ReportMainThreadEvent();
            isMainThreadProfileTaskEnabled_ = true;
            return;
        } else {
            if (CheckEventTimer(currentTime)) {
                ThreadSampler::GetInstance().Sample();
                stackContent_.collectCount++;
            } else {
                stackContent_.detectorCount++;
            }
        }
        if (stackContent_.detectorCount == DETECT_STACK_COUNT) {
            isMainThreadProfileTaskEnabled_ = true;
        }
    };

    WatchdogTask task("ThreadSampler", sampleTask, 0, interval, true);
    InsertWatchdogTaskLocked("ThreadSampler", std::move(task));
    return 0;
}

bool WatchdogInner::CollectStack(std::string& stack)
{
    return ThreadSampler::GetInstance().CollectStack(stack, true);
}

void WatchdogInner::Deinit()
{
    return ThreadSampler::GetInstance().Deinit();
}

void WatchdogInner::ChangeState(int& state)
{
    timeContent_.reportBegin = timeContent_.curBegin;
    timeContent_.reportEnd = timeContent_.curEnd;
    state = DumpStackState::COMPLETE;
}

void WatchdogInner::DayChecker(int& state, TimePoint currenTime, TimePoint lastEndTime)
{
    auto diff = currenTime - lastEndTime;
    int64_t intervalTime = std::chrono::duration_cast<std::chrono::milliseconds>
        (diff).count();
    if (intervalTime >= ONE_DAY_LIMIT) {
        XCOLLIE_LOGI("MainThread StartProfileMainThread Over Oneday");
        state = DumpStackState::DEFAULT;
    }
}

void WatchdogInner::StartTraceProfile(int32_t interval)
{
    if (traceCollector_ == nullptr) {
        XCOLLIE_LOGI("MainThread TraceCollector Failed.");
        return;
    }
    traceContent_.dumpCount = 0;
    traceContent_.traceCount = 0;
    auto traceTask = [this]() {
        traceContent_.traceCount++;
        int64_t currentTime = GetTimeStamp();
        if (CheckEventTimer(currentTime)) {
            traceContent_.dumpCount++;
        }
        if (traceContent_.traceCount >= COLLECT_TRACE_MAX) {
            if (traceContent_.dumpCount >= COLLECT_TRACE_MIN) {
                CreateWatchdogDir();
                appCaller.actionId = UCollectClient::ACTION_ID_DUMP_TRACE;
                auto result = traceCollector_->CaptureDurationTrace(appCaller);
                XCOLLIE_LOGI("MainThread TraceCollector Dump result: %{public}d", result.retCode);
            }
            isMainThreadTraceEnabled_ = true;
        }
    };
    WatchdogTask task("TraceCollector", traceTask, 0, interval, true);
    InsertWatchdogTaskLocked("TraceCollector", std::move(task));
}

void WatchdogInner::CollectTrace()
{
    traceCollector_ = UCollectClient::TraceCollector::Create();
    int32_t pid = getprocpid();
    int32_t uid = static_cast<int64_t>(getuid());
    appCaller.actionId = UCollectClient::ACTION_ID_START_TRACE;
    appCaller.bundleName = bundleName_;
    appCaller.bundleVersion = bundleVersion_;
    appCaller.uid = uid;
    appCaller.pid = pid;
    appCaller.threadName = GetSelfProcName();
    appCaller.foreground = isForeground_;
    appCaller.happenTime = GetTimeStamp() / MILLISEC_TO_NANOSEC;
    appCaller.beginTime = timeContent_.reportBegin / MILLISEC_TO_NANOSEC;
    appCaller.endTime = timeContent_.reportEnd / MILLISEC_TO_NANOSEC;
    auto result = traceCollector_->CaptureDurationTrace(appCaller);
    XCOLLIE_LOGI("MainThread TraceCollector Start result: %{public}d", result.retCode);
    if (result.retCode != 0) {
        return;
    }
    StartTraceProfile(DURATION_TIME);
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
    int64_t durationTime = std::chrono::duration_cast<std::chrono::milliseconds>
        (duration).count();
    if (duration > std::chrono::milliseconds(DISTRIBUTE_TIME)) {
        XCOLLIE_LOGI("BlockMonitor event name: %{public}s, Duration Time: %{public}" PRId64 " ms",
            name.c_str(), durationTime);
    }
    if (!IsCommercialVersion()) {
        return;
    }
    WatchdogInner::GetInstance().timeContent_.curEnd = GetTimeStamp();
    if (WatchdogInner::GetInstance().stackContent_.stackState == DumpStackState::COMPLETE) {
        WatchdogInner::GetInstance().DayChecker(WatchdogInner::GetInstance().stackContent_.stackState,
            endTime, WatchdogInner::GetInstance().stackContent_.lastStackTime);
    }
    if (WatchdogInner::GetInstance().traceContent_.traceState == DumpStackState::COMPLETE) {
        WatchdogInner::GetInstance().DayChecker(WatchdogInner::GetInstance().traceContent_.traceState,
            endTime, WatchdogInner::GetInstance().traceContent_.lastTraceTime);
    }
    if (duration > std::chrono::milliseconds(DURATION_TIME) && duration < std::chrono::milliseconds(DUMPTRACE_TIME) &&
        WatchdogInner::GetInstance().stackContent_.stackState == DumpStackState::DEFAULT) {
        WatchdogInner::GetInstance().ChangeState(WatchdogInner::GetInstance().stackContent_.stackState);
        WatchdogInner::GetInstance().stackContent_.lastStackTime = endTime;

        int32_t ret = WatchdogInner::GetInstance().StartProfileMainThread(TASK_INTERVAL);
        XCOLLIE_LOGI("MainThread StartProfileMainThread ret: %{public}d  "
            "Duration Time: %{public}" PRId64 " ms", ret, durationTime);
    } else if (duration > std::chrono::milliseconds(DUMPTRACE_TIME) &&
        WatchdogInner::GetInstance().traceContent_.traceState == DumpStackState::DEFAULT) {
        XCOLLIE_LOGI("MainThread TraceCollector Duration Time: %{public}" PRId64 " ms", durationTime);
        WatchdogInner::GetInstance().ChangeState(WatchdogInner::GetInstance().traceContent_.traceState);
        WatchdogInner::GetInstance().traceContent_.lastTraceTime = endTime;
        WatchdogInner::GetInstance().CollectTrace();
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
        if (task.id != id) {
            tmpQueue.push(task);
        }
        checkerQueue_.pop();
    }
    if (tmpQueue.size() == size) {
        XCOLLIE_LOGE("Remove XCollieTask fail, can not find timer %{public}lld, size=%{public}zu!",
            static_cast<long long>(id), size);
        return;
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
    XCOLLIE_LOGI("Add periodical task %{public}s to watchdog.", name.c_str());
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
    XCOLLIE_LOGI("SetTimerCountTask name : %{public}s", name.c_str());
    return InsertWatchdogTaskLocked(limitedName, WatchdogTask(limitedName, timeLimit, countLimit));
}

void WatchdogInner::TriggerTimerCountTask(const std::string &name, bool bTrigger, const std::string &message)
{
    std::unique_lock<std::mutex> lock(lock_);

    if (!checkerQueue_.size()) {
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
    if (taskNameSet_.find(name) != taskNameSet_.end()) {
        return true;
    }

    return false;
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
    condition_.notify_all();

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

void WatchdogInner::CreateWatchdogThreadIfNeed()
{
    std::call_once(flag_, [this] {
        if (threadLoop_ == nullptr) {
            if (mainRunner_ == nullptr) {
                mainRunner_ = AppExecFwk::EventRunner::GetMainEventRunner();
            }
            mainRunner_->SetMainLooperWatcher(DistributeStart, DistributeEnd);
            threadLoop_ = std::make_unique<std::thread>(&WatchdogInner::Start, this);
            XCOLLIE_LOGI("Watchdog is running!");
        }
    });
}

uint64_t WatchdogInner::FetchNextTask(uint64_t now, WatchdogTask& task)
{
    std::unique_lock<std::mutex> lock(lock_);
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
    bool popCheck = true;
    if (queuedTaskCheck.name.empty()) {
        checkerQueue_.pop();
        XCOLLIE_LOGW("queuedTask name is empty.");
    } else if (queuedTaskCheck.name == STACK_CHECKER && isMainThreadProfileTaskEnabled_) {
        checkerQueue_.pop();
        taskNameSet_.erase("ThreadSampler");
        isMainThreadProfileTaskEnabled_ = false;
        XCOLLIE_LOGI("STACK_CHECKER Task pop");
    } else if (queuedTaskCheck.name == TRACE_CHECKER && isMainThreadTraceEnabled_) {
        checkerQueue_.pop();
        taskNameSet_.erase("TraceCollector");
        isMainThreadTraceEnabled_ = false;
        XCOLLIE_LOGI("TRACE_CHECKER Task pop");
    } else {
        popCheck = false;
    }
    if (popCheck && checkerQueue_.empty()) {
        return DEFAULT_TIMEOUT;
    }

    const WatchdogTask& queuedTask = checkerQueue_.top();
    if (g_existFile && queuedTask.name == IPC_FULL && now - g_nextKickTime > INTERVAL_KICK_TIME) {
        if (KickWatchdog()) {
            g_nextKickTime = now;
        }
    }
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

    XCOLLIE_LOGI("Watchdog is running in thread(%{public}d)!", gettid());
    if (SetThreadInfoCallback != nullptr) {
        SetThreadInfoCallback(ThreadInfo);
        XCOLLIE_LOGI("Watchdog Set Thread Info Callback");
    }
    while (!isNeedStop_) {
        uint64_t now = GetCurrentTickMillseconds();
        WatchdogTask task;
        uint64_t leftTimeMill = FetchNextTask(now, task);
        if (leftTimeMill == 0) {
            task.Run(now);
            ReInsertTaskIfNeed(task);
            currentScene_ = "thread DfxWatchdog: Current scenario is hicollie.\n";
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
        g_fd = open(SYS_KERNEL_HUNGTASK_USERLIST, O_WRONLY);
        if (g_fd < 0) {
            g_fd = open(HMOS_HUNGTASK_USERLIST, O_WRONLY);
            if (g_fd < 0) {
                XCOLLIE_LOGE("can't open hungtask file");
                g_existFile = false;
                return false;
            }
            XCOLLIE_LOGE("change to hmos kernel");
            isHmos = true;
        } else {
            XCOLLIE_LOGE("change to linux kernel");
        }
    }

    ssize_t watchdogWrite = write(g_fd, msg.c_str(), msg.size());
    if (watchdogWrite < 0 || watchdogWrite != static_cast<ssize_t>(msg.size())) {
        XCOLLIE_LOGE("watchdogWrite msg failed");
        close(g_fd);
        g_fd = -1;
        return false;
    }
    XCOLLIE_LOGE("Send %{public}s to hungtask Successful\n", msg.c_str());
    return true;
}

bool WatchdogInner::KickWatchdog()
{
    return true;
}

void WatchdogInner::IpcCheck()
{
    if (getuid() == WATCHED_UID) {
        if (binderCheckHander_ == nullptr) {
            auto runner = AppExecFwk::EventRunner::Create(IPC_CHECKER);
            binderCheckHander_ = std::make_shared<AppExecFwk::EventHandler>(runner);
            if (!InsertWatchdogTaskLocked(IPC_CHECKER, WatchdogTask(IPC_FULL, binderCheckHander_,
                nullptr, IPC_CHECKER_TIME))) {
                XCOLLIE_LOGE("Add %{public}s thread fail", IPC_CHECKER);
            }
        }
    }
}

void WatchdogInner::WriteStringToFile(uint32_t pid, const char *str)
{
    char file[PATH_LEN] = {0};
    if (snprintf_s(file, PATH_LEN, PATH_LEN - 1, "/proc/%d/unexpected_die_catch", pid) == -1) {
        XCOLLIE_LOGI("failed to build path for %d.", pid);
    }
    int fd = open(file, O_RDWR);
    if (fd == -1) {
        return;
    }
    if (write(fd, str, strlen(str)) < 0) {
        XCOLLIE_LOGI("failed to write 0 for %s", file);
        close(fd);
        return;
    }
    close(fd);
    return;
}

void WatchdogInner::FfrtCallback(uint64_t taskId, const char *taskInfo, uint32_t delayedTaskCount)
{
    std::string description = "FfrtCallback: task(";
    description += taskInfo;
    description += ") blocked " + std::to_string(FFRT_CALLBACK_TIME / TIME_MS_TO_S) + "s";
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
        WatchdogInner::SendFfrtEvent(description, "SERVICE_BLOCK", taskInfo);
        WatchdogInner::KillPeerBinderProcess(description);
    } else {
        WatchdogInner::SendFfrtEvent(description, "SERVICE_WARNING", taskInfo);
    }
}

void WatchdogInner::InitFfrtWatchdog()
{
    CreateWatchdogThreadIfNeed();
    IpcCheck();
    ffrt_watchdog_register(FfrtCallback, FFRT_CALLBACK_TIME, FFRT_CALLBACK_TIME);
}

void WatchdogInner::SendFfrtEvent(const std::string &msg, const std::string &eventName, const char * taskInfo)
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
    ffrt_watchdog_dumpinfo(buffer, FFRT_BUFFER_SIZE);
    sendMsg += buffer;
    sendMsg += "\n" + GetProcessStacktrace();
    delete[] buffer;
    int ret = HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, eventName, HiSysEvent::EventType::FAULT,
        "PID", pid,
        "TGID", gid,
        "UID", uid,
        "MODULE_NAME", taskInfo,
        "PROCESS_NAME", GetSelfProcName(),
        "MSG", sendMsg);
    XCOLLIE_LOGI("hisysevent write result=%{public}d, send event [FRAMEWORK,%{public}s], "
        "msg=%{public}s", ret, eventName.c_str(), msg.c_str());
}

void WatchdogInner::LeftTimeExitProcess(const std::string &description)
{
    int32_t pid = getprocpid();
    if (IsProcessDebug(pid)) {
        XCOLLIE_LOGI("heap dump or debug for %{public}d, don't exit.", pid);
        return;
    }
    DelayBeforeExit(10); // sleep 10s for hiview dump
    XCOLLIE_LOGI("Process is going to exit, reason:%{public}s.", description.c_str());
    WatchdogInner::WriteStringToFile(pid, "0");

    _exit(0);
}

bool WatchdogInner::Stop()
{
    isNeedStop_.store(true);
    condition_.notify_all();
    if (threadLoop_ != nullptr && threadLoop_->joinable()) {
        threadLoop_->join();
        threadLoop_ = nullptr;
    }
    return true;
}

void WatchdogInner::KillPeerBinderProcess(const std::string &description)
{
    bool result = false;
    if (getuid() == WATCHED_UID) {
        result = KillProcessByPid(getprocpid());
    }
    if (!result) {
        WatchdogInner::LeftTimeExitProcess(description);
    }
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
