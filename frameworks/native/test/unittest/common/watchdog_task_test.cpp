/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "watchdog_task_test.h"

#include <gtest/gtest.h>
#include <string>
#include <thread>

#include "watchdog_task.h"
#include "xcollie_utils.h"
#include "directory_ex.h"
#include "file_ex.h"
#include "event_handler.h"
#include "ffrt_inner.h"

using namespace testing::ext;
using namespace OHOS::AppExecFwk;
namespace OHOS {
namespace HiviewDFX {
void WatchdogTaskTest::SetUpTestCase(void)
{
}

void WatchdogTaskTest::TearDownTestCase(void)
{
}

void WatchdogTaskTest::SetUp(void)
{
}

void WatchdogTaskTest::TearDown(void)
{
}

/**
 * @tc.name: WatchdogTaskTest_001
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTaskTest_001, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogTask task("WatchdogTaskTest_001", taskFunc, 0, 0, true);
    task.DoCallback();
    EXPECT_EQ(task.flag, 0);
    task.flag = 1;
    task.DoCallback();
    task.flag = 2;
    task.DoCallback();
}

/**
 * @tc.name: WatchdogTaskTest_002
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTaskTest_002, TestSize.Level1)
{
    uint64_t now = GetCurrentTickMillseconds() + 1000;
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogTask task("WatchdogTaskTest_002", taskFunc, 0, 0, true);
    task.Run(now);
    EXPECT_EQ(task.GetBlockDescription(1), "Watchdog: thread(WatchdogTaskTest_002) blocked 1s");
}

/**
 * @tc.name: WatchdogTaskTest_003
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTaskTest_003, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogTask task("WatchdogTaskTest_003", taskFunc, 0, 0, true);
    task.RunHandlerCheckerTask();
    EXPECT_TRUE(task.checker == nullptr);

    auto runner = EventRunner::Create(true);
    auto handler = std::make_shared<EventHandler>(runner);
    WatchdogTask task1("WatchdogTaskTest_003", handler, nullptr, 5);
    task1.RunHandlerCheckerTask();
    EXPECT_TRUE(task1.checker != nullptr);
    EXPECT_TRUE(task1.checker->GetCheckState() >= 0);
}

/**
 * @tc.name: WatchdogTaskTest_004
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTaskTest_004, TestSize.Level1)
{
    WatchdogTask task("WatchdogTaskTest_004", 0, 1);
    uint64_t now = GetCurrentTickMillseconds();
    task.triggerTimes.push_back(now);
    task.triggerTimes.push_back(now + 1000);
    task.triggerTimes.push_back(now + 2000);
    task.TimerCountTask();
    EXPECT_TRUE(task.countLimit != 0);
}

/**
 * @tc.name: WatchdogTaskTest_005
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTaskTest_005, TestSize.Level1)
{
    IpcFullCallback callback = [] (void* arg) {
        printf("WatchdogTaskTest_005 test ipc full");
    };

    WatchdogTask task(10, callback, nullptr, HiviewDFX::XCOLLIE_FLAG_DEFAULT);
    task.RunHandlerCheckerTask();
    EXPECT_TRUE(task.checker != nullptr);
    EXPECT_TRUE(task.checker->GetCheckState() >= 0);
}

#ifdef SUSPEND_CHECK_ENABLE
/**
 * @tc.name: WatchdogTask_GetSuspendTimeTask_001
 * @tc.desc: verify wherther the time is read successfully from the file
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_GetSuspendTimeTask_001, TestSize.Level1)
{
    uint64_t now = GetCurrentTickMillseconds();
    const char *LAST_SUSPEND_TIME_PATH = "/sys/power/last_sr";
    auto [lastSuspendStartTime, lastSuspendEndTime] = GetSuspendTime(LAST_SUSPEND_TIME_PATH, now);
    ASSERT_GT(lastSuspendStartTime, -1);
    ASSERT_GT(lastSuspendEndTime, -1);
}

/**
 * @tc.name: WatchdogTask_WatchdogSkipCheckWhenSuspend_001
 * @tc.desc: verify wherther the task skipped successfully
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_WatchdogSkipCheckWhenSuspend_001, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    const std::string name = "SkipCheckWhenSuspend_001";
    uint64_t delay = 0;
    uint64_t interval = 5000;
    bool isOneshot = true;
    WatchdogTask task(name, taskFunc, delay, interval, isOneshot);
    double testLastSuspendStartTime = 1500.238412;
    double testLastSuspendEndTime = 900.391023;
    uint64_t testNow = 1600;
    ASSERT_EQ(task.ShouldSkipCheckForSuspend(testNow, testLastSuspendStartTime, testLastSuspendEndTime), true);
}

/**
 * @tc.name: WatchdogTask_WatchdogSkipCheckWhenSuspend_002
 * @tc.desc: verify wherther the task skipped successfully
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_WatchdogSkipCheckWhenSuspend_002, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    const std::string name = "SkipCheckWhenSuspend_002";
    uint64_t delay = 0;
    uint64_t interval = 10000;
    bool isOneshot = true;
    WatchdogTask task(name, taskFunc, delay, interval, isOneshot);
    double testLastSuspendStartTime = 9000.391023;
    double testLastSuspendEndTime = 15000.238412;
    uint64_t testNow = 16000;
    ASSERT_EQ(task.ShouldSkipCheckForSuspend(testNow, testLastSuspendStartTime, testLastSuspendEndTime), true);
}

/**
 * @tc.name: WatchdogTask_WatchdogSkipCheckWhenSuspend_003
 * @tc.desc: verify wherther the task skipped successfully
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_WatchdogSkipCheckWhenSuspend_003, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    const std::string name = "SkipCheckWhenSuspend_003";
    uint64_t delay = 0;
    uint64_t interval = 3000;
    bool isOneshot = true;
    WatchdogTask task(name, taskFunc, delay, interval, isOneshot);
    double testLastSuspendStartTime = 9000.391023;
    double testLastSuspendEndTime = 15000.238412;
    uint64_t testNow = 16000;
    ASSERT_EQ(task.ShouldSkipCheckForSuspend(testNow, testLastSuspendStartTime, testLastSuspendEndTime), false);
}

/**
 * @tc.name: WatchdogTask_WatchdogSkipCheckWhenSuspend_004
 * @tc.desc: verify wherther the task skipped successfully
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_WatchdogSkipCheckWhenSuspend_004, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    const std::string name = "SkipCheckWhenSuspend_004";
    uint64_t delay = 0;
    uint64_t interval = 3000;
    bool isOneshot = true;
    WatchdogTask task(name, taskFunc, delay, interval, isOneshot);
    double testLastSuspendStartTime = -1;
    double testLastSuspendEndTime = 15000.238412;
    uint64_t testNow = 16000;
    ASSERT_EQ(task.ShouldSkipCheckForSuspend(testNow, testLastSuspendStartTime, testLastSuspendEndTime), false);
}

/**
 * @tc.name: WatchdogTask_WatchdogSkipCheckWhenSuspend_005
 * @tc.desc: verify wherther the task skipped successfully
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_WatchdogSkipCheckWhenSuspend_005, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    const std::string name = "SkipCheckWhenSuspend_005";
    uint64_t delay = 0;
    uint64_t interval = 3000;
    bool isOneshot = true;
    WatchdogTask task(name, taskFunc, delay, interval, isOneshot);
    double testLastSuspendStartTime = -1;
    double testLastSuspendEndTime = -1;
    uint64_t testNow = -1;
    ASSERT_EQ(task.ShouldSkipCheckForSuspend(testNow, testLastSuspendStartTime, testLastSuspendEndTime), false);
}

#endif

/**
 * @tc.name: WatchdogTaskTest SendXCollieEvent
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTaskTest_SendXCollieEvent_001, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    const std::string name = "WatchdogTaskTest_SendXCollieEvent_001";
    WatchdogTask task(name, taskFunc, 0, 1000, true);
    task.SendXCollieEvent("1234", "keyMsg", "1234567");
    EXPECT_TRUE(!name.empty());
}

/**
 * @tc.name: WatchdogTaskTest SendEvent
 * @tc.desc: add testcase code coverage
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTaskTest_SendEvent_001, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    const std::string name = "WatchdogTaskTest_SendEvent_001";
    WatchdogTask task(name, taskFunc, 0, 2111, true);
    task.SendEvent("11", "keyMsg", "11111");
    EXPECT_TRUE(!name.empty());
}
} // namespace HiviewDFX
} // namespace OHOS
