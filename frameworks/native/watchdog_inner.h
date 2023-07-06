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
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>

#include "watchdog_task.h"
#include "singleton.h"

namespace OHOS {
namespace HiviewDFX {
class WatchdogInner : public Singleton<WatchdogInner> {
    DECLARE_SINGLETON(WatchdogInner);
public:
    static const int XCOLLIE_CALLBACK_HISTORY_MAX = 5;
    static const int XCOLLIE_CALLBACK_TIMEWIN_MAX = 60;
    int AddThread(const std::string &name, std::shared_ptr<AppExecFwk::EventHandler> handler, uint64_t interval);
    int AddThread(const std::string &name, std::shared_ptr<AppExecFwk::EventHandler> handler,
        TimeOutCallback timeOutCallback, uint64_t interval);
    void RunOneShotTask(const std::string& name, Task&& task, uint64_t delay);
    void RunPeriodicalTask(const std::string& name, Task&& task, uint64_t interval, uint64_t delay);
    int64_t RunXCollieTask(const std::string& name, uint64_t timeout, XCollieCallback func, void *arg,
        unsigned int flag);
    void RemoveXCollieTask(int64_t id);
    void StopWatchdog();
    bool IsCallbackLimit(unsigned int flag);
    std::string currentScene_;

private:
    bool Start();
    bool Stop();
    bool SendMsgToHungtask(const std::string& msg);
    bool KickWatchdog();
    bool IsTaskExistLocked(const std::string& name);
    bool IsExceedMaxTaskLocked();
    int64_t InsertWatchdogTaskLocked(const std::string& name, WatchdogTask&& task);
    uint64_t FetchNextTask(uint64_t now, WatchdogTask& task);
    void ReInsertTaskIfNeed(WatchdogTask& task);
    void CreateWatchdogThreadIfNeed();

    static const unsigned int MAX_WATCH_NUM = 128; // 128: max handler thread
    std::priority_queue<WatchdogTask> checkerQueue_; // protected by lock_
    std::unique_ptr<std::thread> threadLoop_;
    std::mutex lock_;
    std::condition_variable condition_;
    std::atomic_bool isNeedStop_ = false;
    std::once_flag flag_;
    std::set<std::string> taskNameSet_;
    std::shared_ptr<AppExecFwk::EventHandler> binderCheckHander_;
    int cntCallback_;
    time_t timeCallback_;
};
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif
