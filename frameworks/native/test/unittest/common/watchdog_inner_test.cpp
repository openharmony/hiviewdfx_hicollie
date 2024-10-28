/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include <thread>
#include <fstream>
#include <fcntl.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <chrono>
#include "watchdog_inner_test.h"

#define private public
#define protected public
#include "watchdog_inner.h"
#undef private
#undef protected

#include "xcollie_utils.h"
#include "directory_ex.h"
#include "file_ex.h"
#include "event_handler.h"
#include "ffrt_inner.h"

using namespace testing::ext;
using namespace OHOS::AppExecFwk;
namespace OHOS {
namespace HiviewDFX {
void WatchdogInnerTest::SetUpTestCase(void)
{
}

void WatchdogInnerTest::TearDownTestCase(void)
{
}

void WatchdogInnerTest::SetUp(void)
{
}

void WatchdogInnerTest::TearDown(void)
{
}

void InitSeLinuxEnabled()
{
    bool isSelinuxEnabled = false;
    constexpr uint32_t BUF_SIZE_64 = 64;
    char buffer[BUF_SIZE_64] = {'\0'};
    FILE* fp = popen("getenforce", "r");
    if (fp != nullptr) {
        fgets(buffer, sizeof(buffer), fp);
        std::string str = buffer;
        printf("buffer is %s\n", str.c_str());
        if (str.find("Enforcing") != str.npos) {
            printf("Enforcing %s\n", str.c_str());
            isSelinuxEnabled = true;
        } else {
            printf("This isn't Enforcing %s\n", str.c_str());
        }
        pclose(fp);
    } else {
        printf("fp == nullptr\n");
    }
    system("setenforce 0");

    constexpr mode_t defaultLogDirMode = 0770;
    std::string path = "/data/test/log";
    if (!OHOS::FileExists(path)) {
        OHOS::ForceCreateDirectory(path);
        OHOS::ChangeModeDirectory(path, defaultLogDirMode);
    }
}

static void InitBeginFuncTest(const char* name)
{
    std::string nameStr(name);
}

static void InitEndFuncTest(const char* name)
{
    std::string nameStr(name);
}

/**
 * @tc.name: WatchdogInner thread run a oneshot task
 * @tc.desc: Verify whether the task has been executed failed
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_RunOneShotTask_001, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    WatchdogInner::GetInstance().RunOneShotTask("", taskFunc, 0);
    ASSERT_EQ(taskResult, 0);
}

/**
 * @tc.name: WatchdogInner TriggerTimerCountTask Test
 * @tc.desc: add teatcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_TriggerTimerCountTask_001, TestSize.Level1)
{
    std::string name = "WatchdogInnerTest_RunPeriodicalTask_001";
    WatchdogInner::GetInstance().TriggerTimerCountTask(name, true, "test");
    ASSERT_EQ(WatchdogInner::GetInstance().checkerQueue_.size(), 0);
}

/**
 * @tc.name: WatchdogInner thread run a periodical task
 * @tc.desc: Verify whether the task has been executed failed
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_RunPeriodicalTask_001, TestSize.Level1)
{
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    std::string name = "RunPeriodicalTask_001";
    WatchdogInner::GetInstance().RunPeriodicalTask(name, taskFunc, 2000, 0);
    ASSERT_TRUE(WatchdogInner::GetInstance().checkerQueue_.size() > 0);
    WatchdogInner::GetInstance().TriggerTimerCountTask(name, false, "test");
    WatchdogInner::GetInstance().TriggerTimerCountTask(name, true, "test");
    ASSERT_EQ(taskResult, 0);
    size_t beforeSize = WatchdogInner::GetInstance().taskNameSet_.size();
    WatchdogInner::GetInstance().RemoveInnerTask(name);
    size_t afterSize = WatchdogInner::GetInstance().taskNameSet_.size();
    ASSERT_EQ(beforeSize, (afterSize + 1));
    WatchdogInner::GetInstance().RunPeriodicalTask("", taskFunc, 2000, 0);
    WatchdogInner::GetInstance().SetTimerCountTask("", 1, 1);
    WatchdogInner::GetInstance().RemoveInnerTask("");
}

/**
 * @tc.name: WatchdogInner fetch next task;
 * @tc.desc: Verify whether the task acquisition failed
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_FetchNextTask_001, TestSize.Level1)
{
    uint64_t now = GetCurrentTickMillseconds();
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    const std::string name = "task1";
    uint64_t delay = 0;
    uint64_t interval = 0;
    bool isOneshot = true;
    WatchdogTask task(name, taskFunc, delay, interval, isOneshot);
    int id = WatchdogInner::GetInstance().InsertWatchdogTaskLocked(name, WatchdogTask(name, taskFunc,
        delay, interval, isOneshot));
    ASSERT_GT(id, 0);
    WatchdogInner::GetInstance().isNeedStop_.store(true);
    ASSERT_EQ(WatchdogInner::GetInstance().FetchNextTask(now, task), 60000);
    WatchdogInner::GetInstance().isNeedStop_.store(false);
    WatchdogTask task1;
    ASSERT_EQ(WatchdogInner::GetInstance().FetchNextTask(now, task1), 60000);
}

/**
 * @tc.name: WatchdogInner fetch next task;
 * @tc.desc: Verify whether the task acquisition successfully
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_FetchNextTask_002, TestSize.Level1)
{
    uint64_t now = GetCurrentTickMillseconds() + 6500;
    int taskResult = 0;
    auto taskFunc = [&taskResult]() { taskResult = 1; };
    const std::string name = "FetchNextTask_002";
    uint64_t delay = 0;
    uint64_t interval = 0;
    bool isOneshot = true;
    WatchdogTask task(name, taskFunc, delay, interval, isOneshot);
    int id = WatchdogInner::GetInstance().InsertWatchdogTaskLocked(name, WatchdogTask(name, taskFunc,
        delay, interval, isOneshot));
    ASSERT_GT(id, 0);
    ASSERT_EQ(WatchdogInner::GetInstance().FetchNextTask(now, task), 0);
}

/**
 * @tc.name: WatchdogInner is exceedMaxTaskLocked;
 * @tc.desc: Verify whether checkerQueue_ is over MAX_WATCH_NUM;
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_IsExceedMaxTaskLocked_001, TestSize.Level1)
{
    bool ret = WatchdogInner::GetInstance().IsExceedMaxTaskLocked();
    ASSERT_EQ(ret, false);
}

/**
 * @tc.name: WatchdogInner FfrtCallback Test;
 * @tc.desc: add teatcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_FfrtCallback_001, TestSize.Level1)
{
    uint64_t taskId = 1;
    const char *taskInfo = "task";
    uint32_t delayedTaskCount = 0;
    ASSERT_TRUE(WatchdogInner::GetInstance().taskIdCnt.empty());
    WatchdogInner::GetInstance().FfrtCallback(taskId, taskInfo, delayedTaskCount);
}

/**
 * @tc.name: WatchdogInner SendMsgToHungtask Test;
 * @tc.desc: add teatcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_SendMsgToHungtask_001, TestSize.Level1)
{
    bool ret =WatchdogInner::GetInstance().SendMsgToHungtask(
        "WatchdogInnerTest_SendMsgToHungtask_001");
    ASSERT_FALSE(ret);
}

/**
 * @tc.name: WatchdogInner
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_KillProcessTest, TestSize.Level1)
{
    int32_t pid = 12000; // test value
    bool ret = KillProcessByPid(pid);
    EXPECT_EQ(ret, false);
    ret = IsProcessDebug(pid);
    printf("IsProcessDebug ret=%s", ret ? "true" : "false");
    InitSeLinuxEnabled();
    std::string path = "/data/test/log/test1.txt";
    std::ofstream ofs(path, std::ios::trunc);
    if (!ofs.is_open()) {
        printf("open path failed!, path=%s\n", path.c_str());
        FAIL();
    }
    ofs << "aync 1:1 to 2:2 code 9 wait:4 s test" << std::endl;
    ofs << "12000:12000 to 12001:12001 code 9 wait:1 s test" << std::endl;
    ofs << "22000:22000 to 12001:12001 code 9 wait:1 s test" << std::endl;
    ofs << "12000:12000 to 12001:12001 code 9 wait:4 s test" << std::endl;
    ofs.close();

    std::ifstream fin(path);
    if (!fin.is_open()) {
        printf("open path failed!, path=%s\n", path.c_str());
        FAIL();
    }
    int result = ParsePeerBinderPid(fin, pid);
    fin.close();
    EXPECT_TRUE(result > 0);

    path = "/data/test/log/test2.txt";
    ofs.open(path.c_str(), std::ios::trunc);
    if (!ofs.is_open()) {
        printf("open path failed!, path=%s\n", path.c_str());
        FAIL();
    }
    ofs << "context" << std::endl;
    ofs.close();
    fin.open(path.c_str());
    if (!fin.is_open()) {
        printf("open path failed!, path=%s\n", path.c_str());
        FAIL();
    }
    result = ParsePeerBinderPid(fin, pid);
    fin.close();
    EXPECT_TRUE(result < 0);
}

/**
 * @tc.name: WatchdogInner;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_001, TestSize.Level1)
{
    std::string result = GetFormatDate();
    printf("GetFormatDate:%s\n", result.c_str());
    EXPECT_TRUE(!result.empty());
    bool ret = IsEnableVersion();
    printf("ret:%s\n", ret ? "true" : "false");
    bool devRet1 = IsDeveloperOpen();
    bool devRet2 = IsDeveloperOpen();
    bool betaRet1 = IsBetaVersion();
    bool betaRet2 = IsBetaVersion();
    EXPECT_TRUE(devRet1 == devRet2);
    EXPECT_TRUE(betaRet1 == betaRet2);
    int64_t ret1 = GetTimeStamp();
    EXPECT_TRUE(ret1 > 0);
    std::string stack = "";
    const char* samplePath = "libthread_sampler.z.so";
    WatchdogInner::GetInstance().funcHandler_ = dlopen(samplePath, RTLD_LAZY);
    EXPECT_TRUE(WatchdogInner::GetInstance().funcHandler_ != nullptr);
    WatchdogInner::GetInstance().CollectStack(stack);
    printf("stack:\n%s", stack.c_str());
    WatchdogInner::GetInstance().Deinit();
}

/**
 * @tc.name: WatchdogInner;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_002, TestSize.Level1)
{
    int testValue = 150; // test value

    int32_t ret = WatchdogInner::GetInstance().StartProfileMainThread(testValue);
    int32_t left = 4;
    int32_t end = time(nullptr) + left;
    while (left > 0) {
        left = end - time(nullptr);
    }
    EXPECT_EQ(ret, -1);

    left = 10;
    end = time(nullptr) + left;
    while (left > 0) {
        left = end - time(nullptr);
    }

    ret = WatchdogInner::GetInstance().StartProfileMainThread(testValue);
    left = 5;
    end = time(nullptr) + left;
    while (left > 0) {
        left = end - time(nullptr);
    }
    sleep(4);
    EXPECT_EQ(ret, 0);
    std::string stack = "";
    WatchdogInner::GetInstance().CollectStack(stack);
    printf("stack:\n%s", stack.c_str());
    WatchdogInner::GetInstance().Deinit();
    WatchdogInner::GetInstance().CollectTrace();
}

/**
 * @tc.name: WatchdogInner;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_003, TestSize.Level1)
{
    auto timeOutCallback = [](const std::string &name, int waitState) {
        printf("timeOutCallback name is %s, waitState is %d\n", name.c_str(), waitState);
    };
    int result = WatchdogInner::GetInstance().AddThread("AddThread", nullptr, timeOutCallback, 10);
    EXPECT_TRUE(result <= 0);
    int32_t pid = getprocpid();
    WatchdogInner::WriteStringToFile(pid, "0");
    bool ret = WatchdogInner::GetInstance().CheckEventTimer(GetTimeStamp());
    printf("CheckEventTimer ret=%s\n", ret ? "true" : "fasle");
    ret = WatchdogInner::GetInstance().ReportMainThreadEvent();
    printf("ReportMainThreadEvent ret=%s\n", ret ? "true" : "fasle");
    int state = 1; // test value
    WatchdogInner::GetInstance().ChangeState(state, 1);
    int32_t interval = 150; // test value
    WatchdogInner::GetInstance().StartTraceProfile(interval);
    ret = IsFileNameFormat('1');
    EXPECT_TRUE(!ret);
    ret = IsFileNameFormat('b');
    EXPECT_TRUE(!ret);
    ret = IsFileNameFormat('B');
    EXPECT_TRUE(!ret);
    ret = IsFileNameFormat('_');
    EXPECT_TRUE(!ret);
    ret = IsFileNameFormat('*');
    EXPECT_TRUE(ret);
    std::string path = "";
    std::string stack = "STACK";
    ret = WriteStackToFd(getprocpid(), path, stack, "test");
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: WatchdogInner Test
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_004, TestSize.Level1)
{
    WatchdogInner::GetInstance().buissnessThreadInfo_.insert(getproctid());
    EXPECT_TRUE(WatchdogInner::GetInstance().buissnessThreadInfo_.size() > 0);
    printf("ret=%d\n", WatchdogInner::GetInstance().ReportMainThreadEvent());
    WatchdogInner::GetInstance().timeContent_.reportBegin = GetTimeStamp();
    WatchdogInner::GetInstance().timeContent_.reportEnd = GetTimeStamp();
    sleep(2);
    WatchdogInner::GetInstance().timeContent_.curBegin = GetTimeStamp();
    WatchdogInner::GetInstance().timeContent_.curEnd = GetTimeStamp();
    EXPECT_TRUE(WatchdogInner::GetInstance().timeContent_.reportBegin !=
        WatchdogInner::GetInstance().timeContent_.curBegin);
    int state = 1; // test value
    TimePoint currenTime = std::chrono::steady_clock::now();
    TimePoint lastEndTime = std::chrono::steady_clock::now();
    WatchdogInner::GetInstance().DayChecker(state, currenTime, lastEndTime, 2);
    WatchdogInner::GetInstance().DayChecker(state, currenTime, lastEndTime, 0);
    EXPECT_EQ(state, 0);
    WatchdogInner::GetInstance().StartTraceProfile(150); // test value
    FunctionOpen(nullptr, "test");
}

/**
 * @tc.name: WatchdogInner InitMainLooperWatcher Test
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_InitMainLooperWatcher_001, TestSize.Level1)
{
    WatchdogInner::GetInstance().InitMainLooperWatcher(nullptr, nullptr);
    WatchdogInnerBeginFunc beginTest = InitBeginFuncTest;
    WatchdogInnerEndFunc endTest = InitEndFuncTest;
    WatchdogInner::GetInstance().stackContent_.stackState == 0;
    WatchdogInner::GetInstance().InitMainLooperWatcher(&beginTest, &endTest);
    int count = 0;
    sleep(10); // test value
    while (count < 2) {
        beginTest("Test");
        usleep(350 * 1000); // test value
        endTest("Test");
        count++;
    }
    sleep(5);
    WatchdogInner::GetInstance().traceContent_.traceState == 0;
    WatchdogInner::GetInstance().InitMainLooperWatcher(&beginTest, &endTest);
    beginTest("Test");
    sleep(2); // test value
    endTest("Test");
    ASSERT_EQ(WatchdogInner::GetInstance().stackContent_.stackState, 1);
    WatchdogInner::GetInstance().traceContent_.traceState == 1;
    WatchdogInner::GetInstance().stackContent_.stackState == 0;
    beginTest("Test");
    usleep(250 * 1000); // test value
    endTest("Test");
    WatchdogInner::GetInstance().traceCollector_ == nullptr;
    WatchdogInner::GetInstance().StartTraceProfile(150);
}
} // namespace HiviewDFX
} // namespace OHOS
