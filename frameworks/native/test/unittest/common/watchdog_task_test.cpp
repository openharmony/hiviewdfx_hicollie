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
    WatchdogTask task1("WatchdogTaskTest_003", handler, nullptr, 5,
        AppExecFwk::EventQueue::Priority::IMMEDIATE);
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

    WatchdogTask task(10, 0, callback, nullptr, HiviewDFX::XCOLLIE_FLAG_DEFAULT);
    task.RunHandlerCheckerTask();
    int pid = getpid();
    std::string msg = "Thread ID = " + std::to_string(pid) + ") is running";
    task.SendEvent(msg, "IPC_FULL", "");
    sleep(1);
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
    std::string name = "WatchdogTaskTest_SendXCollieEvent_001";
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
    std::string name = "WatchdogTaskTest_SendEvent_001";
    auto runner = EventRunner::Create(true);
    auto handler = std::make_shared<EventHandler>(runner);
    WatchdogTask task1(name, handler, nullptr, 5,
        AppExecFwk::EventQueue::Priority::IMMEDIATE);
    task1.RunHandlerCheckerTask();
    EXPECT_TRUE(task1.checker != nullptr);
    task1.SendEvent("11", "keyMsg", "11111");
    EXPECT_TRUE(!name.empty());
}

/**
 * @tc.name: WatchdogTaskTest ParseTidFromMsg
 * @tc.desc: test ParseTidFromMsg function
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTaskTest_ParseTidFromMsg_001, TestSize.Level1)
{
    std::string name = "WatchdogTaskTest_ParseTidFromMsg_001";
    auto runner = EventRunner::Create(true);
    auto handler = std::make_shared<EventHandler>(runner);
    WatchdogTask task(name, handler, nullptr, 5, AppExecFwk::EventQueue::Priority::IMMEDIATE);
    task.SendEvent("test msg with Thread ID = 12345) is running", "SERVICE_WARNING", "");
    EXPECT_TRUE(!name.empty());
}
 
/**
 * @tc.name: WatchdogTaskTest InsertSampleStackTask
 * @tc.desc: test InsertSampleStackTask function
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTaskTest_InsertSampleStackTask_001, TestSize.Level1)
{
    std::string name = "WatchdogTaskTest_InsertSampleStackTask_001";
    auto runner = EventRunner::Create(true);
    auto handler = std::make_shared<EventHandler>(runner);
    WatchdogTask task(name, handler, nullptr, 5, AppExecFwk::EventQueue::Priority::IMMEDIATE);
    task.sampleStack = "";
    task.SendEvent("test msg with Thread ID = 12345) is running", "SERVICE_BLOCK", "");
    EXPECT_TRUE(!name.empty());
}

#ifdef LOW_MEMORY_FREEZE_STRATEGY_ENABLE
/**
 * @tc.name: WatchdogTask_ShouldCheckLowMemory_001
 * @tc.desc: verify ShouldCheckLowMemory returns false when uid is not RENDER_SERVICE_UID
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_ShouldCheckLowMemory_001, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogTask task("ShouldCheckLowMemory_001", taskFunc, 0, 1000, true);
 
    // 模拟 RENDER_SERVICE_UID 场景（实际测试中可能需要设置 uid）
    EXPECT_FALSE(task.ShouldCheckLowMemory());
}
 
/**
 * @tc.name: WatchdogTask_IsLowMemoryStatus_001
 * @tc.desc: verify IsLowMemoryStatus returns false when lowMemoryCheck is false
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_IsLowMemoryStatus_001, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogTask task("IsLowMemoryStatus_001", taskFunc, 0, 1000, true);
 
    // lowMemoryCheck 默认为 false
    EXPECT_FALSE(task.IsLowMemoryStatus());
}
 
/**
 * @tc.name: WatchdogTask_IsLowMemoryStatus_003
 * @tc.desc: verify IsLowMemoryStatus returns true when in low memory window
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_IsLowMemoryStatus_003, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogTask task("IsLowMemoryStatus_003", taskFunc, 0, 1000, true);
 
    task.lowMemoryCheck = true;
    task.lastLowMemoryTime = GetCurrentTickMillseconds();
    EXPECT_TRUE(task.IsLowMemoryStatus());
}
 
/**
 * @tc.name: WatchdogTask_IsLowMemoryStatus_004
 * @tc.desc: verify IsLowMemoryStatus returns false when out of low memory window
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_IsLowMemoryStatus_004, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogTask task("IsLowMemoryStatus_004", taskFunc, 0, 1000, true);
 
    task.lowMemoryCheck = true;
    // 设置一个超过 LOW_MEMORY_CHECK_WINDOW (60s) 的时间
    task.lastLowMemoryTime = GetCurrentTickMillseconds() - (60 * 1000 + 1000);
    EXPECT_FALSE(task.IsLowMemoryStatus());
}
 
/**
 * @tc.name: WatchdogTask_ShouldSkipExitForLowMemory_001
 * @tc.desc: verify ShouldSkipExitForLowMemory returns false when lowMemoryCheck disable
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_ShouldSkipExitForLowMemory_001, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogTask task("ShouldSkipExitForLowMemory_001", taskFunc, 0, 1000, true);
 
    task.lowMemoryCheck = false;
    EXPECT_FALSE(task.ShouldSkipExitForLowMemory());
}
 
/**
 * @tc.name: WatchdogTask_ShouldSkipExitForLowMemory_002
 * @tc.desc: verify ShouldSkipExitForLowMemory returns true when first detected low memory
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_ShouldSkipExitForLowMemory_002, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogTask task("ShouldSkipExitForLowMemory_002", taskFunc, 0, 1000, true);
 
    task.lowMemoryCheck = true;
    task.lastLowMemoryTime = GetCurrentTickMillseconds();
    task.lastLowMemoryFreezeTime = 0;
    EXPECT_TRUE(task.ShouldSkipExitForLowMemory());
}
 
/**
 * @tc.name: WatchdogTask_ShouldSkipExitForLowMemory_003
 * @tc.desc: verify ShouldSkipExitForLowMemory returns true when not detected low memory
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_ShouldSkipExitForLowMemory_003, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogTask task("ShouldSkipExitForLowMemory_003", taskFunc, 0, 1000, true);
 
    task.lowMemoryCheck = true;
    task.lastLowMemoryTime = GetCurrentTickMillseconds() - (60 * 1000 + 1000);
    task.lastLowMemoryFreezeTime = 0;
    EXPECT_FALSE(task.ShouldSkipExitForLowMemory());
}
 
/**
 * @tc.name: WatchdogTask_ShouldSkipExitForLowMemory_004
 * @tc.desc: verify ShouldSkipExitForLowMemory returns true when within max freeze time
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_ShouldSkipExitForLowMemory_004, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogTask task("ShouldSkipExitForLowMemory_004", taskFunc, 0, 1000, true);
 
    task.lowMemoryCheck = true;
    task.lastLowMemoryTime = GetCurrentTickMillseconds();
    task.lastLowMemoryFreezeTime = GetCurrentTickMillseconds() - 30000; // 30s 前
    EXPECT_TRUE(task.ShouldSkipExitForLowMemory());
}
 
/**
 * @tc.name: WatchdogTask_ShouldSkipExitForLowMemory_005
 * @tc.desc: verify ShouldSkipExitForLowMemory returns false when freeze time exceeds limit
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_ShouldSkipExitForLowMemory_005, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogTask task("ShouldSkipExitForLowMemory_005", taskFunc, 0, 1000, true);
 
    task.lowMemoryCheck = true;
    task.lastLowMemoryTime = GetCurrentTickMillseconds();
    task.lastLowMemoryFreezeTime = GetCurrentTickMillseconds() - (60 * 1000 + 1000); // 超过 60s
    EXPECT_FALSE(task.ShouldSkipExitForLowMemory());
}
 
/**
 * @tc.name: WatchdogTask_ShouldSkipSendEventForLowMemory_001
 * @tc.desc: verify ShouldSkipSendEventForLowMemory returns false when first call
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_ShouldSkipSendEventForLowMemory_001, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogTask task("ShouldSkipSendEventForLowMemory_001", taskFunc, 0, 1000, true);

    task.lowMemoryCheck = true;
    task.hadSendEvent = false;
    task.lastLowMemoryTime = GetCurrentTickMillseconds();
    EXPECT_FALSE(task.ShouldSkipSendEventForLowMemory());
    EXPECT_TRUE(task.hadSendEvent); // 第一次调用后应设置为 true
}

/**
 * @tc.name: WatchdogTask_ShouldSkipSendEventForLowMemory_002
 * @tc.desc: verify ShouldSkipSendEventForLowMemory returns true when hadSendEvent is true
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_ShouldSkipSendEventForLowMemory_002, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogTask task("ShouldSkipSendEventForLowMemory_002", taskFunc, 0, 1000, true);

    task.lowMemoryCheck = true;
    task.hadSendEvent = true;
    task.lastLowMemoryTime = GetCurrentTickMillseconds();
    EXPECT_TRUE(task.ShouldSkipSendEventForLowMemory());
}
 
/**
 * @tc.name: WatchdogTask_LowMemoryStrategy_001
 * @tc.desc: verify low memory strategy integration test
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_LowMemoryStrategy_001, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogTask task("LowMemoryStrategy_001", taskFunc, 0, 1000, true);
 
    // 模拟低内存场景
    task.lowMemoryCheck = true;
    task.lastLowMemoryTime = GetCurrentTickMillseconds();
    task.lastLowMemoryFreezeTime = 0;
 
    // 第一次检测到低内存，应该跳过退出
    EXPECT_TRUE(task.ShouldSkipExitForLowMemory());
 
    // 第一次应该发送事件
    EXPECT_FALSE(task.ShouldSkipSendEventForLowMemory());
 
    task.lastLowMemoryFreezeTime = task.lastLowMemoryFreezeTime - (10 * 1000);
 
    // 第二次不应该发送事件
    EXPECT_TRUE(task.ShouldSkipSendEventForLowMemory());
 
    // 后续60s内检测到低内存，应该跳过退出
    EXPECT_TRUE(task.ShouldSkipExitForLowMemory());
 
    task.lastLowMemoryFreezeTime = task.lastLowMemoryFreezeTime - (60 * 1000 + 1000);
 
    // 连续freeze超过60s，退出进程
    EXPECT_FALSE(task.ShouldSkipExitForLowMemory());
}
 
/**
 * @tc.name: WatchdogTask_LowMemoryStrategy_002
 * @tc.desc: verify low memory strategy integration test
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogTaskTest, WatchdogTask_LowMemoryStrategy_002, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogTask task("LowMemoryStrategy_002", taskFunc, 0, 1000, true);
 
    // 模拟低内存场景
    task.lowMemoryCheck = true;
    task.lastLowMemoryTime = GetCurrentTickMillseconds() - (30 * 1000);;
    task.lastLowMemoryFreezeTime = 0;
 
    // 第一次检测到低内存，应该跳过退出
    EXPECT_TRUE(task.ShouldSkipExitForLowMemory());
 
    // 第一次应该发送事件
    EXPECT_FALSE(task.ShouldSkipSendEventForLowMemory());
 
    task.lastLowMemoryTime = task.lastLowMemoryTime - (50 * 1000);
 
    // 近60s未发现低内存，正常退出进程
    EXPECT_FALSE(task.ShouldSkipExitForLowMemory());
}
#endif
} // namespace HiviewDFX
} // namespace OHOS
