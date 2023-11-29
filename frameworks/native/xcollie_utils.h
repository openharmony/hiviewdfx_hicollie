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

#include "hilog/log.h"

namespace OHOS {
namespace HiviewDFX {
constexpr char WMS_FULL_NAME[] = "WindowManagerService";
constexpr char IPC_FULL[] = "IPC_FULL";
constexpr char IPC_CHECKER[] = "IpcChecker";
constexpr int64_t SEC_TO_MANOSEC = 1000000000;
constexpr int64_t SEC_TO_MICROSEC = 1000000;
const int BUFF_STACK_SIZE = 20 * 1024;
const int MAX_NAME_SIZE = 128;
constexpr OHOS::HiviewDFX::HiLogLabel g_XCOLLIE_LOG_LABEL = {LOG_CORE, 0xD002D06, "XCollie"};
#define XCOLLIE_LOGF(...) (void)OHOS::HiviewDFX::HiLog::Fatal(g_XCOLLIE_LOG_LABEL, ##__VA_ARGS__)
#define XCOLLIE_LOGE(...) (void)OHOS::HiviewDFX::HiLog::Error(g_XCOLLIE_LOG_LABEL, ##__VA_ARGS__)
#define XCOLLIE_LOGW(...) (void)OHOS::HiviewDFX::HiLog::Warn(g_XCOLLIE_LOG_LABEL, ##__VA_ARGS__)
#define XCOLLIE_LOGI(...) (void)OHOS::HiviewDFX::HiLog::Info(g_XCOLLIE_LOG_LABEL, ##__VA_ARGS__)
#define XCOLLIE_LOGD(...) (void)OHOS::HiviewDFX::HiLog::Debug(g_XCOLLIE_LOG_LABEL, ##__VA_ARGS__)
#define MAGIC_NUM           0x9517
#define HTRANSIO            0xAB
#define LOGGER_GET_STACK    _IO(HTRANSIO, 9)

uint64_t GetCurrentTickMillseconds();

std::string GetSelfProcName();

std::string GetFirstLine(const std::string& path);

std::string GetProcessNameFromProcCmdline(int32_t pid);

std::string GetLimitedSizeName(std::string name);
} // end of HiviewDFX
} // end of OHOS
#endif
