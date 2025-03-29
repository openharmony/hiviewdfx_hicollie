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
#include <dlfcn.h>

#include "watchdog.h"

using namespace testing::ext;
namespace OHOS {
namespace HiviewDFX {
const char* LIB_THREAD_SAMPLER_PATH = "libthread_sampler.z.so";

constexpr int SAMPLE_CNT = 10;
constexpr int INTERVAL = 100;
constexpr size_t STACK_LENGTH = 32 * 1024;
constexpr int MILLSEC_TO_MICROSEC = 1000;

static std::mutex threadSamplerSignalMutex_;
SigActionType ThreadSamplerTest::threadSamplerSigHandler_ = nullptr;

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

void WaitSomeTime()
{
    int waitDelay = 3;
    int32_t left = (INTERVAL * SAMPLE_CNT + INTERVAL) / MILLSEC_TO_MICROSEC + waitDelay;
    int32_t end = time(nullptr) + left;
    while (left > 0) {
        left = end - time(nullptr);
    }
    sleep((INTERVAL * SAMPLE_CNT + INTERVAL) / MILLSEC_TO_MICROSEC + waitDelay);
}

uint32_t GetMMapSizeAndName(const std::string& checkName, std::string& mmapName)
{
    uint64_t size = 0;
    mmapName = "";
    std::ifstream mapsFile("/proc/self/maps");
    std::string line;
    int base = 16;
    while (getline(mapsFile, line)) {
        std::istringstream iss(line);
        std::string addrs;
        std::string permissions;
        std::string offset;
        std::string devices;
        std::string inode;
        std::string pathname;
        iss >> addrs >> permissions >> offset >> devices >> inode >> pathname;
        if (pathname.find(checkName) != std::string::npos) {
            std::string start = addrs.substr(0, addrs.find('-'));
            std::string end = addrs.substr(addrs.find('-') + 1);
            size = std::stoul(end, nullptr, base) - std::stoul(start, nullptr, base);
            mmapName = pathname;
        }
    }
    return static_cast<uint32_t>(size);
}

void* FunctionOpen(void* funcHandler, const char* funcName)
{
    dlerror();
    char* err = nullptr;
    void* func = dlsym(funcHandler, funcName);
    err = dlerror();
    if (err != nullptr) {
        return nullptr;
    }
    return func;
}

void ThreadSamplerTest::ThreadSamplerSigHandler(int sig, siginfo_t* si, void* context)
{
    std::lock_guard<std::mutex> lock(threadSamplerSignalMutex_);
    if (ThreadSamplerTest::threadSamplerSigHandler_ == nullptr) {
        return;
    }
    ThreadSamplerTest::threadSamplerSigHandler_(sig, si, context);
}

bool ThreadSamplerTest::InstallThreadSamplerSignal()
{
    struct sigaction action {};
    sigfillset(&action.sa_mask);
    action.sa_sigaction = ThreadSamplerTest::ThreadSamplerSigHandler;
    action.sa_flags = SA_RESTART | SA_SIGINFO;
    if (sigaction(MUSL_SIGNAL_SAMPLE_STACK, &action, nullptr) != 0) {
        return false;
    }
    return true;
}

void ThreadSamplerTest::UninstallThreadSamplerSignal()
{
    std::lock_guard<std::mutex> lock(threadSamplerSignalMutex_);
    threadSamplerSigHandler_ = nullptr;
}

bool ThreadSamplerTest::InitThreadSamplerFuncs()
{
    threadSamplerFuncHandler_ = dlopen(LIB_THREAD_SAMPLER_PATH, RTLD_LAZY);
    if (threadSamplerFuncHandler_ == nullptr) {
        return false;
    }

    threadSamplerInitFunc_ =
        reinterpret_cast<ThreadSamplerInitFunc>(FunctionOpen(threadSamplerFuncHandler_, "ThreadSamplerInit"));
    threadSamplerSampleFunc_ =
        reinterpret_cast<ThreadSamplerSampleFunc>(FunctionOpen(threadSamplerFuncHandler_, "ThreadSamplerSample"));
    threadSamplerCollectFunc_ =
        reinterpret_cast<ThreadSamplerCollectFunc>(FunctionOpen(threadSamplerFuncHandler_, "ThreadSamplerCollect"));
    threadSamplerDeinitFunc_ =
        reinterpret_cast<ThreadSamplerDeinitFunc>(FunctionOpen(threadSamplerFuncHandler_, "ThreadSamplerDeinit"));
    threadSamplerSigHandler_ =
        reinterpret_cast<SigActionType>(FunctionOpen(threadSamplerFuncHandler_, "ThreadSamplerSigHandler"));
    if (threadSamplerInitFunc_ == nullptr || threadSamplerSampleFunc_ == nullptr ||
        threadSamplerCollectFunc_ == nullptr || threadSamplerDeinitFunc_ == nullptr ||
        threadSamplerSigHandler_ == nullptr) {
        threadSamplerInitFunc_ = nullptr;
        threadSamplerSampleFunc_ = nullptr;
        threadSamplerCollectFunc_ = nullptr;
        threadSamplerDeinitFunc_ = nullptr;
        threadSamplerSigHandler_ = nullptr;
        dlclose(threadSamplerFuncHandler_);
        threadSamplerFuncHandler_ = nullptr;
        return false;
    }
    return true;
}

bool ThreadSamplerTest::InitThreadSampler()
{
    if (!InitThreadSamplerFuncs()) {
        return false;
    }

    if (!InstallThreadSamplerSignal()) {
        return false;
    }
    return true;
}

/**
 * @tc.name: ThreadSamplerTest_001
 * @tc.desc: sample thread SAMPLE_CNT times and check the stacktrace
 * @tc.type: FUNC
 * @tc.require
 */
HWTEST_F(ThreadSamplerTest, ThreadSamplerTest_001, TestSize.Level0)
{
    printf("ThreadSamplerTest_001\n");
    printf("Total:%dMS Sample:%dMS \n", INTERVAL * SAMPLE_CNT + INTERVAL, INTERVAL);

    bool flag = InitThreadSampler();
    ASSERT_TRUE(flag);

    threadSamplerInitFunc_(SAMPLE_CNT);
    auto sampleHandler = [this]() {
        threadSamplerSampleFunc_();
    };

    char* stk = new char[STACK_LENGTH];
    char* heaviestStk = new char[STACK_LENGTH];
    auto collectHandler = [this, &stk, &heaviestStk]() {
        int treeFormat = 0;
        threadSamplerCollectFunc_(stk, heaviestStk, STACK_LENGTH, STACK_LENGTH, treeFormat);
    };

    for (int i = 0; i < SAMPLE_CNT; i++) {
        uint64_t delay = INTERVAL * i + INTERVAL;
        Watchdog::GetInstance().RunOneShotTask("ThreadSamplerTest", sampleHandler, delay);
    }
    Watchdog::GetInstance().RunOneShotTask("CollectStackTest", collectHandler, INTERVAL * SAMPLE_CNT + INTERVAL);

    WaitSomeTime();

    std::string stack = stk;
    std::string heaviestStack = heaviestStk;
    ASSERT_NE(stack, "");
    printf("stack:\n%s\nheaviestStack:\n%s", stack.c_str(), heaviestStack.c_str());
    delete[] stk;
    delete[] heaviestStk;
    UninstallThreadSamplerSignal();
    threadSamplerDeinitFunc_();
    dlclose(threadSamplerFuncHandler_);
}

/**
 * @tc.name: ThreadSamplerTest_002
 * @tc.desc: sample thread SAMPLE_CNT times and check the stacktrace in tree format
 * @tc.type: FUNC
 * @tc.require
 */
HWTEST_F(ThreadSamplerTest, ThreadSamplerTest_002, TestSize.Level3)
{
    printf("ThreadSamplerTest_002\n");
    printf("Total:%dMS Sample:%dMS \n", INTERVAL * SAMPLE_CNT + INTERVAL, INTERVAL);
    
    bool flag = InitThreadSampler();
    ASSERT_TRUE(flag);

    threadSamplerInitFunc_(SAMPLE_CNT);
    auto sampleHandler = [this]() {
        threadSamplerSampleFunc_();
    };

    char* stk = new char[STACK_LENGTH];
    char* heaviestStk = new char[STACK_LENGTH];
    auto collectHandler = [this, &stk, &heaviestStk]() {
        int treeFormat = 1;
        threadSamplerCollectFunc_(stk, heaviestStk, STACK_LENGTH, STACK_LENGTH, treeFormat);
    };

    for (int i = 0; i < SAMPLE_CNT; i++) {
        uint64_t delay = INTERVAL * i + INTERVAL;
        Watchdog::GetInstance().RunOneShotTask("ThreadSamplerTest", sampleHandler, delay);
    }
    Watchdog::GetInstance().RunOneShotTask("CollectStackTest", collectHandler, INTERVAL * SAMPLE_CNT + INTERVAL);

    WaitSomeTime();

    std::string stack = stk;
    std::string heaviestStack = heaviestStk;
    ASSERT_NE(stack, "");
    printf("stack:\n%s\nheaviestStack:\n%s", stack.c_str(), heaviestStack.c_str());
    delete[] stk;
    delete[] heaviestStk;
    UninstallThreadSamplerSignal();
    threadSamplerDeinitFunc_();
    dlclose(threadSamplerFuncHandler_);
}

/**
 * @tc.name: ThreadSamplerTest_003
 * @tc.desc: sample thread SAMPLE_CNT times and deinit sampler send SAMPLE_CNT sample requestion and restart sampler.
 * @tc.type: FUNC
 * @tc.require
 */
HWTEST_F(ThreadSamplerTest, ThreadSamplerTest_003, TestSize.Level3)
{
    printf("ThreadSamplerTest_003\n");
    printf("Total:%dMS Sample:%dMS \n", INTERVAL * SAMPLE_CNT + INTERVAL, INTERVAL);

    InitThreadSampler();

    threadSamplerInitFunc_(SAMPLE_CNT);
    auto sampleHandler = [this]() {
        threadSamplerSampleFunc_();
    };

    char* stk = new char[STACK_LENGTH];
    char* heaviestStk = new char[STACK_LENGTH];
    auto collectHandler = [this, &stk, &heaviestStk]() {
        int treeFormat = 1;
        threadSamplerCollectFunc_(stk, heaviestStk, STACK_LENGTH, STACK_LENGTH, treeFormat);
    };

    for (int i = 0; i < SAMPLE_CNT; i++) {
        Watchdog::GetInstance().RunOneShotTask("ThreadSamplerTest", sampleHandler, INTERVAL * i + INTERVAL);
    }
    Watchdog::GetInstance().RunOneShotTask("CollectStackTest", collectHandler, INTERVAL * SAMPLE_CNT + INTERVAL);

    WaitSomeTime();

    std::string stack = stk;
    std::string heaviestStack = heaviestStk;
    ASSERT_NE(stack, "");
    printf("stack:\n%s\nheaviestStack:\n%s", stack.c_str(), heaviestStack.c_str());
    threadSamplerDeinitFunc_();

    for (int i = 0; i < SAMPLE_CNT; i++) {
        Watchdog::GetInstance().RunOneShotTask("ThreadSamplerTest", sampleHandler, INTERVAL * i + INTERVAL);
    }
    Watchdog::GetInstance().RunOneShotTask("CollectStackTest", collectHandler, INTERVAL * SAMPLE_CNT + INTERVAL);

    WaitSomeTime();
    stack = stk;
    heaviestStack = heaviestStk;
    ASSERT_NE(stack, "");
    printf("stack:\n%s\nheaviestStack:\n%s", stack.c_str(), heaviestStack.c_str());

    threadSamplerInitFunc_(SAMPLE_CNT);

    for (int i = 0; i < SAMPLE_CNT; i++) {
        Watchdog::GetInstance().RunOneShotTask("ThreadSamplerTest", sampleHandler, INTERVAL * i + INTERVAL);
    }
    Watchdog::GetInstance().RunOneShotTask("CollectStackTest", collectHandler, INTERVAL * SAMPLE_CNT + INTERVAL);

    WaitSomeTime();
    stack = stk;
    heaviestStack = heaviestStk;
    ASSERT_NE(stack, "");
    printf("stack:\n%s\nheaviestStack:\n%s", stack.c_str(), heaviestStack.c_str());
    delete[] stk;
    delete[] heaviestStk;
    UninstallThreadSamplerSignal();
    threadSamplerDeinitFunc_();
    dlclose(threadSamplerFuncHandler_);
}

/**
 * @tc.name: ThreadSamplerTest_004
 * @tc.desc: sample thread several times but signal is blocked.
 * @tc.type: FUNC
 * @tc.require
 */
HWTEST_F(ThreadSamplerTest, ThreadSamplerTest_004, TestSize.Level3)
{
    printf("ThreadSamplerTest_004\n");
    printf("Total:%dMS Sample:%dMS \n", INTERVAL * SAMPLE_CNT + INTERVAL, INTERVAL);

    bool flag = InitThreadSampler();
    ASSERT_TRUE(flag);

    threadSamplerInitFunc_(SAMPLE_CNT);
    auto sampleHandler = [this]() {
        threadSamplerSampleFunc_();
    };

    char* stk = new char[STACK_LENGTH];
    char* heaviestStk = new char[STACK_LENGTH];
    auto collectHandler = [this, &stk, &heaviestStk]() {
        int treeFormat = 1;
        threadSamplerCollectFunc_(stk, heaviestStk, STACK_LENGTH, STACK_LENGTH, treeFormat);
    };

    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, MUSL_SIGNAL_SAMPLE_STACK);
    sigprocmask(SIG_BLOCK, &sigset, nullptr);

    for (int i = 0; i < SAMPLE_CNT; i++) {
        uint64_t delay = INTERVAL * i + INTERVAL;
        Watchdog::GetInstance().RunOneShotTask("ThreadSamplerTest", sampleHandler, delay);
    }
    Watchdog::GetInstance().RunOneShotTask("CollectStackTest", collectHandler, INTERVAL * SAMPLE_CNT + INTERVAL);

    WaitSomeTime();

    std::string stack = stk;
    std::string heaviestStack = heaviestStk;
    ASSERT_NE(stack, "");
    printf("stack:\n%s\nheaviestStack:\n%s", stack.c_str(), heaviestStack.c_str());
    sigprocmask(SIG_UNBLOCK, &sigset, nullptr);
    sigdelset(&sigset, MUSL_SIGNAL_SAMPLE_STACK);
    delete[] stk;
    delete[] heaviestStk;
    UninstallThreadSamplerSignal();
    threadSamplerDeinitFunc_();
    dlclose(threadSamplerFuncHandler_);
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

    auto isSubStr = [](const std::string& str, const std::string& sub) {
        return str.find(sub) != std::string::npos;
    };

    uint32_t uniTableSize = 0;
    std::string uniStackTableMMapName = "";

    bool flag = InitThreadSamplerFuncs();
    ASSERT_TRUE(flag);

    threadSamplerInitFunc_(SAMPLE_CNT);
    uniTableSize = GetMMapSizeAndName("hicollie_buf", uniStackTableMMapName);
    
    uint32_t bufSize = 128 * 1024;
    ASSERT_EQ(uniTableSize, bufSize);
    ASSERT_EQ(isSubStr(uniStackTableMMapName, "hicollie_buf"), true);
    printf("mmap name: %s, size: %u KB\n", uniStackTableMMapName.c_str(), uniTableSize);

    threadSamplerDeinitFunc_();
    dlclose(threadSamplerFuncHandler_);
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
