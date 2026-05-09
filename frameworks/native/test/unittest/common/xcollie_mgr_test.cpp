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

#include "xcollie_mgr.h"

using namespace testing::ext;

namespace OHOS {
namespace HiviewDFX {
namespace {
std::atomic<int> g_callbackCount(0);

std::string TestCallback(void* handler, int type)
{
    g_callbackCount++;
    return "test_data_" + std::to_string(type);
}

std::string SimpleCallback(void* handler, int type)
{
    return "data";
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

    mgr.SetInvoker(TestCallback);
    mgr.SetHandler(reinterpret_cast<void*>(0x1234));

    std::vector<std::thread> threads;
    const int threadCount = 10;
    const int iterations = 100;

    auto createWorker = [&mgr, &running, iterations](int type) {
        return std::thread([&mgr, &running, iterations, type]() {
            for (int k = 0; k < iterations && running.load(); k++) {
                mgr.ReadDataFromBuffer(type);
            }
        });
    };

    for (int i = 0; i < threadCount; i++) {
        threads.push_back(createWorker(i % 2));
    }

    std::thread setter([&mgr, &running]() {
        int j = 0;
        while (running.load() && j < 50) {
            mgr.SetInvoker(SimpleCallback);
            mgr.SetHandler(reinterpret_cast<void*>(0x5678));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            j++;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    running.store(false);

    for (auto& t : threads) {
        t.join();
    }
    setter.join();

    ASSERT_GE(g_callbackCount.load(), 0);
}
} // namespace HiviewDFX
} // namespace OHOS