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
#include <map>
#include <vector>

#include "hilog/log.h"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002D06

#undef LOG_TAG
#define LOG_TAG "XCollie"

namespace OHOS {
namespace HiviewDFX {
constexpr uint64_t MIN_APP_UID = 20000;
constexpr uint64_t TO_MILLISECOND_MULTPLE = 1000;
constexpr int64_t SEC_TO_MICROSEC = 1000000;
constexpr const char* const WATCHDOG_DIR = "/data/storage/el2/log/watchdog/";
constexpr const char* const FREEZE_DIR = "/data/storage/el2/log/watchdog/freeze/";

#define XCOLLIE_LOGF(...) HILOG_FATAL(LOG_CORE, ##__VA_ARGS__)
#define XCOLLIE_LOGE(...) HILOG_ERROR(LOG_CORE, ##__VA_ARGS__)
#define XCOLLIE_LOGW(...) HILOG_WARN(LOG_CORE, ##__VA_ARGS__)
#define XCOLLIE_LOGI(...) HILOG_INFO(LOG_CORE, ##__VA_ARGS__)
#define XCOLLIE_LOGD(...) HILOG_DEBUG(LOG_CORE, ##__VA_ARGS__)
#define MAGIC_NUM           0x9517
#define HTRANSIO            0xAB
#define LOGGER_GET_STACK    _IO(HTRANSIO, 9)

constexpr OHOS::HiviewDFX::HiLogLabel KLOG_LABEL = {
    LOG_KMSG,
    LOG_DOMAIN,
    LOG_TAG
};
#define XCOLLIE_KLOGI(...) \
    do { \
        (void)OHOS::HiviewDFX::HiLog::Info(KLOG_LABEL, __VA_ARGS__); \
        HILOG_INFO(LOG_CORE, __VA_ARGS__); \
    } while (0)

#define XCOLLIE_KLOGE(...) \
    do { \
        (void)OHOS::HiviewDFX::HiLog::Error(KLOG_LABEL, __VA_ARGS__); \
        HILOG_ERROR(LOG_CORE, __VA_ARGS__); \
    } while (0)

struct FileInfo {
    std::string filePath;
    time_t mtime;
};
#ifdef SUSPEND_CHECK_ENABLE
std::pair<double, double> GetSuspendTime(const char* path, uint64_t &now);
#endif

uint64_t GetCurrentTickMillseconds();

#ifdef SUSPEND_CHECK_ENABLE
uint64_t GetCurrentBootMillseconds();

void CalculateTimes(uint64_t& bootTimeStart, uint64_t& monoTimeStart);

uint64_t GetNumsDiffAbs(const uint64_t& numOne, const uint64_t& numTwo);
#endif

bool IsFileNameFormat(char c);

std::string GetSelfProcName();

std::string GetFirstLine(const std::string& path);

std::string GetProcessNameFromProcCmdline(int32_t pid = 0);

std::string GetLimitedSizeName(std::string name);

bool IsProcessDebug(int32_t pid);

void DelayBeforeExit(unsigned int leftTime);

std::string TrimStr(const std::string& str, const char cTrim = ' ');

void SplitStr(const std::string& str, const std::string& sep,
    std::vector<std::string>& strs, bool canEmpty = false, bool needTrim = true);

int ParsePeerBinderPid(std::ifstream& fin, int32_t pid);

bool KillProcessByPid(int32_t pid);

bool IsDeveloperOpen();

bool IsBetaVersion();

bool IsFansStage();

bool IsAsyncStackBlockBundle(const std::string& bundleName);

std::string GetFormatDate();

std::string FormatTime(const std::string &format);

bool CreateDir(const std::string& dirPath);

void GetFilesByDir(std::vector<FileInfo> &fileList, const std::string& dir);

int ClearOldFiles(const std::vector<FileInfo> &fileList);

bool ClearFreezeFileIfNeed(uint64_t stackSize);

bool WriteStackToFd(int32_t pid, std::string& path, const std::string& stack,
    const std::string& eventName, bool& isOverLimit);

int64_t GetTimeStamp();

void* FunctionOpen(void* funcHandler, const char* funcName);

int32_t GetUidByPid(const int32_t pid);

int64_t GetAppStartTime(int32_t pid, int64_t tid);

std::map<std::string, int> GetReportTimesMap();

void UpdateReportTimes(const std::string& bundleName, int32_t& times, int32_t& checkInterval);

bool SaveStringToFile(const std::string& path, const std::string& content);

bool IsNum(const std::string& str);

bool GetKeyValueByStr(const std::string& tokens, std::string& key, std::string& value,
    char flag);
} // end of HiviewDFX
} // end of OHOS
#endif
