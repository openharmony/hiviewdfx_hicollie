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

#ifndef XCOLLIE_MGR_H
#define XCOLLIE_MGR_H

#include <mutex>
#include <unordered_map>
#include "singleton.h"
#include "xcollie_define.h"
#include "hicollie.h"
namespace OHOS {
namespace HiviewDFX {
class XcollieMgr : public Singleton<XcollieMgr> {
    DECLARE_SINGLETON(XcollieMgr);
public:
    void* SetHandler(OH_HiCollie_FreezeCallback handler);
    std::string ReadDataFromBuffer(int type);

private:
    bool CheckCallDuration(int type);

    std::mutex mutex_;
    std::unordered_map<int, int64_t> lastCallTime_;
    OH_HiCollie_FreezeCallback handler_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // FREEZE_CALLBACK_MGR_H