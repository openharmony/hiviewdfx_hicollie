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

#ifndef RELIABILITY_MAIN_THREAD_SAMPLER_TEST_H
#define RELIABILITY_MAIN_THREAD_SAMPLER_TEST_H

#include <gtest/gtest.h>

namespace OHOS {
namespace HiviewDFX {
typedef int (*ThreadSamplerInitFunc)(int);
typedef int32_t (*ThreadSamplerSampleFunc)();
typedef int (*ThreadSamplerCollectFunc)(char*, size_t, int);
typedef int (*ThreadSamplerDeinitFunc)();
typedef void (*SigActionType)(int, siginfo_t*, void*);

class ThreadSamplerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    bool InitThreadSamplerFuncs();
    static void ThreadSamplerSigHandler(int sig, siginfo_t* si, void* context);
    bool InstallThreadSamplerSignal();
    void UninstallThreadSamplerSignal();
    bool InitThreadSampler();

    void* threadSamplerFuncHandler_  {nullptr};
    ThreadSamplerInitFunc threadSamplerInitFunc_ {nullptr};
    ThreadSamplerSampleFunc threadSamplerSampleFunc_ {nullptr};
    ThreadSamplerCollectFunc threadSamplerCollectFunc_ {nullptr};
    ThreadSamplerDeinitFunc threadSamplerDeinitFunc_ {nullptr};

    static SigActionType threadSamplerSigHandler_;
};
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif
