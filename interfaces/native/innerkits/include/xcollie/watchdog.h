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

#include <map>
#include <string>
#include "singleton.h"

using Task = std::function<void()>;
using TimeOutCallback = std::function<void(const std::string &name, int waitState)>;

typedef void (*WatchdogBeginFunc)(const char* eventName);
typedef void (*WatchdogEndFunc)(const char* eventName);

namespace OHOS {
namespace AppExecFwk {
    class EventHandler;
}

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

    /**
     * watchdog adapt ffrt and not influence ipc full, hungtask delete watchdog, merge xcollie and watchdog.
     *
     */
    void InitFfrtWatchdog();

    /**
     *
     * @brief Set bundle info.
     *
     */
    void SetBundleInfo(const std::string& bundleName, const std::string& bundleVersion);

    /**
     *
     * @brief Set system app.
     *
     */
    void SetSystemApp(bool isSystemApp);

    /**
     *
     * @brief Set foreground.
     *
     */
    void SetForeground(const bool& isForeground);

    /**
     *
     * @brief Get foreground.
     *
     */
    bool GetForeground();

    /**
     * @brief Remove a periodical task by name.
     *
     * @param name, task name
     */
    void RemovePeriodicalTask(const std::string& name);

    /**
     * @brief Remove the thread by name.
     *
     * @param name: task name
     */
    void RemoveThread(const std::string& name);

    /**
     * @brief Init MainLooperWatcher in bussiness watchdog thread.
     */
    void InitMainLooperWatcher(WatchdogBeginFunc* beginFunc, WatchdogEndFunc* endFunc);

    /**
     * @brief Set isAppDebug.
     */
    void SetAppDebug(bool isAppDebug);

    /**
     * @brief Get isAppDebug.
     */
    bool GetAppDebug();

    /**
     * @brief Set sample jank params.
     */
    int SetEventConfig(std::map<std::string, std::string> paramsMap);

    /**
     * @brief Set specified process name.
     */
    void SetSpecifiedProcessName(const std::string& name);

    /**
     * @brief Set scroll param.
     */
    void SetScrollState(bool isScroll);

    /**
     * @brief Start freeze stack sample.
     */
    void StartSample(int duration, int interval);

    /**
     * @brief Stop freeze stack sample.
     */
    std::string StopSample(int sampleCount);

    /**
     * @brief Get sampler result.
     */
    void GetSamplerResult(uint64_t &samplerStartTime, uint64_t &samplerFinishTime, int32_t &samplerCount);
};
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif

