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
#include <fstream>
#include <sstream>

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
    Watchdog::GetInstance().RunOneShotTask("CollectStackTest", collectHandler, 450);

    int32_t left = 4;
    int32_t end = time(nullptr) + left;
    while (left > 0) {
        left = end - time(nullptr);
    }
    sleep(4);
    ASSERT_NE(stack, "");
    printf("stack:\n%s", stack.c_str());
    ThreadSampler::GetInstance().Deinit();
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
    Watchdog::GetInstance().RunOneShotTask("CollectStackTest", collectHandler, 450);

    int32_t left = 4;
    int32_t end = time(nullptr) + left;
    while (left > 0) {
        left = end - time(nullptr);
    }
    sleep(4);
    ASSERT_NE(stack, "");
    printf("stack:\n%s", stack.c_str());
    ThreadSampler::GetInstance().Deinit();
}

/**
 * @tc.name: ThreadSamplerTest_003
 * @tc.desc: sample thread 2 times but signal is blocked
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
    sigaddset(&sigset, MUSL_SIGNAL_SAMPLE_STACK);
    sigprocmask(SIG_BLOCK, &sigset, nullptr);

    Watchdog::GetInstance().RunOneShotTask("ThreadSamplerTest", sampleHandler, 150);
    Watchdog::GetInstance().RunOneShotTask("ThreadSamplerTest", sampleHandler, 300);
    Watchdog::GetInstance().RunOneShotTask("CollectStackTest", collectHandler, 450);

    int32_t left = 4;
    int32_t end = time(nullptr) + left;
    while (left > 0) {
        left = end - time(nullptr);
    }
    sleep(4);
    printf("stack:\n%s", stack.c_str());
    sigprocmask(SIG_UNBLOCK, &sigset, nullptr);
    sigdelset(&sigset, MUSL_SIGNAL_SAMPLE_STACK);
    ThreadSampler::GetInstance().Deinit();
}

/**
 * @tc.name: ThreadSamplerTest_004
 * @tc.desc: sample thread 2 times and deinit sampler send 2 sample requestion and restart sampler.
 * @tc.type: FUNC
 * @tc.require
 */
HWTEST_F(ThreadSamplerTest, ThreadSamplerTest_004, TestSize.Level3)
{
    printf("ThreadSamplerTest_004\n");
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
    Watchdog::GetInstance().RunOneShotTask("CollectStackTest", collectHandler, 450);

    int32_t left = 4;
    int32_t end = time(nullptr) + left;
    while (left > 0) {
        left = end - time(nullptr);
    }
    sleep(4);
    ASSERT_NE(stack, "");
    printf("stack:\n%s", stack.c_str());

    ThreadSampler::GetInstance().Deinit();
    Watchdog::GetInstance().RunOneShotTask("ThreadSamplerTest", sampleHandler, 150);
    Watchdog::GetInstance().RunOneShotTask("ThreadSamplerTest", sampleHandler, 300);
    Watchdog::GetInstance().RunOneShotTask("CollectStackTest", collectHandler, 450);
    
    left = 4;
    end = time(nullptr) + left;
    while (left > 0) {
        left = end - time(nullptr);
    }
    sleep(4);
    stack.clear();
    printf("stack:\n%s", stack.c_str());

    ThreadSampler::GetInstance().Init();
    Watchdog::GetInstance().RunOneShotTask("ThreadSamplerTest", sampleHandler, 150);
    Watchdog::GetInstance().RunOneShotTask("ThreadSamplerTest", sampleHandler, 300);
    Watchdog::GetInstance().RunOneShotTask("CollectStackTest", collectHandler, 450);

    left = 4;
    end = time(nullptr) + left;
    while (left > 0) {
        left = end - time(nullptr);
    }
    sleep(4);
    ASSERT_NE(stack, "");
    printf("stack:\n%s", stack.c_str());
    ThreadSampler::GetInstance().Deinit();
}

/**
 * @tc.name: ThreadSamplerTest_005
 * @tc.desc: Check the size and name of uniqueStackTable mmap.
 * @tc.type: FUNC
 * @tc.require
 */
HWTEST_F(ThreadSamplerTest, ThreadSamplerTest_005, TestSize.Level3)
{
    printf("ThreadSamplerTest_005\n");

    auto GetMMapSizeAndName = [](const std::string& checkName, std::string& mmapName) -> uint32_t {
        uint64_t size = 0;
        mmapName = "";
        std::ifstream mapsFile("/proc/self/maps");
        std::string line;
        while (getline(mapsFile, line)) {
            std::istringstream iss(line);
            std::string addrs, permissions, offset, devices, inode, pathname;
            iss >> addrs >> permissions >> offset >> devices >> inode >> pathname;
            if (pathname.find(checkName) != std::string::npos) {
                std::string start = addrs.substr(0, addrs.find('-'));
                std::string end = addrs.substr(addrs.find('-') + 1);
                size = std::stoul(end, 0, 16) - std::stoul(start, 0, 16);
                mmapName = pathname;
            }
        }
        return static_cast<uint32_t>(size);
    };

    auto isSubStr = [](const std::string& str, const std::string& sub) {
        return str.find(sub) != std::string::npos;
    };

    uint32_t uniTableSize = 0;
    std::string uniStackTableMMapName = "";

    ThreadSampler::GetInstance().Init();
    uniTableSize = GetMMapSizeAndName(ThreadSampler::GetInstance().GetUniTableMMapName(), uniStackTableMMapName);

    ASSERT_EQ(uniTableSize, ThreadSampler::GetInstance().GetuniqueStackTableSize());
    ASSERT_EQ(isSubStr(uniStackTableMMapName, ThreadSampler::GetInstance().GetUniTableMMapName()), true);
    printf("mmap name: %s, size: %u KB\n", uniStackTableMMapName.c_str(), uniTableSize);

    ThreadSampler::GetInstance().Deinit();
    uniTableSize = GetMMapSizeAndName(ThreadSampler::GetInstance().GetUniTableMMapName(), uniStackTableMMapName);

    ASSERT_EQ(uniTableSize, 0);
    ASSERT_EQ(uniStackTableMMapName, "");

    ThreadSampler::GetInstance().Init();
    uint32_t newSize = 64 * 1024;
    std::string newName = "hicollie_test";
    ThreadSampler::GetInstance().SetUniStackTableSize(newSize);
    ThreadSampler::GetInstance().SetUniStackTableMMapName(newName);
    ASSERT_EQ(newSize, ThreadSampler::GetInstance().GetuniqueStackTableSize());
    ASSERT_EQ(newName, ThreadSampler::GetInstance().GetUniTableMMapName());

    uniTableSize = GetMMapSizeAndName(ThreadSampler::GetInstance().GetUniTableMMapName(), uniStackTableMMapName);

    ASSERT_EQ(uniTableSize, ThreadSampler::GetInstance().GetuniqueStackTableSize());
    ASSERT_EQ(isSubStr(uniStackTableMMapName, ThreadSampler::GetInstance().GetUniTableMMapName()), true);
    printf("mmap name: %s, size: %u KB\n", uniStackTableMMapName.c_str(), uniTableSize);
    ThreadSampler::GetInstance().Deinit();
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
