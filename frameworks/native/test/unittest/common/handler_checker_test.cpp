/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "handler_checker_test.h"

#include <atomic>
#include <thread>

#include "handler_checker.h"
#include "event_handler.h"
using namespace testing::ext;
using namespace OHOS::AppExecFwk;
namespace OHOS {
namespace HiviewDFX {
namespace {
class TestEventHandler : public EventHandler {
public:
    explicit TestEventHandler(const std::shared_ptr<EventRunner> &runner)
        : EventHandler(runner) {};
    ~TestEventHandler() {};
};
}

void HandlerCheckerTest::SetUpTestCase(void)
{
    printf("SetUpTestCase.\n");
}

void HandlerCheckerTest::TearDownTestCase(void)
{
    printf("TearDownTestCase.\n");
}

void HandlerCheckerTest::SetUp(void)
{
    printf("SetUp.\n");
}

void HandlerCheckerTest::TearDown(void)
{
    printf("TearDown.\n");
}

/**
 * @tc.name: HandlerCheckerTest
 * @tc.desc: code branch coverage
 * @tc.type: FUNC
 * @tc.require: SR000CPN2F AR000CTAMB
 */
HWTEST_F(HandlerCheckerTest, HandlerCheckerTest_001, TestSize.Level3)
{
    std::atomic<bool> flage = false;
    auto blockFunc = [&flage]() {
        printf("before block 30s in %d\n", gettid());
        sleep(3); // sleep 3s
        printf("after block 30s in %d\n", gettid());
        flage = true;
    };
    auto runner = EventRunner::Create(true);
    auto handler = std::make_shared<TestEventHandler>(runner);
    HandlerChecker handlerChecker("HandlerCheckerTest_001", handler);
    bool ret = handler->PostTask(blockFunc, "Block3", 0, EventQueue::Priority::IMMEDIATE);
    ASSERT_EQ(ret, true);
    handlerChecker.ScheduleCheck();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ASSERT_EQ(handlerChecker.GetCheckState(), CheckStatus::WAITED_HALF);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ASSERT_EQ(handlerChecker.GetCheckState(), CheckStatus::WAITING);
    for (int i = 0; !flage; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (i > 5) {
            FAIL();
        }
    }
    ASSERT_EQ(handlerChecker.GetCheckState(), CheckStatus::COMPLETED);
}

/**
 * @tc.name: HandlerCheckerTest_002
 * @tc.desc: code branch coverage
 * @tc.type: FUNC
 * @tc.require: SR000CPN2F AR000CTAMB
 */
HWTEST_F(HandlerCheckerTest, HandlerCheckerTest_002, TestSize.Level3)
{
    auto runner = EventRunner::Create(true);
    auto handler = std::make_shared<TestEventHandler>(runner);
    HandlerChecker handlerChecker("HandlerCheckerTest_002", handler);

    std::string ret = handlerChecker.GetDumpInfo();
    if (ret.find("EventHandler") == std::string::npos) {
        FAIL();
    }
    ASSERT_NE(handlerChecker.GetHandler(), nullptr);
}

/**
 * @tc.name: HandlerCheckerTest_003
 * @tc.desc: code branch coverage
 * @tc.type: FUNC
 * @tc.require: SR000CPN2F AR000CTAMB
 */
HWTEST_F(HandlerCheckerTest, HandlerCheckerTest_003, TestSize.Level3)
{
    HandlerChecker handlerChecker("HandlerCheckerTest_003", nullptr);

    std::string ret = handlerChecker.GetDumpInfo();
    ASSERT_EQ(ret.empty(), true);
}

/**
 * @tc.name: HandlerCheckerTest_004
 * @tc.desc: code branch coverage
 * @tc.type: FUNC
 * @tc.require: SR000CPN2F AR000CTAMB
 */
HWTEST_F(HandlerCheckerTest, HandlerCheckerTest_004, TestSize.Level3)
{
    auto runner = EventRunner::Create(true);
    auto handler = std::make_shared<TestEventHandler>(runner);
    HandlerDumper handlerDumper;
    handler->Dump(handlerDumper);
    std::string ret = handlerDumper.GetDumpInfo();
    if (ret.find("EventHandler") == std::string::npos) {
        FAIL();
    }
    
    ASSERT_EQ(handlerDumper.GetTag(), "");
}
} // end of namespace HiviewDFX
} // end of namespace OHOS