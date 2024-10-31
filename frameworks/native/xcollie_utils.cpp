/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "xcollie_utils.h"
#include <ctime>
#include <cinttypes>
#include <algorithm>
#include <cstdlib>
#include <csignal>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <set>
#include "directory_ex.h"
#include "file_ex.h"
#include "storage_acl.h"
#include "parameter.h"
#include "parameters.h"
#include <dlfcn.h>

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr int64_t SEC_TO_MANOSEC = 1000000000;
constexpr int64_t SEC_TO_MICROSEC = 1000000;
constexpr uint64_t MAX_FILE_SIZE = 10 * 1024 * 1024; // 10M
const int MAX_NAME_SIZE = 128;
const int MIN_WAIT_NUM = 3;
const int TIME_INDEX_MAX = 32;
const int INIT_PID = 1;
constexpr const char* const LOGGER_TEANSPROC_PATH = "/proc/transaction_proc";
constexpr const char* const WATCHDOG_DIR = "/data/storage/el2/log/watchdog";
constexpr const char* const KEY_ANCO_ENABLE_TYPE = "persist.hmos_fusion_mgr.ctl.support_hmos";
constexpr const char* const KEY_DEVELOPER_MODE_STATE = "const.security.developermode.state";
constexpr const char* const KEY_BETA_TYPE = "const.logsystem.versiontype";
constexpr const char* const ENABLE_VAULE = "true";
constexpr const char* const ENABLE_BETA_VAULE = "beta";
static std::string g_curProcName;
static int32_t g_lastPid;
static std::mutex g_lock;
}
uint64_t GetCurrentTickMillseconds()
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return static_cast<uint64_t>((t.tv_sec) * SEC_TO_MANOSEC + t.tv_nsec) / SEC_TO_MICROSEC;
}

bool IsFileNameFormat(char c)
{
    if (c >= '0' && c <= '9') {
        return false;
    }

    if (c >= 'a' && c <= 'z') {
        return false;
    }

    if (c >= 'A' && c <= 'Z') {
        return false;
    }

    if (c == '.' || c == '-' || c == '_') {
        return false;
    }

    return true;
}

std::string GetSelfProcName()
{
    constexpr uint16_t READ_SIZE = 128;
    std::ifstream fin;
    fin.open("/proc/self/comm", std::ifstream::in);
    if (!fin.is_open()) {
        XCOLLIE_LOGE("fin.is_open() false");
        return "";
    }
    char readStr[READ_SIZE] = {'\0'};
    fin.getline(readStr, READ_SIZE - 1);
    fin.close();

    std::string ret = std::string(readStr);
    ret.erase(std::remove_if(ret.begin(), ret.end(), IsFileNameFormat), ret.end());
    return ret;
}

std::string GetFirstLine(const std::string& path)
{
    char checkPath[PATH_MAX] = {0};
    if (realpath(path.c_str(), checkPath) == nullptr) {
        XCOLLIE_LOGE("canonicalize failed. path is %{public}s", path.c_str());
        return "";
    }

    std::ifstream inFile(checkPath);
    if (!inFile) {
        return "";
    }
    std::string firstLine;
    getline(inFile, firstLine);
    inFile.close();
    return firstLine;
}

bool IsDeveloperOpen()
{
    static std::string isDeveloperOpen;
    if (!isDeveloperOpen.empty()) {
        return (isDeveloperOpen.find(ENABLE_VAULE) != std::string::npos);
    }
    isDeveloperOpen = system::GetParameter(KEY_DEVELOPER_MODE_STATE, "");
    return (isDeveloperOpen.find(ENABLE_VAULE) != std::string::npos);
}

bool IsBetaVersion()
{
    static std::string isBetaVersion;
    if (!isBetaVersion.empty()) {
        return (isBetaVersion.find(ENABLE_BETA_VAULE) != std::string::npos);
    }
    isBetaVersion = system::GetParameter(KEY_BETA_TYPE, "");
    return (isBetaVersion.find(ENABLE_BETA_VAULE) != std::string::npos);
}

std::string GetProcessNameFromProcCmdline(int32_t pid)
{
    if (!g_curProcName.empty() && g_lastPid == pid) {
        return g_curProcName;
    }
    std::string procCmdlinePath = "/proc/" + std::to_string(pid) + "/cmdline";
    std::string procCmdlineContent = GetFirstLine(procCmdlinePath);
    if (procCmdlineContent.empty()) {
        return "";
    }

    size_t procNameStartPos = 0;
    size_t procNameEndPos = procCmdlineContent.size();
    for (size_t i = 0; i < procCmdlineContent.size(); i++) {
        if (procCmdlineContent[i] == '/') {
            procNameStartPos = i + 1;
        } else if (procCmdlineContent[i] == '\0') {
            procNameEndPos = i;
            break;
        }
    }
    size_t endPos = procNameEndPos - procNameStartPos;
    g_lock.lock();
    g_curProcName = procCmdlineContent.substr(procNameStartPos, endPos);
    g_lastPid = pid;
    g_lock.unlock();
    XCOLLIE_LOGD("g_curProcName is empty, name %{public}s pid %{public}d", g_curProcName.c_str(), pid);
    return g_curProcName;
}

std::string GetLimitedSizeName(std::string name)
{
    if (name.size() > MAX_NAME_SIZE) {
        return name.substr(0, MAX_NAME_SIZE);
    }
    return name;
}

bool IsProcessDebug(int32_t pid)
{
    const int buffSize = 128;
    char paramBundle[buffSize] = {0};
    GetParameter("hiviewdfx.appfreeze.filter_bundle_name", "", paramBundle, buffSize - 1);
    std::string debugBundle(paramBundle);
    std::string procCmdlineContent = GetProcessNameFromProcCmdline(pid);
    if (procCmdlineContent.compare(debugBundle) == 0) {
        XCOLLIE_LOGI("appfreeze filtration %{public}s_%{public}s don't exit.",
            debugBundle.c_str(), procCmdlineContent.c_str());
        return true;
    }
    return false;
}

void DelayBeforeExit(unsigned int leftTime)
{
    while (leftTime > 0) {
        leftTime = sleep(leftTime);
    }
}

std::string TrimStr(const std::string& str, const char cTrim)
{
    std::string strTmp = str;
    strTmp.erase(0, strTmp.find_first_not_of(cTrim));
    strTmp.erase(strTmp.find_last_not_of(cTrim) + sizeof(char));
    return strTmp;
}

void SplitStr(const std::string& str, const std::string& sep,
    std::vector<std::string>& strs, bool canEmpty, bool needTrim)
{
    strs.clear();
    std::string strTmp = needTrim ? TrimStr(str) : str;
    std::string strPart;
    while (true) {
        std::string::size_type pos = strTmp.find(sep);
        if (pos == std::string::npos || sep.empty()) {
            strPart = needTrim ? TrimStr(strTmp) : strTmp;
            if (!strPart.empty() || canEmpty) {
                strs.push_back(strPart);
            }
            break;
        } else {
            strPart = needTrim ? TrimStr(strTmp.substr(0, pos)) : strTmp.substr(0, pos);
            if (!strPart.empty() || canEmpty) {
                strs.push_back(strPart);
            }
            strTmp = strTmp.substr(sep.size() + pos, strTmp.size() - sep.size() - pos);
        }
    }
}

int ParsePeerBinderPid(std::ifstream& fin, int32_t pid)
{
    const int decimal = 10;
    std::string line;
    bool isBinderMatchup = false;
    while (getline(fin, line)) {
        if (isBinderMatchup) {
            break;
        }

        if (line.find("async\t") != std::string::npos) {
            continue;
        }

        std::istringstream lineStream(line);
        std::vector<std::string> strList;
        std::string tmpstr;
        while (lineStream >> tmpstr) {
            strList.push_back(tmpstr);
        }

        auto splitPhase = [](const std::string& str, uint16_t index) -> std::string {
            std::vector<std::string> strings;
            SplitStr(str, ":", strings);
            if (index < strings.size()) {
                return strings[index];
            }
            return "";
        };

        if (strList.size() >= 7) { // 7: valid array size
            // 2: peer id,
            std::string server = splitPhase(strList[2], 0);
            // 0: local id,
            std::string client = splitPhase(strList[0], 0);
            // 5: wait time, s
            std::string wait = splitPhase(strList[5], 1);
            if (server == "" || client == "" || wait == "") {
                continue;
            }
            int serverNum = std::strtol(server.c_str(), nullptr, decimal);
            int clientNum = std::strtol(client.c_str(), nullptr, decimal);
            int waitNum = std::strtol(wait.c_str(), nullptr, decimal);
            XCOLLIE_LOGI("server:%{public}d, client:%{public}d, wait:%{public}d",
                serverNum, clientNum, waitNum);
            if (clientNum != pid || waitNum < MIN_WAIT_NUM) {
                continue;
            }
            return serverNum;
        }
        if (line.find("context") != line.npos) {
            isBinderMatchup = true;
        }
    }
    return -1;
}

bool KillProcessByPid(int32_t pid)
{
    std::ifstream fin;
    std::string path = std::string(LOGGER_TEANSPROC_PATH);
    char resolvePath[PATH_MAX] = {0};
    if (realpath(path.c_str(), resolvePath) == nullptr) {
        XCOLLIE_LOGI("GetBinderPeerPids realpath error");
        return false;
    }
    fin.open(resolvePath);
    if (!fin.is_open()) {
        XCOLLIE_LOGI("open file failed, %{public}s.", resolvePath);
        return false;
    }

    int peerBinderPid = ParsePeerBinderPid(fin, pid);
    fin.close();
    if (peerBinderPid <= INIT_PID || peerBinderPid == pid) {
        XCOLLIE_LOGI("No PeerBinder process freeze occurs in the current process. "
            "peerBinderPid=%{public}d, pid=%{public}d", peerBinderPid, pid);
        return false;
    }

    XCOLLIE_LOGI("try to Kill PeerBinder process, name=%{public}s, pid=%{public}d",
        GetProcessNameFromProcCmdline(peerBinderPid).c_str(), peerBinderPid);
    int32_t ret = kill(peerBinderPid, SIGKILL);
    if (ret == -1) {
        XCOLLIE_LOGI("Kill PeerBinder process failed");
    }
    return (ret >= 0);
}

bool CreateWatchdogDir()
{
    constexpr mode_t defaultLogDirMode = 0770;
    if (!OHOS::FileExists(WATCHDOG_DIR)) {
        OHOS::ForceCreateDirectory(WATCHDOG_DIR);
        OHOS::ChangeModeDirectory(WATCHDOG_DIR, defaultLogDirMode);
    }
    if (OHOS::StorageDaemon::AclSetAccess(WATCHDOG_DIR, "g:1201:rwx") != 0) {
        XCOLLIE_LOGI("Failed to AclSetAccess");
        return false;
    }
    return true;
}

bool WriteStackToFd(int32_t pid, std::string& path, std::string& stack, const std::string& eventName)
{
    if (!CreateWatchdogDir()) {
        return false;
    }
    std::string time = GetFormatDate();
    std::string realPath;
    if (!OHOS::PathToRealPath(WATCHDOG_DIR, realPath)) {
        XCOLLIE_LOGE("Path to realPath failed.");
        return false;
    }
    path = realPath + "/" + eventName + "_" + time.c_str() + "_" +
        std::to_string(pid).c_str() + ".txt";
    uint64_t stackSize = stack.size();
    uint64_t fileSize = OHOS::GetFolderSize(realPath) + stackSize;
    if (fileSize > MAX_FILE_SIZE) {
        XCOLLIE_LOGE("CurrentDir already over limit. Will not write to stack file."
            "MainThread fileSize: %{public}" PRIu64 " MAX_FILE_SIZE: %{public}" PRIu64 ".",
            fileSize, MAX_FILE_SIZE);
        return true;
    }
    constexpr mode_t defaultLogFileMode = 0644;
    auto fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, defaultLogFileMode);
    if (fd < 0) {
        XCOLLIE_LOGE("Failed to create path");
        return false;
    } else {
        XCOLLIE_LOGE("path=%{public}s", path.c_str());
    }
    OHOS::SaveStringToFd(fd, stack);
    close(fd);

    return true;
}

std::string GetFormatDate()
{
    time_t t = time(nullptr);
    char tmp[TIME_INDEX_MAX] = {0};
    strftime(tmp, sizeof(tmp), "%Y%m%d%H%M%S", localtime(&t));
    std::string date(tmp);
    return date;
}

int64_t GetTimeStamp()
{
    std::chrono::nanoseconds ms = std::chrono::duration_cast< std::chrono::nanoseconds >(
        std::chrono::system_clock::now().time_since_epoch());
    return ms.count();
}

bool IsEnableVersion()
{
    auto enableType = system::GetParameter(KEY_ANCO_ENABLE_TYPE, "");
    return (enableType.find(ENABLE_VAULE) != std::string::npos);
}

void* FunctionOpen(void* funcHandler, const char* funcName)
{
    dlerror();
    char* err = nullptr;
    void* func = dlsym(funcHandler, funcName);
    err = dlerror();
    if (err != nullptr) {
        XCOLLIE_LOGE("dlopen %{public}s failed. %{public}s\n", funcName, err);
        return nullptr;
    }
    return func;
}
} // end of HiviewDFX
} // end of OHOS