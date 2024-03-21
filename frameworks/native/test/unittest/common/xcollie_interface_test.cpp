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
     * @tc.steps: step2. input param name include special string,cancel timer
     * @tc.expected: step2. update timer successfully;
     */
    XCollie::GetInstance().CancelTimer(id);

    /**
     * @tc.steps: step3. input param timeout is invalid
     * @tc.expected: step3. set timer failed;
     */
    id = XCollie::GetInstance().SetTimer("XCollieTimerParam_001", 0, nullptr, nullptr, XCOLLIE_FLAG_NOOP);
    ASSERT_EQ(id, INVALID_ID);

    /**
     * @tc.steps: step4. cancelTimer, input param id is invalid
     * @tc.expected: step4. cancel timer failed;
     */
    XCollie::GetInstance().CancelTimer(-1);
}

/**
 * @tc.name: XCollieTimerParamTest
 * @tc.desc: Verify xcollie timer interface param
 * @tc.type: FUNC
 * @tc.require: SR000CPN2F AR000CTAMB
 * @tc.author: yangjing
 */
HWTEST_F(XCollieInterfaceTest, XCollieTimerParam_002, TestSize.Level1)
{
    /**
     * @tc.steps: step5. multiple timer
     * @tc.expected: step5. cancel timer successfully;
     */
    bool flag = false;
    XCollieCallback callbackFunc = [&flag](void *) {
        flag = true;
    };
    bool flag1 = false;
    XCollieCallback callbackFunc1 = [&flag1](void *) {
        flag1 = true;
    };
    int id = XCollie::GetInstance().SetTimer("XCollieTimerParam_002", 2, callbackFunc, nullptr, XCOLLIE_FLAG_NOOP);
    ASSERT_GT(id, 0);
    int id1 = XCollie::GetInstance().SetTimer("XCollieTimerParam_002", 3, callbackFunc1, nullptr, XCOLLIE_FLAG_NOOP);
    ASSERT_GT(id1, 0);
    sleep(1);
    XCollie::GetInstance().CancelTimer(id);
    sleep(3);
    ASSERT_EQ(flag, false);
    ASSERT_EQ(flag1, true);

    flag = false;
    flag1 = false;
    id = XCollie::GetInstance().SetTimer("XCollieTimerParam_002", 2, callbackFunc, nullptr, XCOLLIE_FLAG_NOOP);
    ASSERT_GT(id, 0);
    id1 = XCollie::GetInstance().SetTimer("XCollieTimerParam_002", 3, callbackFunc1, nullptr, XCOLLIE_FLAG_NOOP);
    ASSERT_GT(id1, 0);
    sleep(1);
    XCollie::GetInstance().CancelTimer(id1);
    sleep(3);
    ASSERT_EQ(flag, true);
    ASSERT_EQ(flag1, false);
}

/**
 * @tc.name: XCollieTimerParamTest
 * @tc.desc: Verify xcollie timer interface param
 * @tc.type: FUNC
 * @tc.require: SR000CPN2F AR000CTAMB
 * @tc.author: yangjing
 */
HWTEST_F(XCollieInterfaceTest, XCollieTimerParam_003, TestSize.Level1)
{
    /**
     * @tc.steps: step6. log event
     * @tc.expected: step6. log event successfully
     */
    int id = XCollie::GetInstance().SetTimer("XCollieTimerParam_003", 1, nullptr, nullptr, XCOLLIE_FLAG_LOG);
    ASSERT_GT(id, 0);
    sleep(2);
    XCollie::GetInstance().CancelTimer(id);

    /**
     * @tc.steps: step7. callback test
     * @tc.expected: step7. callback can be executed successfully
     */
    bool flag = false;
    XCollieCallback callbackFunc = [&flag](void *) {
        flag = true;
    };
    id = XCollie::GetInstance().SetTimer("XCollieTimerParam_003", 1, callbackFunc, nullptr, XCOLLIE_FLAG_LOG);
    ASSERT_GT(id, 0);

    int id1 = XCollie::GetInstance().SetTimer("XCollieTimerParam_003", 2, callbackFunc, nullptr, XCOLLIE_FLAG_LOG);
    ASSERT_GT(id1, 0);
    sleep(2);
    XCollie::GetInstance().CancelTimer(id1);
    ASSERT_EQ(flag, true);

    /**
     * @tc.steps: step8. recover test
     * @tc.expected: step8. recover can be executed successfully
     */
    id = XCollie::GetInstance().SetTimer("XCollieTimerParam_003", 2, nullptr, nullptr, XCOLLIE_FLAG_RECOVERY);
    ASSERT_GT(id, 0);
    sleep(1);
}

/**
 * @tc.name: XCollieTimerParamTest
 * @tc.desc: Verify xcollie timer interface param
 * @tc.type: FUNC
 */
HWTEST_F(XCollieInterfaceTest, XCollieTimerParam_004, TestSize.Level1)
{
    bool flag = false;
    XCollieCallback callbackFunc = [&flag](void *) {
        flag = true;
    };
    int id = XCollie::GetInstance().SetTimer("XCollieTimerParam_004", 3, callbackFunc, nullptr, XCOLLIE_FLAG_LOG);
    ASSERT_GT(id, 0);
    sleep(2);
    sleep(3);
    XCollie::GetInstance().CancelTimer(id);
    ASSERT_EQ(flag, true);
}

/**
 * @tc.name: XCollieTimerParamTest
 * @tc.desc: Verify xcollie timer interface param
 * @tc.type: FUNC
 */
HWTEST_F(XCollieInterfaceTest, XCollieTimerParam_005, TestSize.Level1)
{
    int id = XCollie::GetInstance().SetTimerCount("HIT_EMPTY_WARNING", 2, 3);
    ASSERT_GT(id, 0);
    int i = 0;
    while (i < 3) {
        XCollie::GetInstance().TriggerTimerCount("HIT_EMPTY_WARNING", true, std::to_string(i));
        usleep(600 * 1000);
        i++;
    }
    sleep(1);
}
} // namespace HiviewDFX
} // namespace OHOS
