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

#ifndef RELIABILITY_WATCHDOG_INNER_DATA_H
#define RELIABILITY_WATCHDOG_INNER_DATA_H

namespace OHOS {
namespace HiviewDFX {
constexpr const char* KEY_SAMPLE_INTERVAL = "sample_interval";
constexpr const char* KEY_SAMPLE_COUNT = "sample_count";
constexpr const char* KEY_SAMPLE_REPORT_TIMES = "report_times_per_app";
constexpr const char* KEY_LOG_TYPE = "log_type";
constexpr const char* KEY_SET_TIMES_FLAG = "set_report_times_flag";
constexpr const char* KEY_IGNORE_STARTUP_TIME = "ignore_startup_time";
constexpr const char* KEY_CHECKER_INTERVAL = "checker_interval";
constexpr const char* KEY_AUTO_STOP_SAMPLING = "auto_stop_sampling";
constexpr const char* APP_START_CONFIG = "/data/storage/el2/log/xperf_config";
constexpr const char* EVENT_APP_START_SLOW = "APP_START_SLOW";
constexpr const char* EVENT_SLIDING_JANK = "SLIDING_JANK";
constexpr const char* KEY_EVENT_NAME = "event_name";
constexpr const char* KEY_THRESHOLD = "threshold";
constexpr const char* KEY_TRIGGER_INTERVAL = "trigger_interval";
constexpr const char* KEY_COLLECT_TIMES = "collect_times";
constexpr const char* KEY_REPORT_TIMES = "report_times";
constexpr const char* KEY_START_TIME = "start_time";
constexpr const char* KEY_STARTUP_DURATION = "startup_duration";
constexpr const char* EVENT_APP_START_SCROLL_JANK = "APP_START_SCROLL_JANK";
constexpr const char* EVENT_APP_START_JANK = "APP_START_JANK";
constexpr const char* APP_START_SAMPLE = "AppStartSample";
constexpr int SAMPLE_DEFAULT_INTERVAL = 150;
constexpr int SAMPLE_DEFAULT_COUNT = 10;
constexpr int SAMPLE_DEFAULT_REPORT_TIMES = 1;
constexpr int APP_START_PARAM_SIZE = 5;
constexpr int DEFAULT_IGNORE_STARTUP_TIME = 10; // 10s
constexpr int XCOLLIE_CALLBACK_HISTORY_MAX = 5;
constexpr int XCOLLIE_CALLBACK_TIMEWIN_MAX = 60;
constexpr unsigned int MAX_WATCH_NUM = 128; // 128: max handler thread
constexpr int XCOLLIE_TASK_MAX_CONCURRENCY_NUM = 1;
constexpr int64_t APP_START_LIMIT = 7 * 24 * 60 * 60 * 1000; // 7 days
constexpr int SET_TIMES_FLAG = 1;
constexpr int ENABLE_TREE_FORMAT = 1;
constexpr int DEFAULT_RESERVED_TIME = 3500; // 3.5s
constexpr int BETA_RESERVED_TIME = 6000; // 6s

using TimePoint = AppExecFwk::InnerEvent::TimePoint;

struct SamplerResult {
    uint64_t samplerStartTime;
    uint64_t samplerFinishTime;
    int32_t samplerCount;
};

typedef void (*WatchdogInnerBeginFunc)(const char* eventName);
typedef void (*WatchdogInnerEndFunc)(const char* eventName);
typedef int (*ThreadSamplerInitFunc)(int, int);
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
    int reportTimes {SAMPLE_DEFAULT_REPORT_TIMES};
    int scrollTimes {SAMPLE_DEFAULT_REPORT_TIMES};
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
    int reportTimes {SAMPLE_DEFAULT_REPORT_TIMES};
    bool isStartSampleEnabled {true};
    bool isFinishStartSample {false};
    std::atomic_bool enableStartSample {false};
};

struct SampleFreezeInfo {
    uint64_t lastSaveTime {0};
    std::string freezeFile;
};

struct SampleJankParams {
    int logType {0};
    int ignoreStartUpTime {DEFAULT_IGNORE_STARTUP_TIME};
    int sampleInterval {SAMPLE_DEFAULT_INTERVAL};
    int sampleCount {SAMPLE_DEFAULT_COUNT};
    int reportTimes {SAMPLE_DEFAULT_REPORT_TIMES};
    int autoStopSampling {0};
    int eventType {0};
};
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif
