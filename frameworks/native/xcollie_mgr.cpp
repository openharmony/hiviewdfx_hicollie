/*
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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
#include "xcollie_mgr.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string>
#include <sys/syscall.h>

#include "xcollie_define.h"
#include "xcollie_utils.h"
#include "ffrt_inner.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
    constexpr uint32_t TIME_MIN_TO_S = 60;
    constexpr uint32_t TIME_S_TO_MS = 1000;
    constexpr uint32_t COUNT_OF_FAULT_TYPE = 1000;
    constexpr uint32_t CALLBACK_WAIT_TIME = 10000; // 10s
}

XCollieMgr::XCollieMgr(){

}

XCollieMgr::~XCollieMgr(){

}

void XCollieMgr::SetInvoker(XCollieCallback callback)
{
    lastFreezeCallback_ = callback;
}

void XCollieMgr::SetHandler(void *handler)
{
    handler_ = handler;
}

bool XCollieMgr::CheckCallDuration(int type)
{
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::
        steady_clock::now().time_since_epoch()).count();
    if (now - lastCallTime_[type] > TIME_MIN_TO_S * TIME_S_TO_MS) {
        lastCallTime_[type] = now;
        return true;
    }
    return false;
}

std::string XCollieMgr::ReadDataFromBuffer(int type)
{
    if (type < 0 || type > COUNT_OF_FAULT_TYPE) {
        XCOLLIE_LOGE("invalid freeze type");
        return "";
    }
    if (!CheckCallDuration(type)) {
        XCOLLIE_LOGE("can not ReadDataFromBuffer twice within 1 min");
        return "";
    }
    XCollieCallback callback = lastFreezeCallback_;
    if (callback == nullptr) {
        XCOLLIE_LOGE("no freeze callback");
        return "";
    }
    auto syncPromise = std::make_shared<ffrt::promise<std::string>>();
    if (syncPromise == nullptr) {
        XCOLLIE_LOGE("syncPromise error");
        return "";
    }
    ffrt::future syncFuture = syncPromise->get_future();
    ffrt::submit([this, syncPromise, callback, type]() {
        syncPromise->set_value(callback(this->handler_, type));
    }, ffrt::task_attr().qos(static_cast<int>(ffrt_qos_user_initiated)));
    ffrt::future_status wait = syncFuture.wait_for(std::chrono::milliseconds(CALLBACK_WAIT_TIME));
    if (wait != ffrt::future_status::ready) {
        XCOLLIE_LOGE("invalid freeze type");
        return "";
    }
    return syncFuture.get();
}
} // end of HiviewDFX
} // end of OHOS