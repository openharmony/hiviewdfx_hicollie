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

#ifndef RELIABILITY_XCOLLIE_H
#define RELIABILITY_XCOLLIE_H
#include <string>
#include "singleton.h"
#include "xcollie_define.h"

using XCollieCallback = std::function<void (void *)>;
namespace OHOS {
namespace HiviewDFX {
class XCollie : public Singleton<XCollie> {
    DECLARE_SINGLETON(XCollie);
public:
    // set timer
    // name : timer name
    // timeout : timeout, unit s
    // func : callback
    // arg : the callback's param
    // flag : timer timeout operation. the value can be:
    //                               XCOLLIE_FLAG_DEFAULT :do all callback function
    //                               XCOLLIE_FLAG_NOOP  : do nothing but the caller defined function
    //                               XCOLLIE_FLAG_LOG :  generate log file
    //                               XCOLLIE_FLAG_RECOVERY  : die when timeout
    // return: the timer id
    int SetTimer(const std::string &name, unsigned int timeout,
        XCollieCallback func, void *arg, unsigned int flag);

    // cancel timer
    // id: timer id
    void CancelTimer(int id);

    // set timer and count
    // name: timer and count name
    // timeLimit: event occured time limt
    // countLimit: event occured count limit
    int SetTimerCount(const std::string &name, unsigned int timeLimit, int countLimit);

    // trigger timer count
    // name: tigger name
    // bTrigger: trigger or cancel
    // message: need report pid current message
    void TriggerTimerCount(const std::string &name, bool bTrigger, const std::string &message);
};
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif

