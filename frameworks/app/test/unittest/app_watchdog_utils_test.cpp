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

#include "app_watchdog_utils.h"

#include <gtest/gtest.h>
#include <string>
#include <thread>

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
class AppWatchdogUtilsTest : public testing::Test {
public:
    AppWatchdogUtilsTest()
    {}
    ~AppWatchdogUtilsTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void AppWatchdogUtilsTest::SetUpTestCase(void)
{
}

void AppWatchdogUtilsTest::TearDownTestCase(void)
{
}

void AppWatchdogUtilsTest::SetUp(void)
{
}

void AppWatchdogUtilsTest::TearDown(void)
{
}

/**
 * @tc.name: AppWatchdogUtils IsBetaVersion Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(AppWatchdogUtilsTest, AppWatchdogUtils_IsBetaVersion_001, TestSize.Level1)
{
    bool ret = AppWatchdogUtils::IsBetaVersion();
    EXPECT_TRUE(!ret);
}
} // namespace HiviewDFX
} // namespace OHOS
