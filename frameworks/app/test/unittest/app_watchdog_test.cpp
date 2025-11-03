/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "app_watchdog.h"

#include <gtest/gtest.h>
#include <string>
#include <thread>

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
class AppWatchdogTest : public testing::Test {
public:
    AppWatchdogTest()
    {}
    ~AppWatchdogTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void AppWatchdogTest::SetUpTestCase(void)
{
}

void AppWatchdogTest::TearDownTestCase(void)
{
}

void AppWatchdogTest::SetUp(void)
{
}

void AppWatchdogTest::TearDown(void)
{
}

/**
 * @tc.name: AppWatchdog GetReservedTimeForLogging Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogTest, AppWatchdog_GetReservedTimeForLogging_001, TestSize.Level1)
{
    int ret = AppWatchdog::GetInstance().GetReservedTimeForLogging();
    EXPECT_TRUE(ret >= 3500);
    ret = AppWatchdog::GetInstance().GetReservedTimeForLogging();
    EXPECT_TRUE(ret >= 3500);
}
} // namespace HiviewDFX
} // namespace OHOS
