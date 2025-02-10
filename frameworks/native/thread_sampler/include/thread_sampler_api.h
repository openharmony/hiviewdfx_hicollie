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

#ifndef RELIABILITY_THREAD_SAMPLER_API_H
#define RELIABILITY_THREAD_SAMPLER_API_H

#include "thread_sampler.h"

namespace OHOS {
namespace HiviewDFX {
constexpr int SUCCESS = 0;
constexpr int FAIL = -1;

#ifdef __cplusplus
extern "C" {
#endif

/* To initialize thread sampler, load resources needed, return 0 for success. */
int ThreadSamplerInit(int collectStackCount);

/* To start sample stack with thread sampler. */
int32_t ThreadSamplerSample();

/* To collect the stack infomation collected by thread sampler in string.
 * stack: the string to save the stack infomation.
 * size: the length of stack.
 * treeFormat: collect stack infomation in tree format or not. 1 for tree format and 0 for not.
 * return 0 for success and -1 for fail.
 */
int ThreadSamplerCollect(char* stack, char* heaviestStack, size_t stackSize, size_t heaviestSize, int treeFormat = 1);

/* To deinitial thread sampler and unload the resources. */
int ThreadSamplerDeinit();

/* The signal handler function */
void ThreadSamplerSigHandler(int sig, siginfo_t* si, void* context);

#ifdef __cplusplus
}
#endif
}
}
#endif
