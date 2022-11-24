/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "xcollie_checker_interface_test.h"

#include <gtest/gtest.h>
#include <string>

#include "xcollie.h"
#include "xcollie_checker_test.h"

using namespace testing::ext;

namespace OHOS {
namespace HiviewDFX {

void XCollieCheckerInterfaceTest::SetUpTestCase(void)
{
}

void XCollieCheckerInterfaceTest::TearDownTestCase(void)
{
}

void XCollieCheckerInterfaceTest::SetUp(void)
{
}

void XCollieCheckerInterfaceTest::TearDown(void)
{
}

/**
 * @tc.name: XCollieCheckerInterfaceTest
 * @tc.desc: Verify xcollie register checker interface param
 * @tc.type: FUNC
 */
HWTEST_F(XCollieCheckerInterfaceTest, XCollieRegisterCheckerParam_001, TestSize.Level1)
{
    /**
     * @tc.steps: step2. input param type is invalid
     * @tc.expected: step2. register checker failed;
     */
    sptr<XCollieCheckerTest> checkerTest = new XCollieCheckerTest("CheckerTest_NoBlock");
    checkerTest->SetBlockTime(1); // 1: 1second
    checkerTest->SetThreadBlockResult(false);
    ASSERT_EQ(checkerTest->GetThreadBlockResult(), false);
    ASSERT_EQ(checkerTest->GetCheckerName(), "CheckerTest_NoBlock");
    XCollie::GetInstance().RegisterXCollieChecker(checkerTest, XCOLLIE_LOCK | XCOLLIE_THREAD);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // 1: 1second
    checkerTest->CheckLock();
    checkerTest->CheckThreadBlock();
    const int conutOut = 60;
    for (int i = 0; !checkerTest->GetThreadBlockResult(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 100: milliseconds
        if (i > conutOut) {
            break;
        }
    }

    ASSERT_EQ(checkerTest->GetThreadBlockResult(), true);

    checkerTest = nullptr;
}
} // namespace HiviewDFX
} // namespace OHOS
