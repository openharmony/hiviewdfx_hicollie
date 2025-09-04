/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef RELIABILITY_WATCHDOG_INNER_DATA_H
#define RELIABILITY_WATCHDOG_INNER_DATA_H

namespace OHOS {
namespace HiviewDFX {
constexpr const char* const KEY_SAMPLE_INTERVAL = "sample_interval";
constexpr const char* const KEY_SAMPLE_COUNT = "sample_count";
constexpr const char* const KEY_SAMPLE_REPORT_TIMES = "report_times_per_app";
constexpr const char* const KEY_LOG_TYPE = "log_type";
constexpr const char* const KEY_SET_TIMES_FLAG = "set_report_times_flag";
constexpr const char* const KEY_IGNORE_STARTUP_TIME = "ignore_startup_time";
constexpr const char* const KEY_CHECKER_INTERVAL = "checker_interval";
constexpr const char* const APP_START_CONFIG = "/data/storage/el2/log/xperf_config";
constexpr const char* const EVENT_APP_START_SLOW = "APP_START_SLOW";
constexpr const char* const EVENT_SLIDING_JANK = "SLIDING_JANK";
constexpr const char* const KEY_EVENT_NAME = "event_name";
constexpr const char* const KEY_THRESHOLD = "threshold";
constexpr const char* const KEY_TRIGGER_INTERVAL = "trigger_interval";
constexpr const char* const KEY_COLLECT_TIMES = "collect_times";
constexpr const char* const KEY_REPORT_TIMES = "report_times";
constexpr const char* const KEY_START_TIME = "start_time";
constexpr const char* const KEY_STARTUP_DURATION = "startup_duration";
constexpr const char* const EVENT_APP_START_SCROLL_JANK = "APP_START_SCROLL_JANK";
constexpr const char* const EVENT_APP_START_JANK = "APP_START_JANK";
constexpr const char* const APP_START_SAMPLE = "AppStartSample";
const int SAMPLE_DEFULE_INTERVAL = 150;
const int SAMPLE_DEFULE_COUNT = 10;
const int SAMPLE_DEFULE_REPORT_TIMES = 1;
const int SET_TIMES_FLAG = 1;
const int DEFAULT_IGNORE_STARTUP_TIME = 10; // 10s
const int ENABLE_TREE_FORMAT = 1;
const int APP_START_PARAM_SIZE = 5;
const int XCOLLIE_CALLBACK_HISTORY_MAX = 5;
const int XCOLLIE_CALLBACK_TIMEWIN_MAX = 60;
const unsigned int MAX_WATCH_NUM = 128; // 128: max handler thread
constexpr int64_t APP_START_LIMIT = 7 * 24 * 60 * 60 * 1000; // 7 days

using TimePoint = AppExecFwk::InnerEvent::TimePoint;

struct SamplerResult {
    uint64_t samplerStartTime;
    uint64_t samplerFinishTime;
    int32_t samplerCount;
};

typedef void (*WatchdogInnerBeginFunc)(const char* eventName);
typedef void (*WatchdogInnerEndFunc)(const char* eventName);
typedef int (*ThreadSamplerInitFunc)(int);
typedef int32_t (*ThreadSamplerSampleFunc)();
typedef int (*ThreadSamplerCollectFunc)(char*, char*, size_t, size_t, int);
typedef int (*ThreadSamplerDeinitFunc)();
typedef void (*SigActionType)(int, siginfo_t*, void*);
typedef SamplerResult (*ThreadSamplerGetResultFunc)();

struct TimeContent {
    int64_t curBegin;
    int64_t curEnd;
};

struct StackContent {
    bool isStartSampleEnabled {true};
    int detectorCount {0};
    int collectCount {0};
    int reportTimes {SAMPLE_DEFULE_REPORT_TIMES};
    int scrollTimes {SAMPLE_DEFULE_REPORT_TIMES};
    int64_t reportBegin {0};
    int64_t reportEnd {0};
    TimePoint lastEndTime;
};

struct TraceContent {
    int traceState {0};
    int traceCount {0};
    int dumpCount {0};
    int64_t reportBegin {0};
    int64_t reportEnd {0};
    TimePoint lastEndTime;
};

struct AppStartContent {
    int64_t startTime {0};
    int64_t sampleInterval {0};
    int64_t startUpDuration {0};
    int64_t threshold {0};
    std::atomic_int collectCount {0};
    int targetCount {0};
    int reportTimes {SAMPLE_DEFULE_REPORT_TIMES};
    bool isStartSampleEnabled {true};
    bool isFinishStartSample {false};
    std::atomic_bool enableStartSample {false};
};
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif
