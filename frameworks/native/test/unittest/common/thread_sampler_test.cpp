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

#include "thread_sampler_test.h"

#include <csignal>

#include "thread_sampler.h"
#include "watchdog.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
void ThreadSamplerTest::SetUpTestCase(void)
{
    printf("SetUpTestCase.\n");
}

void ThreadSamplerTest::TearDownTestCase(void)
{
    printf("TearDownTestCase.\n");
    Watchdog::GetInstance().StopWatchdog();
}

void ThreadSamplerTest::SetUp(void)
{
    printf("SetUp.\n");
}

void ThreadSamplerTest::TearDown(void)
{
    printf("TearDown.\n");
}

/**
 * @tc.name: ThreadSamplerTest_001
 * @tc.desc: sample thread 2 times and check the stacktrace
 * @tc.type: FUNC
 * @tc.require
 */
HWTEST_F(ThreadSamplerTest, ThreadSamplerTest_001, TestSize.Level3)
{
    printf("ThreadSamplerTest_001\n");
    printf("Total:450MS Sample:150MS \n");
    ThreadSampler::GetInstance().Init();
    auto sampleHandler = []() {
        ThreadSampler::GetInstance().Sample();
    };

    std::string stack;
    auto collectHandler = [&stack]() {
        ThreadSampler::GetInstance().CollectStack(stack, false);
    };

    Watchdog::GetInstance().RunOneShotTask("ThreadSamplerTest", sampleHandler, 150);
    Watchdog::GetInstance().RunOneShotTask("ThreadSamplerTest", sampleHandler, 300);
    Watchdog::GetInstance().RunOneShotTask("CollectHandler", collectHandler, 450);

    int32_t left = 4;
    int32_t end = time(nullptr) + left;
    while (left > 0) {
        left = end - time(nullptr);
    }
    sleep(4);
    ASSERT_NE(stack, "");
    printf("stack:\n%s", stack.c_str());
}

/**
 * @tc.name: ThreadSamplerTest_002
 * @tc.desc: sample thread 2 times and check the stacktrace in tree format
 * @tc.type: FUNC
 * @tc.require
 */
HWTEST_F(ThreadSamplerTest, ThreadSamplerTest_002, TestSize.Level3)
{
    printf("ThreadSamplerTest_002\n");
    printf("Total:450MS Sample:150MS \n");
    ThreadSampler::GetInstance().Init();
    auto sampleHandler = []() {
        ThreadSampler::GetInstance().Sample();
    };

    std::string stack;
    auto collectHandler = [&stack]() {
        ThreadSampler::GetInstance().CollectStack(stack, true);
    };

    Watchdog::GetInstance().RunOneShotTask("ThreadSamplerTest", sampleHandler, 150);
    Watchdog::GetInstance().RunOneShotTask("ThreadSamplerTest", sampleHandler, 300);
    Watchdog::GetInstance().RunOneShotTask("CollectHandler", collectHandler, 450);

    int32_t left = 4;
    int32_t end = time(nullptr) + left;
    while (left > 0) {
        left = end - time(nullptr);
    }
    sleep(4);
    ASSERT_NE(stack, "");
    printf("stack:\n%s", stack.c_str());
}

/**
 * @tc.name: ThreadSamplerTest_003
 * @tc.desc: sample thread 2 times but signal is masked
 * @tc.type: FUNC
 * @tc.require
 */
HWTEST_F(ThreadSamplerTest, ThreadSamplerTest_003, TestSize.Level3)
{
    printf("ThreadSamplerTest_003\n");
    printf("Total:450MS Sample:150MS \n");
    ThreadSampler::GetInstance().Init();
    auto sampleHandler = []() {
        ThreadSampler::GetInstance().Sample();
    };

    std::string stack;
    auto collectHandler = [&stack]() {
        ThreadSampler::GetInstance().CollectStack(stack, true);
    };

    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGNAL_SAMPLE_STACK);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    Watchdog::GetInstance().RunOneShotTask("ThreadSamplerTest", sampleHandler, 150);
    Watchdog::GetInstance().RunOneShotTask("ThreadSamplerTest", sampleHandler, 300);
    Watchdog::GetInstance().RunOneShotTask("CollectHandler", collectHandler, 450);

    int32_t left = 4;
    int32_t end = time(nullptr) + left;
    while (left > 0) {
        left = end - time(nullptr);
    }
    sleep(4);
    printf("stack:\n%s", stack.c_str());
    sigdelset(&sigset, SIGNAL_SAMPLE_STACK);
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
