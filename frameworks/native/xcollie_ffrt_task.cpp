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

#include "xcollie_ffrt_task.h"
#include "xcollie_utils.h"

namespace OHOS {
namespace HiviewDFX {
int XCollieFfrtTask::maxTaskNum_ = 0;
int XCollieFfrtTask::openTraceCode_ = -1;
ffrt::mutex XCollieFfrtTask::traceCodeMutex_;

XCollieFfrtTask::XCollieFfrtTask(int maxFfrtNum)
{
    maxTaskNum_ = maxFfrtNum;
}

void XCollieFfrtTask::InitFfrtTask()
{
    if (!xcollieQueue_) {
        xcollieQueue_ = std::make_unique<ffrt::queue>(ffrt::queue_concurrent,
            "XCollieFfrtTask_queue",
            ffrt::queue_attr().qos(ffrt::qos_default).max_concurrency(maxTaskNum_));
    }
}

void XCollieFfrtTask::StartTrace(UCollectClient::AppCaller appCaller,
    const std::shared_ptr<UCollectClient::TraceCollector> &traceCollector)
{
    uint64_t beforeTime = GetCurrentTickMillseconds();
    auto result = traceCollector->CaptureDurationTrace(appCaller);
    {
        std::unique_lock<ffrt::mutex> lock(traceCodeMutex_);
        openTraceCode_ = result.retCode;
    }
    XCOLLIE_LOGI("Start to open trace result: %{public}d, interval: %{public}" PRIu64 " ms",
        result.retCode, (GetCurrentTickMillseconds() - beforeTime));
}

void XCollieFfrtTask::DumpTrace(UCollectClient::AppCaller appCaller,
    const std::shared_ptr<UCollectClient::TraceCollector> &traceCollector)
{
    int ret = -1;
    {
        std::unique_lock<ffrt::mutex> lock(traceCodeMutex_);
        ret = openTraceCode_;
    }
    if (ret != 0) {
        XCOLLIE_LOGI("Dump trace failed, openTrace result: %{public}d", ret);
        return;
    }
    uint64_t beforeTime = GetCurrentTickMillseconds();
    auto result = traceCollector->CaptureDurationTrace(appCaller);
    XCOLLIE_LOGI("Dump trace result: %{public}d, interval: %{public}" PRIu64 " ms",
        result.retCode, (GetCurrentTickMillseconds() - beforeTime));
}

void XCollieFfrtTask::SubmitStartTraceTask(UCollectClient::AppCaller appCaller,
    const std::shared_ptr<UCollectClient::TraceCollector> &traceCollector)
{
    if (!xcollieQueue_) {
        XCOLLIE_LOGE("open trace failed");
        return;
    }
    xcollieQueue_->submit([this, appCaller, traceCollector] {
        this->StartTrace(appCaller, traceCollector);
        }, ffrt::task_attr().name("xcollie_start_trace")
    );
}

void XCollieFfrtTask::SubmitDumpTraceTask(UCollectClient::AppCaller appCaller,
    const std::shared_ptr<UCollectClient::TraceCollector> &traceCollector)
{
    if (!xcollieQueue_) {
        XCOLLIE_LOGE("dump trace failed");
        return;
    }
    xcollieQueue_->submit([this, appCaller, traceCollector] {
        this->DumpTrace(appCaller, traceCollector);
        }, ffrt::task_attr().name("xcollie_dump_trace")
    );
}

} // end of namespace HiviewDFX
} // end of namespace OHOS
