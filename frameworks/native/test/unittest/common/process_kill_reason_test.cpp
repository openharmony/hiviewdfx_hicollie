/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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


#include "process_kill_reason.h"
#include <string>

using namespace testing::ext;
using namespace OHOS::AppExecFwk;
namespace OHOS {
namespace HiviewDFX {

class ProcessKillReasonTest : public testing::Test {
public:
    ProcessKillReasonTest()
    {}
    ~ProcessKillReasonTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void ProcessKillReasonTest::SetUpTestCase(void)
{
}

void ProcessKillReasonTest::TearDownTestCase(void)
{
}

void ProcessKillReasonTest::SetUp(void)
{
}

void ProcessKillReasonTest::TearDown(void)
{
}

/**
 * @tc.name: GetKillReason test
 * @tc.desc: add test case
 * @tc.type: FUNC
 */
HWTEST_F(ProcessKillReasonTest, ProcessKillReasonTest_GetKillReason_001, TestSize.Level1)
{
    int killId = ProcessKillReason::REASON_KILL_APPLICATION_BY_BUNDLE_NAME;
    std::string ret = ProcessKillReason::GetKillReason(killId);
    EXPECT_EQ(ret, "KillApplicationByBundleName");
    ret = ProcessKillReason::GetAppExitReason(killId);
    EXPECT_EQ(ret, "KillApplication");
}

/**
 * @tc.name: GetKillReason test
 * @tc.desc: add test case
 * @tc.type: FUNC
 */
HWTEST_F(ProcessKillReasonTest, ProcessKillReasonTest_GetKillReason_002, TestSize.Level1)
{
    int killId = -1;
    std::string ret = ProcessKillReason::GetKillReason(killId);
    EXPECT_EQ(ret, "InvalidKillId");
    ret = ProcessKillReason::GetAppExitReason(killId);
    EXPECT_EQ(ret, "InvalidKillId");
}
} // namespace HiviewDFX
} // namespace OHOS
