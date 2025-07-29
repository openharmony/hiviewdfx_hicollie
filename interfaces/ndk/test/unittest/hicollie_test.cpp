/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <cstdlib>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <chrono>

#include "watchdog.h"
#include "hicollie.h"

using namespace testing::ext;

namespace {
class HiCollieTest : public ::testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};

void TaskTest()
{
    printf("TaskTest");
}

void InitBeginFunc(const char* eventName)
{
    std::string str(eventName);
    printf("InitBeginFunc eventName: %s\n", str.c_str());
}

void InitEndFunc(const char* eventName)
{
    std::string str(eventName);
    printf("InitBeginFunc eventName: %s\n", str.c_str());
}

/**
 * @tc.name: OH_HiCollie_Init_StuckDetection
 * @tc.desc: test OH_HiCollie_Init_StuckDetection
 * @tc.type: FUNC
 */
HWTEST_F(HiCollieTest, Test_OH_HiCollie_Init_StuckDetection_1, TestSize.Level1)
{
    int result = OH_HiCollie_Init_StuckDetection(&TaskTest);
    EXPECT_EQ(result, HICOLLIE_SUCCESS);
}

/**
 * @tc.name: OH_HiCollie_Init_StuckDetection
 * @tc.desc: test OH_HiCollie_Init_StuckDetection
 * @tc.type: FUNC
 */
HWTEST_F(HiCollieTest, Test_OH_HiCollie_Init_StuckDetection_2, TestSize.Level1)
{
    int result = OH_HiCollie_Init_StuckDetection(nullptr);
    EXPECT_EQ(result, HICOLLIE_SUCCESS);
}

/**
 * @tc.name: OH_HiCollie_Init_StuckDetectionWithTimeout
 * @tc.desc: test OH_HiCollie_Init_StuckDetectionWithTimeout
 * @tc.type: FUNC
 */
HWTEST_F(HiCollieTest, Test_OH_HiCollie_Init_StuckDetectionWithTimeout_1, TestSize.Level0)
{
    int result = OH_HiCollie_Init_StuckDetectionWithTimeout(&TaskTest, 5);
    EXPECT_EQ(result, HICOLLIE_SUCCESS);
    result = OH_HiCollie_Init_StuckDetection(&TaskTest);
    EXPECT_EQ(result, HICOLLIE_SUCCESS);
}

/**
 * @tc.name: OH_HiCollie_Init_StuckDetectionWithTimeout
 * @tc.desc: test OH_HiCollie_Init_StuckDetectionWithTimeout
 * @tc.type: FUNC
 */
HWTEST_F(HiCollieTest, Test_OH_HiCollie_Init_StuckDetectionWithTimeout_2, TestSize.Level1)
{
    int result = OH_HiCollie_Init_StuckDetectionWithTimeout(nullptr, 10);
    EXPECT_EQ(result, HICOLLIE_SUCCESS);
}

/**
 * @tc.name: OH_HiCollie_Init_StuckDetectionWithTimeout
 * @tc.desc: test OH_HiCollie_Init_StuckDetectionWithTimeout
 * @tc.type: FUNC
 */
HWTEST_F(HiCollieTest, Test_OH_HiCollie_Init_StuckDetectionWithTimeout_3, TestSize.Level1)
{
    int result = OH_HiCollie_Init_StuckDetectionWithTimeout(&TaskTest, 1);
    EXPECT_EQ(result, HICOLLIE_INVALID_ARGUMENT);
    result = OH_HiCollie_Init_StuckDetectionWithTimeout(&TaskTest, 16);
    EXPECT_EQ(result, HICOLLIE_INVALID_ARGUMENT);
}

/**
 * @tc.name: OH_HiCollie_Init_JankDetection
 * @tc.desc: test OH_HiCollie_Init_JankDetection
 * @tc.type: FUNC
 */
HWTEST_F(HiCollieTest, Test_OH_HiCollie_Init_JankDetection_1, TestSize.Level1)
{
    OH_HiCollie_BeginFunc begin_ = InitBeginFunc;
    OH_HiCollie_EndFunc end_ = InitEndFunc;
    HiCollie_DetectionParam param {
        .sampleStackTriggerTime = 150,
        .reserved = 0,
    };
    int result = OH_HiCollie_Init_JankDetection(&begin_, &end_, param);
    EXPECT_EQ(result, HICOLLIE_SUCCESS);
    result = OH_HiCollie_Init_JankDetection(nullptr, nullptr, param);
    EXPECT_EQ(result, HICOLLIE_SUCCESS);
}

/**
 * @tc.name: OH_HiCollie_Report
 * @tc.desc: test OH_HiCollie_Report
 * @tc.type: FUNC
 */
HWTEST_F(HiCollieTest, Test_OH_HiCollie_Report_1, TestSize.Level1)
{
    bool isSixSecond = false;
    OHOS::HiviewDFX::Watchdog::GetInstance().SetForeground(true);
    int result = OH_HiCollie_Report(&isSixSecond);
    printf("OH_HiCollie_Report result: %d\n", result);
    EXPECT_TRUE(isSixSecond);
    result = OH_HiCollie_Report(&isSixSecond);
    printf("OH_HiCollie_Report result: %d\n", result);
    EXPECT_FALSE(isSixSecond);
    result = OH_HiCollie_Report(nullptr);
    EXPECT_EQ(result, HICOLLIE_INVALID_ARGUMENT);
    OHOS::HiviewDFX::Watchdog::GetInstance().SetAppDebug(true);
    result = OH_HiCollie_Report(&isSixSecond);
    EXPECT_EQ(result, HICOLLIE_SUCCESS);
}

/**
 * @tc.name: OH_HiCollie_SetTimer
 * @tc.desc: test OH_HiCollie_SetTimer
 * @tc.type: FUNC
 */
HWTEST_F(HiCollieTest, Test_OH_HiCollie_SetTimer_1, TestSize.Level1)
{
    int id;
    HiCollie_SetTimerParam param = {nullptr, 1, nullptr, nullptr, HiCollie_Flag::HICOLLIE_FLAG_NOOP};
    HiCollie_ErrorCode errCode = OH_HiCollie_SetTimer(param, &id);
    EXPECT_EQ(errCode, HICOLLIE_INVALID_TIMER_NAME);
    param = {"testSetTimer", 0, nullptr, nullptr, HiCollie_Flag::HICOLLIE_FLAG_NOOP};
    errCode = OH_HiCollie_SetTimer(param, &id);
    EXPECT_EQ(errCode, HICOLLIE_INVALID_TIMEOUT_VALUE);
    param = {"testSetTimer", 1, nullptr, nullptr, HiCollie_Flag::HICOLLIE_FLAG_NOOP};
    errCode = OH_HiCollie_SetTimer(param, nullptr);
    EXPECT_EQ(errCode, HICOLLIE_WRONG_TIMER_ID_OUTPUT_PARAM);
    param = {"testSetTimer", 1, nullptr, nullptr, HiCollie_Flag::HICOLLIE_FLAG_NOOP};
    errCode = OH_HiCollie_SetTimer(param, &id);
    EXPECT_FALSE(errCode == HICOLLIE_WRONG_PROCESS_CONTEXT);
    EXPECT_EQ(errCode, HICOLLIE_SUCCESS);
    printf("OH_HiCollie_SetTimer id: %d\n", id);
    EXPECT_TRUE(id > 0);
}
 
/**
 * @tc.name: OH_HiCollie_CancelTimer
 * @tc.desc: test OH_HiCollie_CancelTimer
 * @tc.type: FUNC
 */
HWTEST_F(HiCollieTest, Test_OH_HiCollie_CancelTimer_1, TestSize.Level1)
{
    bool flag = true;
    int id = 2025;
    OH_HiCollie_CancelTimer(id);
    EXPECT_TRUE(flag);
}
} // namespace
