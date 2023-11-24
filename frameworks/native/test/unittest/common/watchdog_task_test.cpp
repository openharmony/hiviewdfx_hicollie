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
    int ret = task1.EvaluateCheckerState();
    EXPECT_TRUE(ret >= 0);
}
} // namespace HiviewDFX
} // namespace OHOS
