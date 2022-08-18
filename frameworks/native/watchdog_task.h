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

#include "event_handler.h"
#include "handler_checker.h"

using Task = std::function<void()>;
namespace OHOS {
namespace HiviewDFX {
class WatchdogTask {
public:
    WatchdogTask(std::string name, std::shared_ptr<AppExecFwk::EventHandler> handler, uint64_t interval);
    WatchdogTask(std::string name, Task&& task, uint64_t delay, uint64_t interval);
    WatchdogTask() {};
    ~WatchdogTask() {};
    void Run(uint64_t now);
    void RunHandlerCheckerTask();
    void SendEvent(const std::string &msg) const;

    int EvaluateCheckerState();
    std::string GetBlockDescription(uint64_t interval);
    std::string name;
    Task task;
    std::shared_ptr<HandlerChecker> checker;
    uint64_t checkInterval;
    uint64_t nextTickTime;
    bool isTaskScheduled;
};
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif