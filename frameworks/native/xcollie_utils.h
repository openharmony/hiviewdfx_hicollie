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
constexpr char IPC_FULL[] = "IPC_FULL";
constexpr uint64_t MIN_APP_UID = 20000;

#define XCOLLIE_LOGF(...) HILOG_FATAL(LOG_CORE, ##__VA_ARGS__)
#define XCOLLIE_LOGE(...) HILOG_ERROR(LOG_CORE, ##__VA_ARGS__)
#define XCOLLIE_LOGW(...) HILOG_WARN(LOG_CORE, ##__VA_ARGS__)
#define XCOLLIE_LOGI(...) HILOG_INFO(LOG_CORE, ##__VA_ARGS__)
#define XCOLLIE_LOGD(...) HILOG_DEBUG(LOG_CORE, ##__VA_ARGS__)
#define MAGIC_NUM           0x9517
#define HTRANSIO            0xAB
#define LOGGER_GET_STACK    _IO(HTRANSIO, 9)

uint64_t GetCurrentTickMillseconds();

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

std::string GetFormatDate();

bool CreateWatchdogDir();

bool WriteStackToFd(int32_t pid, std::string& path, std::string& stack,
    const std::string& eventName);

int64_t GetTimeStamp();

bool IsEnableVersion();

void* FunctionOpen(void* funcHandler, const char* funcName);
} // end of HiviewDFX
} // end of OHOS
#endif
