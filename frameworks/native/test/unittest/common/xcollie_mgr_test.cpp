/*
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#include "xcollie_mgr_test.h"

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <securec.h>

#include "xcollie_mgr.h"

using namespace testing::ext;

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr uint32_t TIMEOUT_SIMULATE_MS = 2000;
constexpr uint32_t SET_INTERVAL_MS = 10;
constexpr uint32_t WAIT_BEFORE_STOP_001_MS = 500;
constexpr uint32_t SET_INTERVAL_MS_002 = 5;
constexpr uint32_t WAIT_BEFORE_STOP_002_MS = 300;
constexpr uint32_t WAIT_BEFORE_STOP_003_MS = 200;
constexpr uint32_t WAIT_BEFORE_STOP_004_MS = 100;
constexpr uint32_t TOGGLE_INTERVAL_MS = 15;
constexpr uint32_t WAIT_BEFORE_STOP_005_MS = 400;
constexpr int ALTERNATE_DIVISOR = 2;
std::atomic<int> g_callbackCount(0);

struct BufferLoopParam {
    int bufferType;
    int loopCount;
    bool countEmpty;
};

void ReadDataFromBufferLoop(XcollieMgr& mgr, std::atomic<int>& count, std::atomic<bool>& running,
    const BufferLoopParam& param)
{
    int k = 0;
    while (running.load() && k < param.loopCount) {
        std::string result = mgr.ReadDataFromBuffer(param.bufferType);
        if (result.empty() == param.countEmpty) {
            count++;
        }
        k++;
    }
}

void ReaderLoopWithBranch(XcollieMgr& mgr, std::atomic<bool>& running, int i, int iterations)
{
    int k = 0;
    while (running.load() && k < iterations) {
        if (i % ALTERNATE_DIVISOR == 0) {
            mgr.ReadDataFromBuffer(0);
        } else {
            mgr.ReadDataFromBuffer(1);
        }
        k++;
    }
}

size_t TestCallback(OH_HiCollie_Freeze_Type type, void* buffer, size_t size)
{
    g_callbackCount++;
    std::string source = "test_data_" + std::to_string(type);
    char* temp = (char*)buffer;
    int needed = snprintf_s(temp, size, size - 1, "%s", source.c_str());
    return needed;
}

size_t TestCallbackWithHandler(OH_HiCollie_Freeze_Type type, void* buffer, size_t size)
{
    g_callbackCount++;
    std::string source = "handler_valid" + std::to_string(type);
    char* temp = (char*)buffer;
    int needed = snprintf_s(temp, size, size - 1, "%s", source.c_str());
    return needed;
}

size_t SimpleCallback(OH_HiCollie_Freeze_Type type, void* buffer, size_t size)
{
    std::string source = "data" + std::to_string(type);
    char* temp = (char*)buffer;
    int needed = snprintf_s(temp, size, size - 1, "%s", source.c_str());
    return needed;
}

size_t TypeCallback(OH_HiCollie_Freeze_Type type, void* buffer, size_t size)
{
    g_callbackCount++;
    std::string source = "type_" + std::to_string(type);
    char* temp = (char*)buffer;
    int needed = snprintf_s(temp, size, size - 1, "%s", source.c_str());
    return needed;
}

size_t QuickReturnCallback(OH_HiCollie_Freeze_Type type, void* buffer, size_t size)
{
    g_callbackCount++;
    std::string source = "quick_data_" + std::to_string(type);
    char* temp = (char*)buffer;
    int needed = snprintf_s(temp, size, size - 1, "%s", source.c_str());
    return needed;
}

size_t SlowReturnCallback(OH_HiCollie_Freeze_Type type, void* buffer, size_t size)
{
    g_callbackCount++;
    std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_SIMULATE_MS));
    std::string source = "timeout_data_" + std::to_string(type);
    char* temp = (char*)buffer;
    int needed = snprintf_s(temp, size, size - 1, "%s", source.c_str());
    return needed;
}
}

void XcollieMgrTest::SetUpTestCase(void)
{
}

void XcollieMgrTest::TearDownTestCase(void)
{
}

void XcollieMgrTest::SetUp(void)
{
    g_callbackCount.store(0);
}

void XcollieMgrTest::TearDown(void)
{
}

/**
 * @tc.name: XcollieMgrConcurrencyTest
 * @tc.desc: Verify xcollie mgr concurrent SetInvoker and ReadDataFromBuffer
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(XcollieMgrTest, XcollieMgrConcurrency_001, TestSize.Level1)
{
    XcollieMgr& mgr = XcollieMgr::GetInstance();
    std::atomic<bool> running(true);

    mgr.SetHandler(TestCallback);

    std::vector<std::thread> threads;
    const int threadCount = 10;
    const int iterations = 100;

    for (int i = 0; i < threadCount; i++) {
        threads.emplace_back([&mgr, &running, i, iterations]() {
            ReaderLoopWithBranch(mgr, running, i, iterations);
        });
    }

    std::thread setter([&mgr, &running]() {
        int j = 0;
        while (running.load() && j < 50) {
            mgr.SetHandler(SimpleCallback);
            std::this_thread::sleep_for(std::chrono::milliseconds(SET_INTERVAL_MS));
            j++;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_BEFORE_STOP_001_MS));
    running.store(false);

    for (auto& t : threads) {
        t.join();
    }
    setter.join();

    ASSERT_GE(g_callbackCount.load(), 0);
}

/**
 * @tc.name: XcollieMgrConcurrencyTest
 * @tc.desc: Verify xcollie mgr concurrent SetHandler and ReadDataFromBuffer
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(XcollieMgrTest, XcollieMgrConcurrency_002, TestSize.Level1)
{
    XcollieMgr& mgr = XcollieMgr::GetInstance();
    std::atomic<bool> running(true);

    mgr.SetHandler(TestCallbackWithHandler);

    std::vector<std::thread> threads;
    const int threadCount = 8;

    for (int i = 0; i < threadCount; i++) {
        threads.emplace_back([&mgr, &running]() {
            int k = 0;
            while (running.load() && k < 50) {
                mgr.ReadDataFromBuffer(2);
                k++;
            }
        });
    }

    std::thread handlerSetter([&mgr, &running]() {
        int j = 0;
        while (running.load() && j < 100) {
            mgr.SetHandler(SimpleCallback);
            std::this_thread::sleep_for(std::chrono::milliseconds(SET_INTERVAL_MS_002));
            j++;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_BEFORE_STOP_002_MS));
    running.store(false);

    for (auto& t : threads) {
        t.join();
    }
    handlerSetter.join();

    ASSERT_GE(g_callbackCount.load(), 0);
}

/**
 * @tc.name: XcollieMgrConcurrencyTest
 * @tc.desc: Verify xcollie mgr concurrent CheckCallDuration with same type
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(XcollieMgrTest, XcollieMgrConcurrency_003, TestSize.Level1)
{
    XcollieMgr& mgr = XcollieMgr::GetInstance();
    std::atomic<int> successCount(0);
    std::atomic<bool> running(true);

    mgr.SetHandler(SimpleCallback);

    std::vector<std::thread> threads;
    const int threadCount = 20;

    for (int i = 0; i < threadCount; i++) {
        threads.emplace_back([&mgr, &successCount, &running]() {
            ReadDataFromBufferLoop(mgr, successCount, running, {3, 10, false});
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_BEFORE_STOP_003_MS));
    running.store(false);

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_GT(successCount.load(), 0);
}

/**
 * @tc.name: XcollieMgrConcurrencyTest
 * @tc.desc: Verify xcollie mgr concurrent ReadDataFromBuffer with different types
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(XcollieMgrTest, XcollieMgrConcurrency_004, TestSize.Level1)
{
    XcollieMgr& mgr = XcollieMgr::GetInstance();
    std::atomic<bool> running(true);

    mgr.SetHandler(TypeCallback);

    std::vector<std::thread> threads;
    const int threadCount = 5;

    for (int i = 0; i < threadCount; i++) {
        threads.emplace_back([&mgr, &running, i]() {
            int k = 0;
            while (running.load() && k < 20) {
                mgr.ReadDataFromBuffer(i);
                k++;
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_BEFORE_STOP_004_MS));
    running.store(false);

    for (auto& t : threads) {
        t.join();
    }

    ASSERT_GT(g_callbackCount.load(), 0);
}

/**
 * @tc.name: XcollieMgrConcurrencyTest
 * @tc.desc: Verify xcollie mgr null callback handling in concurrent scenario
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(XcollieMgrTest, XcollieMgrConcurrency_005, TestSize.Level1)
{
    XcollieMgr& mgr = XcollieMgr::GetInstance();
    std::atomic<int> emptyCount(0);
    std::atomic<bool> running(true);

    mgr.SetHandler(nullptr);

    std::vector<std::thread> threads;
    const int threadCount = 10;

    for (int i = 0; i < threadCount; i++) {
        threads.emplace_back([&mgr, &emptyCount, &running]() {
            ReadDataFromBufferLoop(mgr, emptyCount, running, {4, 30, true});
        });
    }

    std::thread toggler([&mgr, &running]() {
        int j = 0;
        while (running.load() && j < 20) {
            if (j % ALTERNATE_DIVISOR == 0) {
                mgr.SetHandler(nullptr);
            } else {
                mgr.SetHandler(SimpleCallback);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(TOGGLE_INTERVAL_MS));
            j++;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_BEFORE_STOP_005_MS));
    running.store(false);

    for (auto& t : threads) {
        t.join();
    }
    toggler.join();

    ASSERT_GT(emptyCount.load(), 0);
}

/**
 * @tc.name: XcollieMgrFutureTest
 * @tc.desc: Verify std::future callback returns quickly within timeout
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(XcollieMgrTest, XcollieMgrFuture_001, TestSize.Level1)
{
    XcollieMgr& mgr = XcollieMgr::GetInstance();

    mgr.SetHandler(QuickReturnCallback);

    std::string result = mgr.ReadDataFromBuffer(5);

    EXPECT_EQ(result, "quick_data_5");
}

/**
 * @tc.name: XcollieMgrFutureTest
 * @tc.desc: Verify std::future callback timeout handling
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(XcollieMgrTest, XcollieMgrFuture_002, TestSize.Level1)
{
    XcollieMgr& mgr = XcollieMgr::GetInstance();

    mgr.SetHandler(SlowReturnCallback);

    std::string result = mgr.ReadDataFromBuffer(6);

    EXPECT_TRUE(result.empty());
}
} // namespace HiviewDFX
} // namespace OHOS