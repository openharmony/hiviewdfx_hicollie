/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "xcollie_interface_test.h"

#include <gtest/gtest.h>
#include <string>

#include "xcollie.h"

using namespace testing::ext;

namespace OHOS {
namespace HiviewDFX {
void XCollieInterfaceTest::SetUpTestCase(void)
{
}

void XCollieInterfaceTest::TearDownTestCase(void)
{
}

void XCollieInterfaceTest::SetUp(void)
{
}

void XCollieInterfaceTest::TearDown(void)
{
}

/**
 * @tc.name: XCollieTimerParamTest
 * @tc.desc: Verify xcollie timer interface param
 * @tc.type: FUNC
 * @tc.require: SR000CPN2F AR000CTAMB
 * @tc.author: yangjing
 */
HWTEST_F(XCollieInterfaceTest, XCollieTimerParam_001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. input param name include special string
     * @tc.expected: step1. set timer successfully;
     */
    int id = XCollie::GetInstance().SetTimer("TimeoutTimerxce!@#$%^&*()", 1, nullptr, nullptr, XCOLLIE_FLAG_NOOP);
    ASSERT_GT(id, 0);

    /**
     * @tc.steps: step2. input param name include special string,update timer
     * @tc.expected: step2. update timer successfully;
     */
    bool ret = XCollie::GetInstance().UpdateTimer(id, 1);
    ASSERT_EQ(ret, true);
    XCollie::GetInstance().CancelTimer(id);

    /**
     * @tc.steps: step3. input param timeout is invalid
     * @tc.expected: step3. set timer failed;
     */
    id = XCollie::GetInstance().SetTimer("Timer", 0, nullptr, nullptr, XCOLLIE_FLAG_NOOP);
    ASSERT_EQ(id, INVALID_ID);

    /**
     * @tc.steps: step4. cancelTimer, input param id is invalid
     */
    XCollie::GetInstance().CancelTimer(-1);

    /**
     * @tc.steps: step5. updateTimer, param id is invalid
     * @tc.expected: step5. update timer failed;
     */
    ret = XCollie::GetInstance().UpdateTimer(-1, 10);
    ASSERT_EQ(ret, false);

    /**
     * @tc.steps: step6. updateTimer, param timeout is invalid
     * @tc.expected: step6. update timer failed;
     */
    ret = XCollie::GetInstance().UpdateTimer(10, 0);
    ASSERT_EQ(ret, false);

    /**
     * @tc.steps: step7. multple timer
     * @tc.expected: step7. set modify and cancel timer successfully;
     */
    id = XCollie::GetInstance().SetTimer("MyTimeout", 2, nullptr, nullptr, XCOLLIE_FLAG_NOOP);
    ASSERT_GT(id, 0);
    int id1 = XCollie::GetInstance().SetTimer("MyTimeout", 3, nullptr, nullptr, XCOLLIE_FLAG_NOOP);
    ASSERT_GT(id1, 0);
    sleep(1);
    XCollie::GetInstance().CancelTimer(id);
    int id2 = XCollie::GetInstance().SetTimer("MyTimeout", 4, nullptr, nullptr, XCOLLIE_FLAG_NOOP);
    ASSERT_GT(id2, 0);
    ret = XCollie::GetInstance().UpdateTimer(id1, 1);
    ASSERT_EQ(ret, true);
    sleep(2);
    XCollie::GetInstance().CancelTimer(id1);
    XCollie::GetInstance().CancelTimer(id2);

    /**
     * @tc.steps: step8. log event
     * @tc.expected: step8. log event successfully
     */
    ret = XCollie::GetInstance().SetTimer("MyTimeout", 1, nullptr, nullptr, XCOLLIE_FLAG_LOG);
    ASSERT_GT(id, 0);
    sleep(2);
    XCollie::GetInstance().CancelTimer(id);

    /**
     * @tc.steps: step9. callback test
     * @tc.expected: step9. callback can be executed successfully
     */
    bool flag = false;
    XCollieCallback callbackFunc = [&flag](void *) {
        flag = true;
    };
    id = XCollie::GetInstance().SetTimer("MyTimeout", 1, callbackFunc, nullptr, XCOLLIE_FLAG_LOG);
    ASSERT_GT(id, 0);
    sleep(2);
    ASSERT_EQ(flag, true);
    flag = false;
    id1 = XCollie::GetInstance().SetTimer("MyTimeout", 2, callbackFunc, nullptr, XCOLLIE_FLAG_LOG);
    ASSERT_GT(id1, 0);
    sleep(1);
    XCollie::GetInstance().CancelTimer(id1);
    ASSERT_EQ(flag, false);

    /**
     * @tc.steps: step10. recover test
     * @tc.expected: step10. recover can be executed successfully
     */
    id = XCollie::GetInstance().SetTimer("MyTimeout", 2, nullptr, nullptr, XCOLLIE_FLAG_RECOVERY);
    ASSERT_GT(id, 0);
    sleep(1);
}
} // namespace HiviewDFX
} // namespace OHOS
