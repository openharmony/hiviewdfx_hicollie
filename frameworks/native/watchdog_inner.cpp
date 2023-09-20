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

#include <securec.h>

#include "backtrace_local.h"
#include "hisysevent.h"
#include "xcollie_utils.h"
#include "xcollie_define.h"

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
const int BUF_SIZE_512 = 512;
const char* g_sysKernelHungtaskUserlist = "/sys/kernel/hungtask/userlist";
const std::string ON_KICK_TIME = "on,63";
const std::string KICK_TIME = "kick";
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

    XCOLLIE_LOGI("Add thread %{public}s to watchdog.", name.c_str());
    std::unique_lock<std::mutex> lock(lock_);

    IpcCheck();

    if (!InsertWatchdogTaskLocked(name, WatchdogTask(name, handler, timeOutCallback, interval))) {
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
    InsertWatchdogTaskLocked(name, WatchdogTask(name, std::move(task), delay, 0, true));
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
    return InsertWatchdogTaskLocked(name, WatchdogTask(name, timeout, func, arg, flag));
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
    while (!checkerQueue_.empty())
    {
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

    XCOLLIE_LOGI("Add periodical task %{public}s to watchdog.", name.c_str());
    std::unique_lock<std::mutex> lock(lock_);
    InsertWatchdogTaskLocked(name, WatchdogTask(name, std::move(task), delay, interval, false));
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
    if (pthread_setname_np(pthread_self(), "DfxWatchdog") != 0) {
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
        g_fd = open(g_sysKernelHungtaskUserlist, O_WRONLY);
        if (g_fd < 0) {
            XCOLLIE_LOGE("can't open hungtask file");
            g_existFile = false;
            return false;
        }
    }

    ssize_t watchdogWrite = write(g_fd, msg.c_str(), msg.size());
    if (watchdogWrite < 0 || watchdogWrite != msg.size()) {
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
    if (g_fd == NOT_OPEN) {
        if (!SendMsgToHungtask(ON_KICK_TIME)) {
            XCOLLIE_LOGE("KickWatchdog SendMsgToHungtask false");
            return false;
        }
    }
    return SendMsgToHungtask(KICK_TIME);
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

void WatchdogInner::FfrtCallback(uint64_t taskId, const char *taskInfo, uint32_t delayedTaskCount)
{
    std::string desc = "FfrtCallback: task(";
    desc += taskInfo;
    desc += ") blocked " + std::to_string(FFRT_CALLBACK_TIME / TIME_MS_TO_S) + "s";
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
        desc += ", report twice instead of exiting process."; // 1s = 1000ms
        WatchdogInner::SendFfrtEvent(desc, "SERVICE_BLOCK", taskInfo);
        unsigned int leftTime = 3;
        while (leftTime > 0) {
            leftTime = sleep(leftTime);
        }
        XCOLLIE_LOGI("Process is going to exit, reason:%{public}s.", desc.c_str());
        _exit(0);
    } else {
        WatchdogInner::SendFfrtEvent(desc, "SERVICE_WARNING", taskInfo);
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
    uint32_t pid = getpid();
    uint32_t gid = getgid();
    uint32_t uid = getuid();
    time_t curTime = time(nullptr);
    std::string sendMsg = std::string((ctime(&curTime) == nullptr) ? "" : ctime(&curTime)) +
        "\n" + msg + "\n";
    char buff[BUFF_STACK_SIZE] = {0};
    ffrt_watchdog_dumpinfo(buff, BUFF_STACK_SIZE);
    sendMsg += buff;
    HiSysEventWrite(HiSysEvent::Domain::FRAMEWORK, eventName, HiSysEvent::EventType::FAULT,
        "PID", pid,
        "TGID", gid,
        "UID", uid,
        "MODULE_NAME", taskInfo,
        "PROCESS_NAME", GetSelfProcName(),
        "MSG", sendMsg);
    XCOLLIE_LOGI("send event [FRAMEWORK,%{public}s], msg=%{public}s", eventName.c_str(), msg.c_str());
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
} // end of namespace HiviewDFX
} // end of namespace OHOS
