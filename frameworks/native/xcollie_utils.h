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

#ifndef RELIABILITY_XCOLLIE_UTILS_H
#define RELIABILITY_XCOLLIE_UTILS_H

#include <chrono>
#include <string>
#include <sys/ioctl.h>
#include <fstream>
#include <vector>

#include "hilog/log.h"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002D06

#undef LOG_TAG
#define LOG_TAG "XCollie"

namespace OHOS {
namespace HiviewDFX {
enum DumpStackState {
    DEFAULT = 0,
    COMPLETE = 1,
    SAMPLE_COMPLETE = 2
};
constexpr char WMS_FULL_NAME[] = "WindowManagerService";
constexpr char IPC_FULL[] = "IPC_FULL";
constexpr char IPC_CHECKER[] = "IpcChecker";
constexpr char STACK_CHECKER[] = "ThreadSampler";
constexpr char TRACE_CHECKER[] = "TraceCollector";
constexpr int64_t SEC_TO_MANOSEC = 1000000000;
constexpr int64_t SEC_TO_MICROSEC = 1000000;
constexpr int64_t ONE_DAY_LIMIT = 86400000;
constexpr int64_t MILLISEC_TO_NANOSEC = 1000000;
constexpr uint64_t MAX_FILE_SIZE = 10 * 1024 * 1024; // 10M
const int BUFF_STACK_SIZE = 20 * 1024;
const int FFRT_BUFFER_SIZE = 512 * 1024;
const int MAX_NAME_SIZE = 128;
const int MIN_WAIT_NUM = 3;
const int TIME_INDEX_MAX = 32;
const int DETECT_STACK_COUNT = 2;
const int COLLECT_STACK_COUNT = 10;
const int COLLECT_TRACE_MIN = 1;
const int COLLECT_TRACE_MAX = 20;
const int TASK_INTERVAL = 155;
const int DURATION_TIME = 150;
const int INIT_PID = 1;
const int64_t DISTRIBUTE_TIME = 2000;
const int64_t DUMPTRACE_TIME = 450;
const inline std::string LOGGER_BINDER_PROC_PATH = "/proc/transaction_proc";
const std::string WATCHDOG_DIR = "/data/storage/el2/log/watchdog";
const std::string KEY_HIVIEW_USER_TYPE = "const.logsystem.versiontype";

#define XCOLLIE_LOGF(...) HILOG_FATAL(LOG_CORE, ##__VA_ARGS__)
#define XCOLLIE_LOGE(...) HILOG_ERROR(LOG_CORE, ##__VA_ARGS__)
#define XCOLLIE_LOGW(...) HILOG_WARN(LOG_CORE, ##__VA_ARGS__)
#define XCOLLIE_LOGI(...) HILOG_INFO(LOG_CORE, ##__VA_ARGS__)
#define XCOLLIE_LOGD(...) HILOG_DEBUG(LOG_CORE, ##__VA_ARGS__)
#define MAGIC_NUM           0x9517
#define HTRANSIO            0xAB
#define LOGGER_GET_STACK    _IO(HTRANSIO, 9)

uint64_t GetCurrentTickMillseconds();

std::string GetSelfProcName();

std::string GetFirstLine(const std::string& path);

std::string GetProcessNameFromProcCmdline(int32_t pid);

std::string GetLimitedSizeName(std::string name);

bool IsProcessDebug(int32_t pid);

void DelayBeforeExit(unsigned int leftTime);

std::string TrimStr(const std::string& str, const char cTrim = ' ');

void SplitStr(const std::string& str, const std::string& sep,
    std::vector<std::string>& strs, bool canEmpty = false, bool needTrim = true);

int ParsePeerBinderPid(std::ifstream& fin, int32_t pid);

bool KillProcessByPid(int32_t pid);

std::string GetFormatDate();

bool CreateWatchdogDir();

bool WriteStackToFd(int32_t pid, std::string& path, std::string& stack);

int64_t GetTimeStamp();

bool IsCommercialVersion();
} // end of HiviewDFX
} // end of OHOS
#endif
