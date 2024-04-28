/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef RELIABILITY_WATCHDOG_TASK_H
#define RELIABILITY_WATCHDOG_TASK_H

#include <functional>
#include <string>
#include <sys/types.h>

#include "event_handler.h"
#include "handler_checker.h"

using Task = std::function<void()>;
using TimeOutCallback = std::function<void(const std::string &name, int waitState)>;
using XCollieCallback = std::function<void (void *)>;
namespace OHOS {
namespace HiviewDFX {
class WatchdogTask {
    static int64_t curId;
    static const int countLimitNumMaxRatio = 2;
    static const int timeLimitIntervalRatio = 2;
    static const int uidTypeThreshold = 20000;
public:
    WatchdogTask(std::string name, std::shared_ptr<AppExecFwk::EventHandler> handler,
        TimeOutCallback timeOutCallback, uint64_t interval);
    WatchdogTask(std::string name, Task&& task, uint64_t delay, uint64_t interval, bool isOneshot);
    WatchdogTask(std::string name, unsigned int timeout, XCollieCallback func, void *arg, unsigned int flag);
    WatchdogTask(std::string name, unsigned int timeLimit, int countLimit);
    WatchdogTask()
        : name(""),
          task(nullptr),
          timeOutCallback(nullptr),
          checker(nullptr),
          checkInterval(0),
          nextTickTime(0),
          isOneshotTask(false),
          watchdogTid(0),
          timeLimit(0),
          countLimit(0) {};
    ~WatchdogTask() {};

    bool operator<(const WatchdogTask &obj) const
    {
        // as we use std::priority_queue, the event with smaller target time will be in the top of the queue
        return (this->nextTickTime > obj.nextTickTime);
    }

    void Run(uint64_t now);
    void RunHandlerCheckerTask();
    void SendEvent(const std::string &msg, const std::string &eventName) const;
    void SendXCollieEvent(const std::string &timerName, const std::string &keyMsg) const;
    void DoCallback();
    void TimerCountTask();

    int EvaluateCheckerState();
    std::string GetBlockDescription(uint64_t interval);
    std::string name;
    std::string message;
    Task task;
    TimeOutCallback timeOutCallback;
    std::shared_ptr<HandlerChecker> checker;
    int64_t id;
    uint64_t timeout;
    XCollieCallback func;
    void *arg;
    unsigned int flag;
    uint64_t checkInterval;
    uint64_t nextTickTime;
    bool isTaskScheduled;
    bool isOneshotTask;
    pid_t watchdogTid;
    uint64_t timeLimit;
    int countLimit;
    std::vector<uint64_t> triggerTimes;
};
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif
