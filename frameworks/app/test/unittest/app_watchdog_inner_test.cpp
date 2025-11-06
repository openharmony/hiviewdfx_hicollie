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

#include "app_watchdog_inner.h"

#include <gtest/gtest.h>
#include <string>
#include <thread>

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
class AppWatchdogInnerTest : public testing::Test {
public:
    AppWatchdogInnerTest()
    {}
    ~AppWatchdogInnerTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void AppWatchdogInnerTest::SetUpTestCase(void)
{
}

void AppWatchdogInnerTest::TearDownTestCase(void)
{
}

void AppWatchdogInnerTest::SetUp(void)
{
}

void AppWatchdogInnerTest::TearDown(void)
{
}

/**
 * @tc.name: AppWatchdogInner GetReservedTimeForLogging Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogInnerTest, AppWatchdogInner_GetReservedTimeForLogging_001, TestSize.Level1)
{
    int ret = AppWatchdogInner::GetInstance().GetReservedTimeForLogging();
    EXPECT_TRUE(ret >= 3500);
    ret = AppWatchdogInner::GetInstance().GetReservedTimeForLogging();
    EXPECT_TRUE(ret >= 3500);
}
} // namespace HiviewDFX
} // namespace OHOS
