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
/**
 * @tc.name: AppWatchdogInner BundleName Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogInnerTest, AppWatchdogInnerTest_BundleName_001, TestSize.Level1)
{
    std::string testValue = "AppWatchdogInnerTest_BundleName_001";
    AppWatchdogInner::GetInstance().SetBundleInfo(testValue, "1.1.0");
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetBundleName(), testValue);
    testValue = "AppWatchdogInner";
    AppWatchdogInner::GetInstance().SetBundleInfo(testValue, "1.1.0");
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetBundleName(), testValue);
    testValue = "com.ohos.sceneboard";
    AppWatchdogInner::GetInstance().SetBundleInfo(testValue, "1.1.0");
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetBundleName(), testValue);
}

/**
 * @tc.name: AppWatchdogInner IsSystemApp Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogInnerTest, AppWatchdogInnerTest_IsSystemApp_001, TestSize.Level1)
{
    bool isSystemApp = true;
    AppWatchdogInner::GetInstance().SetSystemApp(isSystemApp);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetSystemApp(), isSystemApp);
    isSystemApp = false;
    AppWatchdogInner::GetInstance().SetSystemApp(isSystemApp);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetSystemApp(), isSystemApp);
}

/**
 * @tc.name: AppWatchdogInner AppDebug Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogInnerTest, AppWatchdogInnerTest_AppDebug_001, TestSize.Level1)
{
    bool isAppDebug = true;
    AppWatchdogInner::GetInstance().SetAppDebug(isAppDebug);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetAppDebug(), isAppDebug);
    isAppDebug = false;
    AppWatchdogInner::GetInstance().SetAppDebug(isAppDebug);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetAppDebug(), isAppDebug);
}

/**
 * @tc.name: AppWatchdogInner Foreground Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogInnerTest, AppWatchdogInnerTest_Foreground_001, TestSize.Level1)
{
    bool isForeground = true;
    AppWatchdogInner::GetInstance().SetForeground(isForeground);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetForeground(), isForeground);
    isForeground = false;
    AppWatchdogInner::GetInstance().SetForeground(isForeground);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetForeground(), isForeground);
}

/**
 * @tc.name: AppWatchdogInner SpecifiedProcessName Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogInnerTest, AppWatchdogInnerTest_SpecifiedProcessName_001, TestSize.Level1)
{
    std::string testValue = "AppWatchdogInnerTest_SpecifiedProcessName_001";
    AppWatchdogInner::GetInstance().SetSpecifiedProcessName(testValue);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetSpecifiedProcessName(), testValue);
    testValue = "AppWatchdogInner";
    AppWatchdogInner::GetInstance().SetSpecifiedProcessName(testValue);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetSpecifiedProcessName(), testValue);
}

/**
 * @tc.name: AppWatchdogInner ScrollState Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogInnerTest, AppWatchdogInnerTest_ScrollState_001, TestSize.Level1)
{
    bool isScroll = true;
    AppWatchdogInner::GetInstance().SetScrollState(isScroll);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetScrollState(), isScroll);
    isScroll = false;
    AppWatchdogInner::GetInstance().SetScrollState(isScroll);
    EXPECT_EQ(AppWatchdogInner::GetInstance().GetScrollState(), isScroll);
}
} // namespace HiviewDFX
} // namespace OHOS
