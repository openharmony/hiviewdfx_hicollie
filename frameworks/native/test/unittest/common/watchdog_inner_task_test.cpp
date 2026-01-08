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
#include <fstream>
#include <fcntl.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <chrono>
#include <cstdio>

#define private public
#define protected public
#include "watchdog_inner.h"
#undef private
#undef protected

#include "xcollie_define.h"
#include "xcollie_utils.h"
#include "directory_ex.h"
#include "file_ex.h"
#include "parameters.h"
#include "watchdog_inner_util_test.h"

using namespace testing::ext;
using namespace OHOS::AppExecFwk;
namespace OHOS {
namespace HiviewDFX {

class WatchdogInnerTaskTest : public testing::Test {
public:
    WatchdogInnerTaskTest()
    {}
    ~WatchdogInnerTaskTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void WatchdogInnerTaskTest::SetUpTestCase(void)
{
}

void WatchdogInnerTaskTest::TearDownTestCase(void)
{
}

void WatchdogInnerTaskTest::SetUp(void)
{
    InitSeLinuxEnabled();
}

void WatchdogInnerTaskTest::TearDown(void)
{
    CancelSeLinuxEnabled();
}

static void InitBeginFuncTest(const char* name)
{
    std::string nameStr(name);
}

static void InitEndFuncTest(const char* name)
{
    std::string nameStr(name);
}

/**
 * @tc.name: WatchdogInner thread RemoveInnerTask test
 * @tc.desc: add test case
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTaskTest, WatchdogInnerTaskTest_TaskRemoveInnerTask_001, TestSize.Level1)
{
    WatchdogInner::GetInstance().RemoveInnerTask("WatchdogInnerTaskTest_RemoveInnerTask_001");
    ASSERT_EQ(WatchdogInner::GetInstance().checkerQueue_.size(), 0);
}

/**
 * @tc.name: WatchdogInner thread run a oneshot task
 * @tc.desc: Verify whether the task has been executed failed
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTaskTest, WatchdogInnerTaskTest_RunOneShotTask_001, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogInner::GetInstance().RunOneShotTask("", taskFunc, 0);
    ASSERT_EQ(WatchdogInner::GetInstance().checkerQueue_.size(), 0);
}

/**
 * @tc.name: WatchdogInner thread run a periodical task
 * @tc.desc: Verify whether the task has been executed failed
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTaskTest, WatchdogInnerTaskTest_RunPeriodicalTask_001, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    std::string name = "RunPeriodicalTask_001";
    WatchdogInner::GetInstance().RunPeriodicalTask(name, taskFunc, 2000, 0);
    ASSERT_TRUE(WatchdogInner::GetInstance().checkerQueue_.size() > 0);
    WatchdogInner::GetInstance().TriggerTimerCountTask(name, false, "test");
    WatchdogInner::GetInstance().TriggerTimerCountTask(name, true, "test");
    size_t beforeSize = WatchdogInner::GetInstance().taskNameSet_.size();
    WatchdogInner::GetInstance().RemoveInnerTask(name);
    size_t afterSize = WatchdogInner::GetInstance().taskNameSet_.size();
    ASSERT_EQ(beforeSize, (afterSize + 1));
    WatchdogInner::GetInstance().RunPeriodicalTask("", taskFunc, 2000, 0);
    WatchdogInner::GetInstance().SetTimerCountTask("", 1, 1);
    WatchdogInner::GetInstance().RemoveInnerTask("");
}

/**
 * @tc.name: WatchdogInner fetch next task;
 * @tc.desc: Verify whether the task acquisition failed
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTaskTest, WatchdogInnerTaskTest_FetchNextTask_001, TestSize.Level1)
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
    WatchdogTask task2("", taskFunc, delay, interval, isOneshot);
    ASSERT_EQ(WatchdogInner::GetInstance().FetchNextTask(now, task1), 60000);
}
} // namespace HiviewDFX
} // namespace OHOS
