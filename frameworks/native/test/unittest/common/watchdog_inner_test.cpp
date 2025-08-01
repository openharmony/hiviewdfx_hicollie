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
#include <cstdio>
#include "watchdog_inner_test.h"

#define private public
#define protected public
#include "watchdog_inner.h"
#undef private
#undef protected

#include "xcollie_define.h"
#include "xcollie_utils.h"
#include "directory_ex.h"
#include "file_ex.h"
#include "event_handler.h"
#include "ffrt_inner.h"
#include "parameters.h"
#include "watchdog_inner_util_test.h"

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
    InitSeLinuxEnabled();
}

void WatchdogInnerTest::TearDown(void)
{
    CancelSeLinuxEnabled();
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
 * @tc.name: WatchdogInner thread RemoveInnerTask test
 * @tc.desc: add test case
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_TaskRemoveInnerTask_001, TestSize.Level1)
{
    WatchdogInner::GetInstance().RemoveInnerTask("WatchdogInnerTest_RemoveInnerTask_001");
    ASSERT_EQ(WatchdogInner::GetInstance().checkerQueue_.size(), 0);
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
    WatchdogTask task2("", taskFunc, delay, interval, isOneshot);
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
    uint64_t result = WatchdogInner::GetInstance().FetchNextTask(now, task);
    ASSERT_TRUE(result >= 0);
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
 * @tc.name: WatchdogInner FfrtCallback Test;
 * @tc.desc: add teatcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_FfrtCallback_002, TestSize.Level1)
{
    uint64_t taskId = 1;
    const char *taskInfo = "Queue_Schedule_Timeout";
    uint32_t delayedTaskCount = 0;
    OHOS::system::SetParameter("hiviewdfx.appfreeze.filter_bundle_name", "WatchdogInnerUnitTest");
    EXPECT_TRUE(IsProcessDebug(getprocpid()));
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
    bool devRet1 = IsDeveloperOpen();
    bool devRet2 = IsDeveloperOpen();
    bool betaRet1 = IsBetaVersion();
    bool betaRet2 = IsBetaVersion();
    EXPECT_TRUE(devRet1 == devRet2);
    EXPECT_TRUE(betaRet1 == betaRet2);
    int64_t ret1 = GetTimeStamp();
    EXPECT_TRUE(ret1 > 0);
    std::string stack = "";
    std::string heaviestStack = "";
    const char* samplePath = "libthread_sampler.z.so";
    WatchdogInner::GetInstance().threadSamplerFuncHandler_ = dlopen(samplePath, RTLD_LAZY);
    EXPECT_TRUE(WatchdogInner::GetInstance().threadSamplerFuncHandler_ != nullptr);
    WatchdogInner::GetInstance().InitThreadSamplerFuncs();
    WatchdogInner::GetInstance().CollectStack(stack, heaviestStack);
    printf("stack:\n%s", stack.c_str());
    printf("heaviestStack:\n%s", heaviestStack.c_str());
    WatchdogInner::GetInstance().Deinit();
    WatchdogInner::GetInstance().ResetThreadSamplerFuncs();
}

/**
 * @tc.name: WatchdogInner;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_002, TestSize.Level1)
{
    TimePoint endTime = std::chrono::steady_clock::now();
    int sampleInterval = SAMPLE_DEFULE_INTERVAL;
    WatchdogInner::GetInstance().StartProfileMainThread(endTime, 150, sampleInterval);
    int32_t left = 4;
    int32_t end = time(nullptr) + left;
    while (left > 0) {
        left = end - time(nullptr);
    }

    left = 10;
    end = time(nullptr) + left;
    while (left > 0) {
        left = end - time(nullptr);
    }

    WatchdogInner::GetInstance().StartProfileMainThread(endTime, 150, sampleInterval);
    left = 5;
    end = time(nullptr) + left;
    while (left > 0) {
        left = end - time(nullptr);
    }
    sleep(4);
    std::string stack = "";
    std::string heaviestStack = "";
    WatchdogInner::GetInstance().CollectStack(stack, heaviestStack);
    printf("stack:\n%s", stack.c_str());
    printf("heaviestStack:\n%s", heaviestStack.c_str());
    WatchdogInner::GetInstance().Deinit();
    WatchdogInner::GetInstance().ResetThreadSamplerFuncs();
    EXPECT_TRUE(stack.size() >= heaviestStack.size());
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
    bool writeResult = WatchdogInner::WriteStringToFile(pid, "0");
    printf("writeResult=%d\n", writeResult);
    std::string eventName = WatchdogInner::GetInstance().buissnessThreadInfo_.empty() ?
        "MAIN_THREAD_JANK" : "BUSSINESS_THREAD_JANK";
    bool ret = WatchdogInner::GetInstance().ReportMainThreadEvent(gettid(), eventName);
    printf("ReportMainThreadEvent ret=%s\n", ret ? "true" : "fasle");
    int32_t interval = 150; // test value
    WatchdogInner::GetInstance().StartTraceProfile();
    WatchdogInner::GetInstance().DumpTraceProfile(interval);
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
    bool isOverLimit = false;
    ret = WriteStackToFd(getprocpid(), path, stack, "test", isOverLimit);
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
    printf("ret=%d\n", WatchdogInner::GetInstance().ReportMainThreadEvent(gettid(),
        "BUSSINESS_THREAD_JANK"));
    WatchdogInner::GetInstance().StartTraceProfile();
    WatchdogInner::GetInstance().DumpTraceProfile(150); // test value
    FunctionOpen(nullptr, "test");
}

/**
 * @tc.name: WatchdogInner GetFfrtTaskTid test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_GetFfrtTaskTid_001, TestSize.Level1)
{
    std::string msg = "us. queue name";
    std::string str = "us. queue name [";
    auto index = msg.find(str);
    EXPECT_TRUE(index == std::string::npos);
    int32_t tid = -1;
    WatchdogInner::GetInstance().GetFfrtTaskTid(tid, msg);
    EXPECT_EQ(tid, -1);
}

/**
 * @tc.name: WatchdogInner GetFfrtTaskTid test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_GetFfrtTaskTid_002, TestSize.Level1)
{
    std::string msg = "us. queue name []";
    std::string str = "], remaining tasks count=";
    auto index = msg.find(str);
    EXPECT_TRUE(index == std::string::npos);
    int32_t tid = -1;
    WatchdogInner::GetInstance().GetFfrtTaskTid(tid, msg);
    EXPECT_EQ(tid, -1);
}

/**
 * @tc.name: WatchdogInner GetFfrtTaskTid test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_GetFfrtTaskTid_003, TestSize.Level1)
{
    std::string msg = "us. queue name [], remaining tasks count=11, worker tid "
        "12220 is running, task id 12221";
    std::string str = "], remaining tasks count=";
    auto index = msg.find(str);
    EXPECT_TRUE(index != std::string::npos);
    int32_t tid = -1;
    WatchdogInner::GetInstance().GetFfrtTaskTid(tid, msg);
    EXPECT_EQ(tid, -1);
}

/**
 * @tc.name: WatchdogInner GetFfrtTaskTid test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_GetFfrtTaskTid_004, TestSize.Level1)
{
    std::string msg = "us. queue name [WatchdogInnerTest_GetFfrtTaskTid_004], "
        "remaining tasks count=11, worker tid abc\\nWatchdogInnerTest_GetFfrtTaskTid_004 "
        " is running, task id 12221";
    std::string str = "], remaining tasks count=";
    auto index = msg.find(str);
    EXPECT_TRUE(index != std::string::npos);
    int32_t tid = -1;
    WatchdogInner::GetInstance().GetFfrtTaskTid(tid, msg);
    EXPECT_EQ(tid, -1);
}

/**
 * @tc.name: WatchdogInner GetProcessNameFromProcCmdline test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_GetProcNameFromProcCmdline_001, TestSize.Level1)
{
    std::string procName1 = GetProcessNameFromProcCmdline(getpid());
    std::string procName2 = GetProcessNameFromProcCmdline(25221); // test value
    EXPECT_TRUE(procName1 != procName2);
    std::string procName3 = GetProcessNameFromProcCmdline(getpid());
    EXPECT_TRUE(procName1 == procName3);
}

/**
 * @tc.name: WatchdogInner SendFfrtEvent test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_SendFfrtEvent_001, TestSize.Level1)
{
    EXPECT_TRUE(IsProcessDebug(getprocpid()));
    std::string faultTimeStr = "\nFault time:" + FormatTime("%Y/%m/%d-%H:%M:%S") + "\n";
    WatchdogInner::GetInstance().SendFfrtEvent("msg", "testName", "taskInfo", faultTimeStr);
}

/**
 * @tc.name: WatchdogInner LeftTimeExitProcess test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_LeftTimeExitProcess_001, TestSize.Level1)
{
    EXPECT_TRUE(IsProcessDebug(getprocpid()));
    WatchdogInner::GetInstance().LeftTimeExitProcess("msg");
}

/**
 * @tc.name: WatchdogInner KillPeerBinderProcess test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_KillPeerBinderProcess_001, TestSize.Level1)
{
    EXPECT_TRUE(IsProcessDebug(getprocpid()));
    WatchdogInner::GetInstance().KillPeerBinderProcess("msg");
}

/**
 * @tc.name: WatchdogInner InitMainLooperWatcher Test
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_InitMainLooperWatcher_001, TestSize.Level0)
{
    WatchdogInner::GetInstance().InitMainLooperWatcher(nullptr, nullptr);
    WatchdogInnerBeginFunc beginTest = InitBeginFuncTest;
    WatchdogInnerEndFunc endTest = InitEndFuncTest;
    WatchdogInner::GetInstance().InitMainLooperWatcher(&beginTest, &endTest);
    int count = 0;
    while (count < 40) {
        beginTest("Test");
        usleep(350 * 1000); // test value
        endTest("Test");
        count++;
    }
    WatchdogInner::GetInstance().traceContent_.traceState = 0;
    WatchdogInner::GetInstance().InitMainLooperWatcher(&beginTest, &endTest);
    beginTest("Test");
    sleep(2); // test value
    endTest("Test");
    WatchdogInner::GetInstance().traceContent_.traceState = 1;
    beginTest("Test");
    usleep(3500 * 1000); // test value
    endTest("Test");
    EXPECT_TRUE(WatchdogInner::GetInstance().stackContent_.reportTimes < 1);
    EXPECT_EQ(WatchdogInner::GetInstance().stackContent_.isStartSampleEnabled, true);
}

/**
 * @tc.name: WatchdogInner InitMainLooperWatcher Test
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_InitMainLooperWatcher_002, TestSize.Level0)
{
    WatchdogInner::GetInstance().InitMainLooperWatcher(nullptr, nullptr);
    WatchdogInnerBeginFunc beginTest = InitBeginFuncTest;
    WatchdogInnerEndFunc endTest = InitEndFuncTest;
    WatchdogInner::GetInstance().InitMainLooperWatcher(&beginTest, &endTest);
    WatchdogInner::GetInstance().SetScrollState(true);
    int count = 0;
    while (count < 10) {
        sleep(1); // test value
        count++;
    }
    count = 0;
    while (count < 4) {
        beginTest("Test");
        usleep(50 * 1000); // test value
        endTest("Test");
        count++;
    }
    sleep(2); // test value
    EXPECT_TRUE(WatchdogInner::GetInstance().stackContent_.scrollTimes < 1);
    EXPECT_EQ(WatchdogInner::GetInstance().stackContent_.isStartSampleEnabled, true);
}

/**
 * @tc.name: WatchdogInner SetEventConfig test;
 * @tc.desc: set log_type failed.
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_SetEventConfig_001, TestSize.Level1)
{
    /**
     * @tc.name: WatchdogInner SetEventConfig test;
     * @tc.desc: set paramsMap is null.
     * @tc.type: FUNC
     */
    std::map<std::string, std::string> paramsMap;
    int ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, -1);
    /**
     * @tc.name: WatchdogInner SetEventConfig test;
     * @tc.desc: set log_type is not a number.
     * @tc.type: FUNC
     */
    paramsMap[KEY_LOG_TYPE] = "";
    ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, -1);
    paramsMap[KEY_LOG_TYPE] = "ab0";
    ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, -1);
    /**
     * @tc.name: WatchdogInner SetEventConfig test;
     * @tc.desc: set log_type is negative number.
     * @tc.type: FUNC
     */
    paramsMap[KEY_LOG_TYPE] = "-1";
    ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, -1);
    /**
     * @tc.name: WatchdogInner SetEventConfig test;
     * @tc.desc: set log_type success.
     * @tc.type: FUNC
     */
    paramsMap[KEY_LOG_TYPE] = "0";
    ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, 0);
    /**
     * @tc.name: WatchdogInner SetEventConfig test;
     * @tc.desc: set log_type failed.
     * @tc.type: FUNC
     */
    paramsMap[KEY_LOG_TYPE] = "1";
    ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, -1);

    /**
     * @tc.name: WatchdogInner SetEventConfig test;
     * @tc.desc: set log_type failed.
     * @tc.type: FUNC
     */
    paramsMap[KEY_LOG_TYPE] = "2";
    ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, 0);

    /**
     * @tc.name: WatchdogInner SetEventConfig test;
     * @tc.desc: set log_type out of range.
     * @tc.type: FUNC
     */
    paramsMap[KEY_LOG_TYPE] = "100";
    ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name: WatchdogInner SetEventConfig test;
 * @tc.desc: set log_type is 1.
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_SetEventConfig_002, TestSize.Level1)
{
    WatchdogInner::GetInstance().InitMainLooperWatcher(nullptr, nullptr);
    WatchdogInnerBeginFunc beginTest = InitBeginFuncTest;
    WatchdogInnerEndFunc endTest = InitEndFuncTest;
    WatchdogInner::GetInstance().InitMainLooperWatcher(&beginTest, &endTest);

    std::map<std::string, std::string> paramsMap;
    paramsMap[KEY_LOG_TYPE] = "1";
    paramsMap[KEY_SAMPLE_INTERVAL] = "100";
    paramsMap[KEY_IGNORE_STARTUP_TIME] = "12";
    paramsMap[KEY_SAMPLE_COUNT] = "21";
    paramsMap[KEY_SAMPLE_REPORT_TIMES] = "3";
    int ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, 0);
    beginTest("Test");
    sleep(12); // test value
    int count = 0;
    while (count < 3) {
        beginTest("Test");
        usleep(140 * 1000); // test value
        endTest("Test");
        count++;
    }
    sleep(5);
    EXPECT_EQ(WatchdogInner::GetInstance().jankParamsMap[KEY_LOG_TYPE], 1);
    EXPECT_EQ(WatchdogInner::GetInstance().jankParamsMap[KEY_SAMPLE_INTERVAL], 100);
    EXPECT_EQ(WatchdogInner::GetInstance().jankParamsMap[KEY_IGNORE_STARTUP_TIME], 12);
    EXPECT_EQ(WatchdogInner::GetInstance().jankParamsMap[KEY_SAMPLE_COUNT], 21);
    EXPECT_TRUE(WatchdogInner::GetInstance().stackContent_.reportTimes < 3);
    printf("stackContent_.reportTimes: %d", WatchdogInner::GetInstance().stackContent_.reportTimes);
}

/**
 * @tc.name: WatchdogInner SetEventConfig test;
 * @tc.desc: set log_type is 0.
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_SetEventConfig_003, TestSize.Level1)
{
    WatchdogInner::GetInstance().InitMainLooperWatcher(nullptr, nullptr);
    WatchdogInnerBeginFunc beginTest = InitBeginFuncTest;
    WatchdogInnerEndFunc endTest = InitEndFuncTest;
    WatchdogInner::GetInstance().InitMainLooperWatcher(&beginTest, &endTest);

    std::map<std::string, std::string> paramsMap;
    paramsMap[KEY_LOG_TYPE] = "0";
    int ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, 0);
    beginTest("Test");
    sleep(11); // test value
    int count = 0;
    while (count < 2) {
        beginTest("Test");
        usleep(200 * 1000); // test value
        endTest("Test");
        count++;
    }
    sleep(5);
    EXPECT_EQ(WatchdogInner::GetInstance().jankParamsMap[KEY_LOG_TYPE], 0);
    EXPECT_EQ(WatchdogInner::GetInstance().jankParamsMap[KEY_SAMPLE_INTERVAL], SAMPLE_DEFULE_INTERVAL);
    EXPECT_EQ(WatchdogInner::GetInstance().jankParamsMap[KEY_IGNORE_STARTUP_TIME], DEFAULT_IGNORE_STARTUP_TIME);
    EXPECT_EQ(WatchdogInner::GetInstance().jankParamsMap[KEY_SAMPLE_COUNT], SAMPLE_DEFULE_COUNT);
    printf("stackContent_.reportTimes: %d", WatchdogInner::GetInstance().stackContent_.reportTimes);
}

/**
 * @tc.name: WatchdogInner SetEventConfig test;
 * @tc.desc: set log_type is 2.
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_SetEventConfig_004, TestSize.Level1)
{
    WatchdogInner::GetInstance().InitMainLooperWatcher(nullptr, nullptr);
    WatchdogInnerBeginFunc beginTest = InitBeginFuncTest;
    WatchdogInnerEndFunc endTest = InitEndFuncTest;
    WatchdogInner::GetInstance().InitMainLooperWatcher(&beginTest, &endTest);

    std::map<std::string, std::string> paramsMap;
    paramsMap[KEY_LOG_TYPE] = "2";
    int ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, 0);
    beginTest("Test");
    usleep(2000 * 1000); // test value
    endTest("Test");
    EXPECT_EQ(WatchdogInner::GetInstance().jankParamsMap[KEY_LOG_TYPE], 2);
    EXPECT_EQ(WatchdogInner::GetInstance().jankParamsMap[KEY_SAMPLE_INTERVAL], SAMPLE_DEFULE_INTERVAL);
    EXPECT_EQ(WatchdogInner::GetInstance().jankParamsMap[KEY_IGNORE_STARTUP_TIME], DEFAULT_IGNORE_STARTUP_TIME);
    EXPECT_EQ(WatchdogInner::GetInstance().jankParamsMap[KEY_SAMPLE_COUNT], SAMPLE_DEFULE_COUNT);
}

/**
 * @tc.name: WatchdogInner SetEventConfig test;
 * @tc.desc: set KEY_LOG_TYPE is 1, other parameters out of range.
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_SetEventConfig_005, TestSize.Level1)
{
    std::map<std::string, std::string> paramsMap;
    /**
     * @tc.desc: set sample interval out of range.
     */
    paramsMap[KEY_LOG_TYPE] = "1";
    paramsMap[KEY_SAMPLE_INTERVAL] = "49";
    paramsMap[KEY_IGNORE_STARTUP_TIME] = "15";
    paramsMap[KEY_SAMPLE_COUNT] = "21";
    paramsMap[KEY_SAMPLE_REPORT_TIMES] = "3";
    int ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, -1);
    /**
     * @tc.desc: set ignore startup time out of range.
     */
    paramsMap[KEY_SAMPLE_INTERVAL] = "50";
    paramsMap[KEY_IGNORE_STARTUP_TIME] = "1";
    ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, -1);
    /**
     * @tc.desc: set sample count out of range.
     */
    paramsMap[KEY_IGNORE_STARTUP_TIME] = "10";
    paramsMap[KEY_SAMPLE_COUNT] = "1000";
    ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, -1);
    /**
     * @tc.desc: set report times out of range.
     */
    paramsMap[KEY_SAMPLE_COUNT] = "10";
    paramsMap[KEY_SAMPLE_REPORT_TIMES] = "5";
    ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name: WatchdogInner SetEventConfig test;
 * @tc.desc: set param is invalid.
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_SetEventConfig_006, TestSize.Level1)
{
    std::map<std::string, std::string> paramsMap;
    /**
     * @tc.desc: set paramsMap's key is invalid.
     */
    paramsMap["ABC"] = "abc";
    int ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, -1);
    /**
     * @tc.desc: set paramMap size out of range.
     */
    paramsMap[KEY_LOG_TYPE] = "0";
    paramsMap[KEY_SAMPLE_INTERVAL] = "49";
    ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, -1);
    /**
     * @tc.desc: set report times is not a number.
     */
    paramsMap[KEY_LOG_TYPE] = "1";
    paramsMap[KEY_SAMPLE_INTERVAL] = "50";
    paramsMap[KEY_IGNORE_STARTUP_TIME] = "15";
    paramsMap[KEY_SAMPLE_COUNT] = "21";
    paramsMap[KEY_SAMPLE_REPORT_TIMES] = "abc";
    ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, -1);
    /**
     * @tc.desc: set sample count is not a number.
     */
    paramsMap[KEY_SAMPLE_COUNT] = "abc";
    ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, -1);
    /**
     * @tc.desc: set ignore startup time out of range.
     */
    paramsMap[KEY_IGNORE_STARTUP_TIME] = "abc";
    ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, -1);
    /**
     * @tc.desc: set sample interval out of range.
     */
    paramsMap[KEY_SAMPLE_INTERVAL] = "abc";
    ret = WatchdogInner::GetInstance().SetEventConfig(paramsMap);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name: WatchdogInner GetLimitedSizeName test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_GetLimitedSizeName_001, TestSize.Level1)
{
    std::string testStr = "WatchdogInnerTest_GetLimitedSizeName_001";
    std::string name = testStr;
    int limitValue = 128; // name limit value
    while (name.size() <= limitValue) {
        name += testStr;
    }
    EXPECT_TRUE(GetLimitedSizeName(name).size() <= limitValue);
}

/**
 * @tc.name: WatchdogInner IsInSleep test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_IsInSleep_001, TestSize.Level1)
{
    const std::string name = "WatchdogInnerTest_IsInSleep_001";
    bool taskResult = 0;
    XCollieCallback callbackFunc = [&taskResult](void *) {
        taskResult = 1;
    };

    WatchdogTask task(name, 0, callbackFunc, nullptr, XCOLLIE_FLAG_DEFAULT);
    bool ret = WatchdogInner::GetInstance().IsInSleep(task);
    EXPECT_TRUE(!ret);

    task.bootTimeStart = 0;
    task.monoTimeStart = 0;
    ret = WatchdogInner::GetInstance().IsInSleep(task);
    EXPECT_TRUE(!ret);

    uint64_t bootTimeStart = 0;
    uint64_t monoTimeStart = 0;
    CalculateTimes(bootTimeStart, monoTimeStart);
    uint64_t testValue = 2100;
    task.bootTimeStart = bootTimeStart;
    task.monoTimeStart = monoTimeStart + testValue;
    ret = WatchdogInner::GetInstance().IsInSleep(task);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: WatchdogInner GetAppStartTime test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_GetAppStartTime_001, TestSize.Level1)
{
    std::string testStr = "WatchdogInnerTest_GetAppStartTime_001";
    int64_t time = GetAppStartTime(-1, 0);
    EXPECT_TRUE(time != 0);
}

/**
 * @tc.name: WatchdogInner SetSpecifiedProcessName test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_SetSpecifiedProcessName_001, TestSize.Level1)
{
    std::string testStr = "WatchdogInnerTest_SetSpecifiedProcessName_001";
    WatchdogInner::GetInstance().SetSpecifiedProcessName(testStr);
    EXPECT_EQ(WatchdogInner::GetInstance().GetSpecifiedProcessName(), testStr);
}

/**
 * @tc.name: WatchdogInner SetScrollState test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_SetScrollState_001, TestSize.Level1)
{
    WatchdogInner::GetInstance().SetScrollState(true);
    EXPECT_EQ(WatchdogInner::GetInstance().isScroll_, true);
}

/**
 * @tc.name: WatchdogInner UpdateReportTimes Test;
 * @tc.desc: add teatcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_UpdateReportTimes_002, TestSize.Level1)
{
    uint64_t taskId = 1;
    const char *taskInfo = "Queue_Schedule_Timeout";
    uint32_t delayedTaskCount = 0;
    OHOS::system::SetParameter("persist.hiview.jank.reporttimes",
        "WatchdogInnerUnitTest:120;com.sample.test:60");
    int32_t checkInterval = 0;
    int32_t times = 0;
    std::string bundleName = "test";
    UpdateReportTimes(bundleName, times, checkInterval);
    EXPECT_TRUE(times == 0);
    bundleName = "WatchdogInnerUnitTest";
    UpdateReportTimes(bundleName, times, checkInterval);
    EXPECT_TRUE(times == 2);
}

/**
 * @tc.name: WatchdogInner ClearOldFiles Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_ClearOldFiles_001, TestSize.Level1)
{
    std::string dir = "/data/storage/el2/log/watchdog/";
    std::vector<FileInfo> fileList;
    GetFilesByDir(fileList, dir);
    int deleteCount = ClearOldFiles(fileList);
    EXPECT_TRUE(deleteCount >= 0);
    printf("deleteCount: %d\n", deleteCount);
    dir = "/data/storage/el2/log/watchdog";
    GetFilesByDir(fileList, dir);
    deleteCount = ClearOldFiles(fileList);
    EXPECT_TRUE(deleteCount > 0);
}

/**
 * @tc.name: WatchdogInner SaveStringToFile Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_SaveStringToFile_001, TestSize.Level1)
{
    std::string filePath = "/";
    std::string content = "WatchdogInnerTest_SaveStringToFile_001";
    SaveStringToFile(filePath, content);
    filePath = "/data/local/tmp/test.txt";
    bool result = SaveStringToFile(filePath, content);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: WatchdogInner StartSample Test;
 * @tc.desc: add testcase
 * @tc.type: FUNC
 */
HWTEST_F(WatchdogInnerTest, WatchdogInnerTest_StartSample_001, TestSize.Level1)
{
    std::string path = "test.txt";
    WatchdogInner::GetInstance().SaveFreezeStackToFile(path, getpid());
    std::string outFile;
    WatchdogInner::GetInstance().StartSample(-3000, -300, outFile);
    EXPECT_TRUE(outFile.empty());
    WatchdogInner::GetInstance().StartSample(3000, 300, outFile);
    int count = 0;
    while (count < 6) {
        sleep(1);
        count++;
    }
    EXPECT_TRUE(!outFile.empty());
}
} // namespace HiviewDFX
} // namespace OHOS
