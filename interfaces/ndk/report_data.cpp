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

#include "string_ex.h"
#include "report_data.h"
#include "xcollie_utils.h"

namespace OHOS {
namespace HiviewDFX {
bool ReportData::WriteContent(Parcel &parcel) const
{
    if (!parcel.WriteUint32(stuckTimeout)) {
        XCOLLIE_LOGE("StuckTimeout [%{public}u] write uint32 failed.", stuckTimeout);
        return false;
    }

    if (token == nullptr) {
        if (!parcel.WriteBool(false)) {
            XCOLLIE_LOGE("Token falge [false] write bool failed.");
            return false;
        }
    } else {
        if (!parcel.WriteBool(true) || !(static_cast<MessageParcel*>(&parcel))->WriteRemoteObject(token)) {
            XCOLLIE_LOGE("Token falge [true] write bool failed.");
            return false;
        }
    }
    return true;
}

bool ReportData::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteString(errorObject.name)) {
        XCOLLIE_LOGE("Name [%{public}s] write string failed.", errorObject.name.c_str());
        return false;
    }

    if (!parcel.WriteString(errorObject.message)) {
        XCOLLIE_LOGE("Message [%{public}s] write string failed.", errorObject.message.c_str());
        return false;
    }
    
    if (!parcel.WriteString(errorObject.stack)) {
        XCOLLIE_LOGE("Stack [%{public}s] write string failed.", errorObject.stack.c_str());
        return false;
    }

    if (!parcel.WriteInt32(static_cast<int32_t>(faultType))) {
        XCOLLIE_LOGE("FaultType [%{public}d] write int32 failed.", static_cast<int32_t>(faultType));
        return false;
    }

    if (!parcel.WriteString(timeoutMarkers)) {
        XCOLLIE_LOGE("TimeoutMarkers [%{public}s] write string failed.", timeoutMarkers.c_str());
        return false;
    }

    if (!parcel.WriteBool(waitSaveState)) {
        XCOLLIE_LOGE("WaitSaveState [%{public}s] write bool failed.", waitSaveState ? "true" : "false");
        return false;
    }

    if (!parcel.WriteBool(notifyApp)) {
        XCOLLIE_LOGE("NotifyApp [%{public}s] write bool failed.", notifyApp ? "true" : "false");
        return false;
    }

    if (!parcel.WriteBool(forceExit)) {
        XCOLLIE_LOGE("ForceExit [%{public}s] write bool failed.", forceExit ? "true" : "false");
        return false;
    }

    if (!parcel.WriteUint32(state)) {
        XCOLLIE_LOGE("State [%{public}u] write uint32 failed.", state);
        return false;
    }

    if (!parcel.WriteInt32(eventId)) {
        XCOLLIE_LOGE("EventId [%{public}u] write int32 failed.", eventId);
        return false;
    }

    if (!parcel.WriteInt32(tid)) {
        XCOLLIE_LOGE("Tid [%{public}u] write int32 failed.", tid);
        return false;
    }

    return WriteContent(parcel);
}
} // end of HiviewDFX
} // end of OHOS
