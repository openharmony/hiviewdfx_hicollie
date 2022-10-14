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

#include <climits>
#include <mutex>

#include <pthread.h>
#include <unistd.h>

#include "hisysevent.h"
#include "xcollie_utils.h"

namespace OHOS {
namespace HiviewDFX {
constexpr uint64_t DEFAULT_TIMEOUT = 60 * 1000;
WatchdogInner::WatchdogInner()
{
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

bool WatchdogInner::InsertWatchdogTaskLocked(const std::string& name, WatchdogTask&& task)
{
    if (!task.isOneshotTask && IsTaskExistLocked(name)) {
        XCOLLIE_LOGI("Task with %{public}s already exist, failed to insert.", name.c_str());
        return false;
    }

    if (IsExceedMaxTaskLocked()) {
        XCOLLIE_LOGE("Exceed max watchdog task, failed to insert.");
        return false;
    }

    checkerQueue_.push(std::move(task));
    if (!task.isOneshotTask) {
        taskNameSet_.insert(name);
    }
    CreateWatchdogThreadIfNeed();
    condition_.notify_all();
    return true;
}

void WatchdogInner::StopWatchdog()
{
    Stop();
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
    if (queuedTask.nextTickTime > now) {
        return queuedTask.nextTickTime - now;
    }

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
    (void)pthread_setname_np(pthread_self(), "DfxWatchdog");
    XCOLLIE_LOGI("Watchdog is running in thread(%{public}d)!", gettid());
    while (!isNeedStop_) {
        uint64_t now = GetCurrentTickMillseconds();
        WatchdogTask task;
        uint64_t leftTimeMill = FetchNextTask(now, task);
        if (leftTimeMill == 0) {
            task.Run(now);
            ReInsertTaskIfNeed(task);
            continue;
        } else if (isNeedStop_) {
            break;
        } else {
            std::unique_lock<std::mutex> lock(lock_);
            condition_.wait_for(lock, std::chrono::milliseconds(leftTimeMill));
        }
    }
    return true;
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
