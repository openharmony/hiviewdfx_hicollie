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

#include "watchdog_interface_test.h"

#include <gtest/gtest.h>
#include <string>
#include <thread>

#include "event_handler.h"
#include "watchdog.h"

using namespace testing::ext;
using namespace OHOS::AppExecFwk;
namespace OHOS {
namespace HiviewDFX {
class TestEventHandler : public EventHandler {
public:
    explicit TestEventHandler(const std::shared_ptr<EventRunner> &runner)
        : EventHandler(runner) {};
    ~TestEventHandler() {};
    void ProcessEvent(const InnerEvent::Pointer &event)
    {
        count++;
    }
    int count;
};

constexpr int PERIODICAL_TASK_TIME = 2000;
constexpr int TOTAL_WAIT_TIME = 11;
constexpr int EXPECT_RUN_TIMES = 5;
void WatchdogInterfaceTest::SetUpTestCase(void)
{
}

void WatchdogInterfaceTest::TearDownTestCase(void)
{
}

void WatchdogInterfaceTest::SetUp(void)
{
}

void WatchdogInterfaceTest::TearDown(void)
{
}

int g_ret = 0;
void WatchdogInterfaceTest::DoAddWatchThread()
{
    auto runner = EventRunner::Create("test_thread");
    auto handler = std::make_shared<TestEventHandler>(runner);
    g_ret += Watchdog::GetInstance().AddThread("DoAddWatchThread", handler);
    handler.reset();
}

static inline void Sleep(int second)
{
    int left = second;
    while (left > 0) {
        left = sleep(left);
    }
}

HWTEST_F(WatchdogInterfaceTest, WatchdogHandlerCheckerTest_003, TestSize.Level1)
{
    constexpr int BLOCK_TIME = 10;
    constexpr int MAX_THREAD = 5;
    std::vector<std::thread> threads(MAX_THREAD);
    for (int i = 0; i < MAX_THREAD; i++) {
        threads[i] = std::thread(&WatchdogInterfaceTest::DoAddWatchThread, this);
    }

    for (auto& th : threads) {
        th.join();
    }
    ASSERT_EQ(g_ret, -4); // -4 : -1 * 4
    Sleep(BLOCK_TIME);
}

HWTEST_F(WatchdogInterfaceTest, WatchdogHandlerCheckerTest_004, TestSize.Level1)
{
    constexpr int BLOCK_TIME = 10;
    constexpr int CHECK_PERIOD = 2000;
    auto runner = EventRunner::Create("test_thread");
    auto handler = std::make_shared<TestEventHandler>(runner);
    int ret = Watchdog::GetInstance().AddThread("BLOCK2S", handler, CHECK_PERIOD);
    ASSERT_EQ(ret, 0);

    auto taskFunc = []() { Sleep(BLOCK_TIME); };
    Watchdog::GetInstance().RunOneShotTask("block", taskFunc);
    Sleep(BLOCK_TIME);
}

/**
 * @tc.name: Watchdog handler checker with default timeout
 * @tc.desc: Verify default timeout in handler checker interface
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInterfaceTest, WatchdogHandlerCheckerTest_001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. post one task to handler
     * @tc.expected: step1. post task successfully
     */
    constexpr int BLOCK_TIME = 70;
    auto blockFunc = []() {
        printf("before block 70s in %d\n", gettid());
        Sleep(BLOCK_TIME);
        printf("after block 70s in %d\n", gettid());
    };
    auto runner = EventRunner::Create(true);
    auto handler = std::make_shared<TestEventHandler>(runner);
    bool ret = handler->PostTask(blockFunc, "Block70s", 0, EventQueue::Priority::LOW);
    ASSERT_EQ(ret, true);

    /**
     * @tc.steps: step2. add handler to watchdog and check the hisysevent result
     * @tc.expected: step2. add handler to watchdog successfully
     */
    auto timeOutCallback = [](const std::string &name, int waitState) {
        printf("TestBlock70s time out in %d, name is %s, waitState is %d\n", gettid(), name.c_str(), waitState);
    };
    int result = Watchdog::GetInstance().AddThread("TestBlock70s", handler, timeOutCallback);
    ASSERT_EQ(result, 0);

    /**
     * @tc.steps: step3. sleep a while until timeout
     * @tc.expected: step3. SERVICE_BLOCK event has been created and fired
     */
    Sleep(BLOCK_TIME);
}

/**
 * @tc.name: Watchdog handler checker with customized timeout
 * @tc.desc: Verify customized timeout in handler checker interface
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInterfaceTest, WatchdogHandlerCheckerTest_002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. post one task to handler
     * @tc.expected: step1. post task successfully
     */
    constexpr int BLOCK_TIME = 30;
    constexpr int CHECK_PERIOD = 3000;
    auto blockFunc = []() {
        printf("before block 30s in %d\n", gettid());
        Sleep(BLOCK_TIME);
        printf("after block 30s in %d\n", gettid());
    };
    auto runner = EventRunner::Create(true);
    auto handler = std::make_shared<TestEventHandler>(runner);
    bool ret = handler->PostTask(blockFunc, "Block30", 0, EventQueue::Priority::LOW);
    ASSERT_EQ(ret, true);

    /**
     * @tc.steps: step2. add handler to watchdog and check the hisysevent result
     * @tc.expected: step2. add handler to watchdog successfully
     */
    auto timeOutCallback = [](const std::string &name, int waitState) {
        printf("TestBlock20 time out in %d, name is %s, waitState is %d\n", gettid(), name.c_str(), waitState);
    };
    int result = Watchdog::GetInstance().AddThread("TestBlock20", handler, timeOutCallback, CHECK_PERIOD);

    auto timeOutCallback1 = [](const std::string &name, int waitState) {
        printf("TestBlock20_1 time out in %d, name is %s, waitState is %d\n", gettid(), name.c_str(), waitState);
    };
    int result2 = Watchdog::GetInstance().AddThread("TestBlock20_1", handler, timeOutCallback1, CHECK_PERIOD);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(result2, 0);

    /**
     * @tc.steps: step3. sleep a while until timeout
     * @tc.expected: step3. SERVICE_BLOCK event has been created and fired
     */
    Sleep(BLOCK_TIME);
}

/**
 * @tc.name: Watchdog thread run a oneshot task
 * @tc.desc: Verify whether the task has been executed successfully
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInterfaceTest, WatchdogRunTaskTest_001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create 2 oneshot task and add to watchdog
     * @tc.expected: step1. task has been executed
     */
    constexpr int ONESHOT_TASK_TIME = 1;
    constexpr int DELAYED_TASK_TIME = 3;
    constexpr int DELAYED_TASK_TIME_MILLISECOND = 3000;
    int task1Result = 0;
    auto task1Func = [&task1Result]() { task1Result = 1; };
    int task2Result = 0;
    auto task2Func = [&task2Result]() { task2Result = 1; };
    Watchdog::GetInstance().RunOneShotTask("task1", task1Func);
    Watchdog::GetInstance().RunOneShotTask("task2", task2Func);
    Sleep(ONESHOT_TASK_TIME);
    ASSERT_EQ(task1Result, 1);
    ASSERT_EQ(task2Result, 1);

    /**
     * @tc.steps: step2. create a delayed oneshot task and add to watchdog
     * @tc.expected: step2. task has been executed
     */
    int delayedTaskResult = 0;
    auto delayedTaskFunc = [&delayedTaskResult]() { delayedTaskResult = 1; };
    Watchdog::GetInstance().RunOneShotTask("task3", delayedTaskFunc, DELAYED_TASK_TIME_MILLISECOND);
    ASSERT_EQ(delayedTaskResult, 0);
    Sleep(ONESHOT_TASK_TIME + DELAYED_TASK_TIME);
    ASSERT_EQ(delayedTaskResult, 1);
}

/**
 * @tc.name: Watchdog thread run a periodical task
 * @tc.desc: Verify whether the task has been executed successfully
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInterfaceTest, WatchdogRunTaskTest_002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create periodical task and add to watchdog
     * @tc.expected: step1. task has been executed
     */
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult += 1; };
    Watchdog::GetInstance().RunPeriodicalTask("periodicalTask", taskFunc, PERIODICAL_TASK_TIME);
    Sleep(TOTAL_WAIT_TIME);
    ASSERT_GT(taskResult, EXPECT_RUN_TIMES);
}

/**
 * @tc.name: Watchdog thread run a periodical task and stop the watchdog
 * @tc.desc: Verify whether the task has been executed
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInterfaceTest, WatchdogStopTest_001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create periodical task and add to watchdog
     * @tc.expected: step1. task has not been executed
     */
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult += 1; };
    Watchdog::GetInstance().RunPeriodicalTask("periodicalTask2", taskFunc, PERIODICAL_TASK_TIME);
    ASSERT_EQ(taskResult, 0);
    Watchdog::GetInstance().StopWatchdog();
    Sleep(TOTAL_WAIT_TIME);
    ASSERT_EQ(taskResult, 0);
}
} // namespace HiviewDFX
} // namespace OHOS
