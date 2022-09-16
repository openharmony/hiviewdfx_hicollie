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

#ifndef RELIABILITY_WATCHDOG_H
#define RELIABILITY_WATCHDOG_H

#include <string>
#include "event_handler.h"
#include "singleton.h"

using Task = std::function<void()>;
using TimeOutCallback = std::function<void(const std::string &name, int waitState)>;
namespace OHOS {
namespace HiviewDFX {
class Watchdog : public Singleton<Watchdog> {
    DECLARE_SINGLETON(Watchdog);
    static const uint64_t WATCHDOG_TIMEVAL = 30000;
public:

    /**
     * Add handler to watchdog thread with customized check interval
     *
     * @param name, the name of handler check task
     * @param handler, the handler to be checked periodically
     * @param timeOutCallback, callback when timeout
     * @param interval, the period in millisecond
     * @return 0 if added
     *
     */
    int AddThread(const std::string &name, std::shared_ptr<AppExecFwk::EventHandler> handler,
        TimeOutCallback timeOutCallback = nullptr, uint64_t interval = WATCHDOG_TIMEVAL);

    /**
     * Add handler to watchdog thread with customized check interval
     *
     * @param name, the name of handler check task
     * @param handler, the handler to be checked periodically
     * @param interval, the period in millisecond
     * @return 0 if added
     *
     */
    int AddThread(const std::string &name, std::shared_ptr<AppExecFwk::EventHandler> handler,  uint64_t interval);

    /**
     * Run a onshot task in shared watchdog thread, the submitted task should never be time consuming
     *
     * @param name, task name
     * @param task, a short functiona
     * @param delay, delay a few millisecond to run the task
     *
     */
    void RunOneShotTask(const std::string& name, Task&& task, uint64_t delay = 0);

    /**
     * Run a periodical task in shared watchdog thread
     *
     * @param name, task name
     * @param task, a short functiona
     * @param interval, the millisecond interval of the periodical task
     * @param delay, delay a few millisecond to first run the task
     *
     */
    void RunPeriodicalTask(const std::string& name, Task&& task, uint64_t interval, uint64_t delay = 0);

    /**
     * stop watchdog thread and wait for cleanup synchronously
     *
     */
    void StopWatchdog();
};
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif

