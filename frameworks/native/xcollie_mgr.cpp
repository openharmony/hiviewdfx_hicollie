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

#include <string>
#include <memory>
#include <thread>
#include <sys/mman.h>

#include "xcollie_define.h"
#include "xcollie_utils.h"
#include "ffrt_inner.h"

namespace OHOS {
namespace HiviewDFX {
struct SharedState {
    std::mutex cvMutex;
    std::condition_variable cv;
    std::string result = "";
    bool ready = false;
};
namespace {
    constexpr uint32_t TIME_MIN_TO_S = 60;
    constexpr uint32_t TIME_S_TO_MS = 1000;
    constexpr uint32_t COUNT_OF_FAULT_TYPE = 8;
    constexpr uint32_t CALLBACK_WAIT_TIME = 1;
    constexpr std::size_t MAX_BUFFER_SIZE = 64 * 1024; // 65536 bytes
}
XcollieMgr::XcollieMgr()
{
}

XcollieMgr::~XcollieMgr()
{
}

void* XcollieMgr::SetHandler(OH_HiCollie_FreezeCallback handler)
{
    std::lock_guard<std::mutex> lock(mutex_);
    void* temp = (void*)handler_;
    handler_ = handler;
    return temp;
}

bool XcollieMgr::CheckCallDuration(int type)
{
    std::lock_guard<std::mutex> lock(mutex_);
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::
        steady_clock::now().time_since_epoch()).count();
    if (now - lastCallTime_[type] > TIME_MIN_TO_S * TIME_S_TO_MS) {
        lastCallTime_[type] = now;
        return true;
    }
    return false;
}

std::string XcollieMgr::ReadDataFromBuffer(int type)
{
    if (type < 0 || type >= COUNT_OF_FAULT_TYPE) {
        XCOLLIE_LOGE("invalid freeze type");
        return "";
    }
    if (!CheckCallDuration(type)) {
        XCOLLIE_LOGE("can not ReadDataFromBuffer twice within 1 min");
        return "";
    }
    OH_HiCollie_FreezeCallback handler;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        handler = handler_;
    }
    if (handler == nullptr) {
        XCOLLIE_LOGE("no freeze callback");
        return "";
    }
    auto state = std::make_shared<SharedState>();
    std::thread worker([state, handler, type]() {
        size_t mmapSize = OHOS::HiviewDFX::MAX_BUFFER_SIZE;
        void* mptr = mmap(nullptr, mmapSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mptr == MAP_FAILED) {
            XCOLLIE_LOGE("mmap failed! errno:%{public}d", errno);
            return ;
        }
        size_t callbackSize = handler((OH_HiCollie_Freeze_Type)type, mptr, OHOS::HiviewDFX::MAX_BUFFER_SIZE);
        if (callbackSize > OHOS::HiviewDFX::MAX_BUFFER_SIZE) {
            XCOLLIE_LOGW("callback size over 64KB size:%{public}zu", callbackSize);
            munmap(mptr, mmapSize);
            return;
        }
        state->result = (char*)mptr;
        munmap(mptr, mmapSize);
        {
            std::lock_guard<std::mutex> innerLock(state->cvMutex);
            state->ready = true;
        }
        state->cv.notify_one();
    });
    std::unique_lock<std::mutex> lock(state->cvMutex);
    if (!state->cv.wait_for(lock, std::chrono::seconds(CALLBACK_WAIT_TIME), [&]() { return state->ready; })) {
        XCOLLIE_LOGE("freeze callback timeout!");
        worker.detach();
        return "";
    }
    worker.join();
    return state->result;
}
} // end of namespace HiviewDFX
} // end of namespace OHOS
