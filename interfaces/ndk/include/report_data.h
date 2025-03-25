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

#ifndef HIVIEWDFX_HICOLLIE_FAULT_DATA_H
#define HIVIEWDFX_HICOLLIE_FAULT_DATA_H

#include <string>
#include "parcel.h"
#include "iremote_object.h"

namespace OHOS {
namespace HiviewDFX {
struct ErrorObject {
    std::string name = "";
    std::string message = "";
    std::string stack = "";
};

enum class FaultDataType {
    UNKNOWN = -1,
    APP_FREEZE = 3
};

/**
 * @struct ReportData
 * ReportData is used to save information about report data.
 */
struct ReportData : public Parcelable {
    virtual bool Marshalling(Parcel &parcel) const override;
    bool WriteContent(Parcel &parcel) const;
    // error object
    ErrorObject errorObject;
    FaultDataType faultType = FaultDataType::UNKNOWN;
    std::string timeoutMarkers = "";
    bool waitSaveState = false;
    bool notifyApp = false;
    bool forceExit = false;
    bool needKillProcess = true;
    uint32_t state = 0;
    int32_t eventId = -1;
    int32_t tid = -1;
    uint32_t stuckTimeout = 0;
    sptr<IRemoteObject> token = nullptr;
};
} // end of HiviewDFX
} // end of OHOS
#endif  // HIVIEWDFX_HICOLLIE_FAULT_DATA_H
