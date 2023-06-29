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

#include "xcollie.h"

#include <future>

#include "xcollie_utils.h"
#include "watchdog_inner.h"

namespace OHOS {
namespace HiviewDFX {
constexpr uint64_t TO_NANOSECOND_MULTPLE = 1000;
XCollie::XCollie()
{
}
XCollie::~XCollie()
{
}

int XCollie::SetTimer(const std::string &name, unsigned int timeout, std::function<void(void *)> func,
    void *arg, unsigned int flag)
{
    return WatchdogInner::GetInstance().RunXCollieTask(name, timeout * TO_NANOSECOND_MULTPLE, func, arg, flag);
}

void XCollie::CancelTimer(int id)
{
    WatchdogInner::GetInstance().RemoveXCollieTask(id);
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
