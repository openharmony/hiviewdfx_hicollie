/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef RELIABILITY_XCOLLIE_DEFINE_H
#define RELIABILITY_XCOLLIE_DEFINE_H
namespace OHOS {
namespace HiviewDFX {
/* define watchdog type */
constexpr unsigned int XCOLLIE_LOCK = (1 << 0);
constexpr unsigned int XCOLLIE_THREAD = (1 << 1);

/* define timeout flag */
constexpr unsigned int XCOLLIE_FLAG_DEFAULT = (~0); // do all callback function
constexpr unsigned int XCOLLIE_FLAG_NOOP = (0); // do nothing but the caller defined function
constexpr unsigned int XCOLLIE_FLAG_LOG = (1 << 0); // generate log file
constexpr unsigned int XCOLLIE_FLAG_RECOVERY = (1 << 1); // die when timeout

/* define xcollie id */
constexpr int INVALID_ID = -1;

/* define ipc full default params */
constexpr uint64_t DEFAULT_IPC_FULL_INTERVAL = 30;
constexpr uint64_t ADD_IPC_FULL_INTERVAL = 10;
constexpr char IPC_FULL_TASK[] = "OS_DfxIpcCheck";
constexpr char ASYNC_BINDER_SPACE_FULL_TASK[] = "OS_BinderSpace";
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif
