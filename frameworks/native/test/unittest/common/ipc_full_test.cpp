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

#include "ipc_full_test.h"

#include "ipc_full.h"

using namespace testing::ext;

namespace OHOS {
namespace HiviewDFX {
void IpcFullTest::SetUpTestCase(void)
{
}

void IpcFullTest::TearDownTestCase(void)
{
}

void IpcFullTest::SetUp(void)
{
}

void IpcFullTest::TearDown(void)
{
}

/**
 * @tc.name: AddIpcFullTest
 * @tc.desc: Verify add ipc full interface
 * @tc.type: FUNC
 * @tc.require: AR20250307218293
 * @tc.author: zhengchengkai
 */
    HWTEST_F(IpcFullTest, AddIpcFullTest_001, TestSize.Level1)
    {
    /**
     * @tc.steps: step1. input interval param below the min allowable threshold
     * @tc.expected: step1. add ipc full failed;
     */
    bool result = IpcFull::GetInstance().AddIpcFull(4);
    ASSERT_EQ(result, false);

    /**
     * @tc.steps: step2. input interval param above the max allowable threshold
     * @tc.expected: step2. add ipc full failed;
     */
    result = IpcFull::GetInstance().AddIpcFull(32);
    ASSERT_EQ(result, false);

    /**
     * @tc.steps: step3. input valid interval param
     * @tc.expected: step3. add ipc full successfully;
     */
    result = IpcFull::GetInstance().AddIpcFull(12);
    ASSERT_EQ(result, true);

    /**
     * @tc.steps: step4. ipc full task already exists
     * @tc.expected: step4. add ipc full failed;
     */
    result = IpcFull::GetInstance().AddIpcFull(12);
    ASSERT_EQ(result, false);
    }
} // namespace HiviewDFX
} // namespace OHOS
 