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

#ifndef RELIABILITY_XCOLLIE_FFRT_TASK_H
#define RELIABILITY_XCOLLIE_FFRT_TASK_H

#include <string>
#include "ffrt.h"
#include "client/trace_collector_client.h"

namespace OHOS {
namespace HiviewDFX {
class XCollieFfrtTask {
public:
    explicit XCollieFfrtTask(int maxFfrtNum);
    XCollieFfrtTask() {};
    ~XCollieFfrtTask() {};

    void InitFfrtTask();
    void SubmitStartTraceTask(UCollectClient::AppCaller appCaller,
        const std::shared_ptr<UCollectClient::TraceCollector> &traceCollector);
    void SubmitDumpTraceTask(UCollectClient::AppCaller appCaller,
        const std::shared_ptr<UCollectClient::TraceCollector> &traceCollector);

private:
    void StartTrace(UCollectClient::AppCaller appCaller,
        const std::shared_ptr<UCollectClient::TraceCollector> &traceCollector);
    void DumpTrace(UCollectClient::AppCaller appCaller,
        const std::shared_ptr<UCollectClient::TraceCollector> &traceCollector);

    static int maxTaskNum_;
    static int openTraceCode_;
    std::unique_ptr<ffrt::queue> xcollieQueue_ = nullptr;
};
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif

