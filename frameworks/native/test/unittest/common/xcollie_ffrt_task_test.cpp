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

#include <gtest/gtest.h>
#include <string>
#include <chrono>

#define private public
#define protected public
#include "xcollie_ffrt_task.h"
#undef private
#undef protected

#include "xcollie_utils.h"

using namespace testing::ext;

namespace OHOS {
namespace HiviewDFX {
class XCollieFfrtTaskTest : public testing::Test {
public:
    std::shared_ptr<XCollieFfrtTask> xcollieFfrtTask;
    XCollieFfrtTaskTest()
    {}
    ~XCollieFfrtTaskTest()
    {}
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void XCollieFfrtTaskTest::SetUpTestCase(void)
{
}

void XCollieFfrtTaskTest::TearDownTestCase(void)
{
}

void XCollieFfrtTaskTest::SetUp(void)
{
}

void XCollieFfrtTaskTest::TearDown(void)
{
}

/**
 * @tc.name: SubmitStartTraceTask test
 * @tc.desc: add test case
 * @tc.type: FUNC
 */
HWTEST_F(XCollieFfrtTaskTest, XCollieFfrtTaskTest_SubmitStartTraceTask_001, TestSize.Level1)
{
    std::shared_ptr<XCollieFfrtTask> xcollieFfrtTask = std::make_shared<XCollieFfrtTask>(1);
    EXPECT_TRUE(xcollieFfrtTask);
    UCollectClient::AppCaller appCaller;
    std::string testValue = "XCollieFfrtTaskTest_SubmitStartTraceTask_001";
    appCaller.bundleName = testValue;
    appCaller.bundleVersion = testValue;
    appCaller.uid = static_cast<int64_t>(getuid());
    appCaller.pid = getprocpid();
    appCaller.threadName = testValue;
    appCaller.foreground = false;
    appCaller.happenTime = 123; // test value
    appCaller.beginTime = 345; // test value
    appCaller.endTime = 456; // test value
    appCaller.actionId = UCollectClient::ACTION_ID_START_TRACE; // test value
    std::shared_ptr<UCollectClient::TraceCollector> traceCollector = UCollectClient::TraceCollector::Create();
    EXPECT_TRUE(!xcollieFfrtTask->xcollieQueue_);
    xcollieFfrtTask->SubmitStartTraceTask(appCaller, traceCollector);
    xcollieFfrtTask->InitFfrtTask();
    xcollieFfrtTask->SubmitStartTraceTask(appCaller, traceCollector);
}

/**
 * @tc.name: SubmitDumpTraceTask test
 * @tc.desc: add test case
 * @tc.type: FUNC
 */
HWTEST_F(XCollieFfrtTaskTest, XCollieFfrtTaskTest_SubmitDumpTraceTask_001, TestSize.Level1)
{
    std::shared_ptr<XCollieFfrtTask> xcollieFfrtTask = std::make_shared<XCollieFfrtTask>(1);
    EXPECT_TRUE(xcollieFfrtTask);
    UCollectClient::AppCaller appCaller;
    std::string testValue = "XCollieFfrtTaskTest_SubmitDumpTraceTask_001";
    appCaller.bundleName = testValue;
    appCaller.bundleVersion = testValue;
    appCaller.uid = static_cast<int64_t>(getuid());
    appCaller.pid = getprocpid();
    appCaller.threadName = testValue;
    appCaller.foreground = false;
    appCaller.happenTime = 123; // test value
    appCaller.beginTime = 345; // test value
    appCaller.endTime = 456; // test value
    appCaller.actionId = UCollectClient::ACTION_ID_DUMP_TRACE; // test value
    std::shared_ptr<UCollectClient::TraceCollector> traceCollector = UCollectClient::TraceCollector::Create();
    EXPECT_TRUE(xcollieFfrtTask);
    xcollieFfrtTask->SubmitDumpTraceTask(appCaller, traceCollector);
    xcollieFfrtTask->InitFfrtTask();
    xcollieFfrtTask->SubmitDumpTraceTask(appCaller, traceCollector);
}

/**
 * @tc.name: StartTrace test
 * @tc.desc: add test case
 * @tc.type: FUNC
 */
HWTEST_F(XCollieFfrtTaskTest, XCollieFfrtTaskTest_StartTrace_001, TestSize.Level1)
{
    UCollectClient::AppCaller appCaller;
    std::string testValue = "XCollieFfrtTaskTest_StartTrace_001";
    appCaller.bundleName = testValue;
    appCaller.bundleVersion = testValue;
    appCaller.uid = static_cast<int64_t>(getuid());
    appCaller.pid = getprocpid();
    appCaller.threadName = testValue;
    appCaller.foreground = false;
    appCaller.happenTime = 111; // test value
    appCaller.beginTime = 1234; // test value
    appCaller.endTime = 2234; // test value
    appCaller.actionId = UCollectClient::ACTION_ID_START_TRACE; // test value
    std::shared_ptr<UCollectClient::TraceCollector> traceCollector = UCollectClient::TraceCollector::Create();
    std::shared_ptr<XCollieFfrtTask> xcollieFfrtTask = std::make_shared<XCollieFfrtTask>(1);
    EXPECT_TRUE(xcollieFfrtTask);
    xcollieFfrtTask->InitFfrtTask();
    xcollieFfrtTask->StartTrace(appCaller, traceCollector);
}

/**
 * @tc.name: DumpTrace test
 * @tc.desc: add test case
 * @tc.type: FUNC
 */
HWTEST_F(XCollieFfrtTaskTest, XCollieFfrtTaskTest_DumpTrace_001, TestSize.Level1)
{
    UCollectClient::AppCaller appCaller;
    std::string testValue = "XCollieFfrtTaskTest_DumpTrace_001";
    appCaller.bundleName = testValue;
    appCaller.bundleVersion = testValue;
    appCaller.uid = static_cast<int64_t>(getuid());
    appCaller.pid = getprocpid();
    appCaller.threadName = testValue;
    appCaller.foreground = false;
    appCaller.happenTime = 111; // test value
    appCaller.beginTime = 1234; // test value
    appCaller.endTime = 2234; // test value
    appCaller.actionId = UCollectClient::ACTION_ID_DUMP_TRACE; // test value
    std::shared_ptr<UCollectClient::TraceCollector> traceCollector = UCollectClient::TraceCollector::Create();
    std::shared_ptr<XCollieFfrtTask> xcollieFfrtTask = std::make_shared<XCollieFfrtTask>(1);
    EXPECT_TRUE(xcollieFfrtTask);
    xcollieFfrtTask->DumpTrace(appCaller, traceCollector);
    EXPECT_TRUE(xcollieFfrtTask->openTraceCode_ != 0);
    xcollieFfrtTask->openTraceCode_ = 0;
    xcollieFfrtTask->InitFfrtTask();
    xcollieFfrtTask->DumpTrace(appCaller, traceCollector);
}
} // namespace HiviewDFX
} // namespace OHOS
