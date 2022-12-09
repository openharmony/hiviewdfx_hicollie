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

#include "xcollie_inner_test.h"

#include <gtest/gtest.h>
#include <string>

#include "xcollie.h"
#include "xcollie_inner.h"
#include "xcollie_checker_test.h"

using namespace testing::ext;

namespace OHOS {
namespace HiviewDFX {

void XCollieInnerTest::SetUpTestCase(void)
{
}

void XCollieInnerTest::TearDownTestCase(void)
{
}

void XCollieInnerTest::SetUp(void)
{
}

void XCollieInnerTest::TearDown(void)
{
}

/**
 * @tc.name: XCollieInnerTest
 * @tc.desc: Verify xcollie register checker interface param
 * @tc.type: FUNC
 */
HWTEST_F(XCollieInnerTest, XCollieInnerTest_001, TestSize.Level3)
{
    auto& xCollieInner = XCollieInner::GetInstance();
    xCollieInner.RegisterXCollieChecker(nullptr, XCOLLIE_LOCK);
    ASSERT_EQ(xCollieInner.GetBlockdServiceName(), "");
}

/**
 * @tc.name: XCollieInnerTest
 * @tc.desc: Verify xcollie register checker interface param
 * @tc.type: FUNC
 */
HWTEST_F(XCollieInnerTest, XCollieInnerTest_002, TestSize.Level3)
{
    auto& xCollieInner = XCollieInner::GetInstance();
    sptr<XCollieCheckerTest> checkerTest = new XCollieCheckerTest("CheckerTest_NoBlock");
    xCollieInner.RegisterXCollieChecker(checkerTest, 0);
    ASSERT_EQ(checkerTest->GetThreadBlockResult(), false);
}

/**
 * @tc.name: XCollieInnerTest
 * @tc.desc: Verify xcollie register checker interface param
 * @tc.type: FUNC
 */
HWTEST_F(XCollieInnerTest, XCollieInnerTest_003, TestSize.Level3)
{
    auto& xCollieInner = XCollieInner::GetInstance();
    sptr<XCollieCheckerTest> checkerTest = new XCollieCheckerTest("CheckerTest_003");
    xCollieInner.RegisterXCollieChecker(checkerTest, XCOLLIE_LOCK);
    for (int i = 0; xCollieInner.GetBlockdServiceName().empty() && (i < 10); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    xCollieInner.UnRegisterXCollieChecker(checkerTest);
    ASSERT_EQ(xCollieInner.GetBlockdServiceName(), "");
}
} // namespace HiviewDFX
} // namespace OHOS
