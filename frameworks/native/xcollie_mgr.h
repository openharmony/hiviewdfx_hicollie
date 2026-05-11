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

#ifndef HIVIEWDFX_HICOLLIE_XCOLLIE_MGR_H
#define HIVIEWDFX_HICOLLIE_XCOLLIE_MGR_H

#include <mutex>
#include <unordered_map>
#include "singleton.h"
#include "xcollie_define.h"

namespace OHOS {
namespace HiviewDFX {
class XcollieMgr : public Singleton<XcollieMgr> {
    DECLARE_SINGLETON(XcollieMgr);
public:
    void SetInvoker(XCollieInnerCallback callback);
    void SetHandler(void* handler);
    std::string ReadDataFromBuffer(int type);

private:
    std::mutex mutex_;
    void* handler_ = nullptr;
    XCollieInnerCallback lastFreezeCallback_ = nullptr;
    std::unordered_map<int, int64_t> lastCallTime_;
    bool CheckCallDuration(int type);
};
} // end of HiviewDFX
} // end of OHOS
#endif //HIVIEWDFX_HICOLLIE_XCOLLIE_MGR_H
