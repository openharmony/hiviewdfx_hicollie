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

/**
 * @tc.name: AppWatchdog BundleName Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogTest, AppWatchdogTest_BundleName_001, TestSize.Level1)
{
    std::string testValue = "AppWatchdogTest_BundleName_001";
    AppWatchdog::GetInstance().SetBundleInfo(testValue, "1.1.0");
    EXPECT_EQ(AppWatchdog::GetInstance().GetBundleName(), testValue);
    testValue = "AppWatchdog";
    AppWatchdog::GetInstance().SetBundleInfo(testValue, "1.1.0");
    EXPECT_EQ(AppWatchdog::GetInstance().GetBundleName(), testValue);
    testValue = "com.ohos.sceneboard";
    AppWatchdog::GetInstance().SetBundleInfo(testValue, "1.1.0");
    EXPECT_EQ(AppWatchdog::GetInstance().GetBundleName(), testValue);
}

/**
 * @tc.name: AppWatchdog IsSystemApp Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogTest, AppWatchdogTest_IsSystemApp_001, TestSize.Level1)
{
    bool isSystemApp = true;
    AppWatchdog::GetInstance().SetSystemApp(isSystemApp);
    EXPECT_EQ(AppWatchdog::GetInstance().GetSystemApp(), isSystemApp);
    isSystemApp = false;
    AppWatchdog::GetInstance().SetSystemApp(isSystemApp);
    EXPECT_EQ(AppWatchdog::GetInstance().GetSystemApp(), isSystemApp);
}

/**
 * @tc.name: AppWatchdog AppDebug Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogTest, AppWatchdogTest_AppDebug_001, TestSize.Level1)
{
    bool isAppDebug = true;
    AppWatchdog::GetInstance().SetAppDebug(isAppDebug);
    EXPECT_EQ(AppWatchdog::GetInstance().GetAppDebug(), isAppDebug);
    isAppDebug = false;
    AppWatchdog::GetInstance().SetAppDebug(isAppDebug);
    EXPECT_EQ(AppWatchdog::GetInstance().GetAppDebug(), isAppDebug);
}

/**
 * @tc.name: AppWatchdog Foreground Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogTest, AppWatchdogTest_Foreground_001, TestSize.Level1)
{
    bool isForeground = true;
    AppWatchdog::GetInstance().SetForeground(isForeground);
    EXPECT_EQ(AppWatchdog::GetInstance().GetForeground(), isForeground);
    isForeground = false;
    AppWatchdog::GetInstance().SetForeground(isForeground);
    EXPECT_EQ(AppWatchdog::GetInstance().GetForeground(), isForeground);
}

/**
 * @tc.name: AppWatchdog SpecifiedProcessName Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogTest, AppWatchdogTest_SpecifiedProcessName_001, TestSize.Level1)
{
    std::string testValue = "AppWatchdogTest_SpecifiedProcessName_001";
    AppWatchdog::GetInstance().SetSpecifiedProcessName(testValue);
    EXPECT_EQ(AppWatchdog::GetInstance().GetSpecifiedProcessName(), testValue);
    testValue = "AppWatchdog";
    AppWatchdog::GetInstance().SetSpecifiedProcessName(testValue);
    EXPECT_EQ(AppWatchdog::GetInstance().GetSpecifiedProcessName(), testValue);
}

/**
 * @tc.name: AppWatchdog ScrollState Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogTest, AppWatchdogTest_ScrollState_001, TestSize.Level1)
{
    bool isScroll = true;
    AppWatchdog::GetInstance().SetScrollState(isScroll);
    EXPECT_EQ(AppWatchdog::GetInstance().GetScrollState(), isScroll);
    isScroll = false;
    AppWatchdog::GetInstance().SetScrollState(isScroll);
    EXPECT_EQ(AppWatchdog::GetInstance().GetScrollState(), isScroll);
}
} // namespace HiviewDFX
} // namespace OHOS
