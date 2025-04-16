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

#include "ipc_full.h"

#include "watchdog_inner.h"

namespace OHOS {
namespace HiviewDFX {
IpcFull::IpcFull()
{
}
IpcFull::~IpcFull()
{
}

bool IpcFull::AddIpcFull(uint64_t interval, unsigned int flag, IpcFullCallback func, void *arg)
{
    return WatchdogInner::GetInstance().AddIpcFull(interval, flag, func, arg);
}
}
}