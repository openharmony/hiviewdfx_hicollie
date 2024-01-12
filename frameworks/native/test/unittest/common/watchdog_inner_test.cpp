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

#include <gtest/gtest.h>
#include <string>
#include <thread>
#include "watchdog_inner_test.h"

#define private public
#define protected public
#include "watchdog_inner.h"
#undef private
#undef protected

#include "xcollie_utils.h"
#include "directory_ex.h"
#include "file_ex.h"
#include "event_handler.h"
#include "ffrt_inner.h"

using namespace testing::ext;
using namespace OHOS::AppExecFwk;
namespace OHOS {
namespace HiviewDFX {
void WatchdogInnerTest::SetUpTestCase(void)
{
}

void WatchdogInnerTest::TearDownTestCase(void)
{
}

void WatchdogInnerTest::SetUp(void)
{
}

void WatchdogInnerTest::TearDown(void)
{
}

/**
 * @tc.name: WatchdogInner thread run a oneshot task
 * @tc.desc: Verify whether the task has been executed failed
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_RunOneShotTask_001, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogInner::GetInstance().RunOneShotTask("", taskFunc, 0);
    ASSERT_EQ(taskResult, 0);
}

/**
 * @tc.name: WatchdogInner thread run a periodical task
 * @tc.desc: Verify whether the task has been executed failed
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_RunPeriodicalTask_001, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogInner::GetInstance().RunPeriodicalTask("", taskFunc, 2000, 0);
    ASSERT_EQ(taskResult, 0);
}

/**
 * @tc.name: WatchdogInner fetch next task;
 * @tc.desc: Verify whether the task acquisition failed
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_FetchNextTask_001, TestSize.Level1)
{
    uint64_t now = GetCurrentTickMillseconds();
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    const std::string name = "task1";
    uint64_t delay = 0;
    uint64_t interval = 0;
    bool isOneshot = true;
    WatchdogTask task(name, taskFunc, delay, interval, isOneshot);
    int id = WatchdogInner::GetInstance().InsertWatchdogTaskLocked(name, WatchdogTask(name, taskFunc,
        delay, interval, isOneshot));
    ASSERT_GT(id, 0);
    WatchdogInner::GetInstance().isNeedStop_.store(true);
    ASSERT_EQ(WatchdogInner::GetInstance().FetchNextTask(now, task), 60000);
    WatchdogInner::GetInstance().isNeedStop_.store(false);
    WatchdogTask task1;
    ASSERT_EQ(WatchdogInner::GetInstance().FetchNextTask(now, task1), 60000);
}

/**
 * @tc.name: WatchdogInner fetch next task;
 * @tc.desc: Verify whether the task acquisition successfully
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_FetchNextTask_002, TestSize.Level1)
{
    uint64_t now = GetCurrentTickMillseconds() + 6500;
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    const std::string name = "IPC_FULL";
    uint64_t delay = 0;
    uint64_t interval = 0;
    bool isOneshot = true;
    WatchdogTask task(name, taskFunc, delay, interval, isOneshot);
    int id = WatchdogInner::GetInstance().InsertWatchdogTaskLocked(name, WatchdogTask(name, taskFunc,
        delay, interval, isOneshot));
    ASSERT_GT(id, 0);
    ASSERT_EQ(WatchdogInner::GetInstance().FetchNextTask(now, task), 0);
}

/**
 * @tc.name: WatchdogInner FfrtCallback;
 * @tc.desc: Verify FfrtCallback
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_FfrtCallback_001, TestSize.Level1)
{
    uint64_t taskId = 1;
    const char *taskInfo = "task";
    uint32_t delayedTaskCount = 0;
    WatchdogInner::GetInstance().FfrtCallback(taskId, taskInfo, delayedTaskCount);
}

/**
 * @tc.name: WatchdogInner kick watchdog;
 * @tc.desc: Verify whether the kick watchdog failed
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_KickWatchdog_001, TestSize.Level1)
{
    bool ret = WatchdogInner::GetInstance().KickWatchdog();
    ASSERT_EQ(ret, true);
}

/**
 * @tc.name: WatchdogInner IpcCheck;
 * @tc.desc: Verify IpcCheck
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_IpcCheck_001, TestSize.Level1)
{
    WatchdogInner::GetInstance().IpcCheck();
}

/**
 * @tc.name: WatchdogInner is exceedMaxTaskLocked;
 * @tc.desc: Verify whether checkerQueue_ is over MAX_WATCH_NUM;
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_IsExceedMaxTaskLocked_001, TestSize.Level1)
{
    bool ret = WatchdogInner::GetInstance().IsExceedMaxTaskLocked();
    ASSERT_EQ(ret, false);
}
} // namespace HiviewDFX
} // namespace OHOS