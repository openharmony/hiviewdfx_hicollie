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
#include <cstdio>
#include <csignal>
#include <sstream>
#include <securec.h>
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
#include <dirent.h>

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
const int TIMES_ARR_SIZE = 6;
const uint64_t TIMES_AVE_PARAM = 2;
const int32_t APP_MIN_UID = 20000;
const uint64_t START_TIME_INDEX = 21;
const int START_PATH_LEN = 128;
constexpr int64_t MAX_TIME_BUFF = 64;
constexpr int64_t SEC_TO_MILLISEC = 1000;
constexpr int64_t MINUTE_TO_S = 60; // 60s
constexpr size_t TOTAL_HALF = 2; // 2 : remove half of the total
constexpr size_t DEFAULT_LOGSTORE_MIN_KEEP_FILE_COUNT = 100;
constexpr const char* const LOGGER_TEANSPROC_PATH = "/proc/transaction_proc";
constexpr const char* const WATCHDOG_DIR = "/data/storage/el2/log/watchdog/";
constexpr const char* const KEY_DEVELOPER_MODE_STATE = "const.security.developermode.state";
constexpr const char* const KEY_BETA_TYPE = "const.logsystem.versiontype";
constexpr const char* const ENABLE_VAULE = "true";
constexpr const char* const ENABLE_BETA_VAULE = "beta";
constexpr const char* const KEY_REPORT_TIMES_TYPE = "persist.hiview.jank.reporttimes";

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

uint64_t GetCurrentBootMillseconds()
{
    struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = 0;
    clock_gettime(CLOCK_BOOTTIME, &t);
    return static_cast<uint64_t>((t.tv_sec) * SEC_TO_MANOSEC + t.tv_nsec) / SEC_TO_MICROSEC;
}

void CalculateTimes(uint64_t& bootTimeStart, uint64_t& monoTimeStart)
{
    uint64_t timesArr[TIMES_ARR_SIZE] = {0};
    uint64_t minTimeDiff = UINT64_MAX;
    size_t index = 1;

    for (size_t i = 0; i < TIMES_ARR_SIZE ; i++) {
        timesArr[i] = (i & 1) ? GetCurrentTickMillseconds() : GetCurrentBootMillseconds();
        if (i <= 1) {
            continue;
        }
        uint64_t tmpDiff = GetNumsDiffAbs(timesArr[i], timesArr[i - 2]);
        if (tmpDiff < minTimeDiff) {
            minTimeDiff = tmpDiff;
            index = i - 1;
        }
    }
    bootTimeStart = (index & 1) ? (timesArr[index - 1] + timesArr[index + 1]) / TIMES_AVE_PARAM : timesArr[index];
    monoTimeStart = (index & 1) ? timesArr[index] : (timesArr[index - 1] + timesArr[index + 1]) / TIMES_AVE_PARAM;
}

uint64_t GetNumsDiffAbs(const uint64_t& numOne, const uint64_t& numTwo)
{
    return (numOne > numTwo) ? (numOne - numTwo) : (numTwo - numOne);
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
    std::string ret = GetProcessNameFromProcCmdline();
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
    if (pid > 0) {
        g_lock.lock();
        if (!g_curProcName.empty() && g_lastPid == pid) {
            g_lock.unlock();
            return g_curProcName;
        }
    }

    std::string pidStr = pid > 0 ? std::to_string(pid) : "self";
    std::string procCmdlinePath = "/proc/" + pidStr + "/cmdline";
    std::string procCmdlineContent = GetFirstLine(procCmdlinePath);
    if (procCmdlineContent.empty()) {
        if (pid > 0) {
            g_lock.unlock();
        }
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
    if (pid <= 0) {
        return procCmdlineContent.substr(procNameStartPos, endPos);
    }
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
    int32_t uid = GetUidByPid(peerBinderPid);
    if (uid < APP_MIN_UID) {
        XCOLLIE_LOGI("Current peer process can not kill, "
            "peerBinderPid=%{public}d, uid=%{public}d", peerBinderPid, uid);
        return false;
    }

    int32_t ret = kill(peerBinderPid, SIGKILL);
    if (ret == -1) {
        XCOLLIE_LOGI("Kill PeerBinder process failed");
    } else {
        XCOLLIE_LOGI("Kill PeerBinder process success, name=%{public}s, pid=%{public}d",
            GetProcessNameFromProcCmdline(peerBinderPid).c_str(), peerBinderPid);
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

std::vector<FileInfo> GetFilesByDir(const std::string& dirPath)
{
    std::vector<FileInfo> fileInfos;
    DIR* dir = opendir(dirPath.c_str());
    if (dir == nullptr) {
        XCOLLIE_LOGW("open dir failed, dir: %{public}s", dirPath.c_str());
        return fileInfos;
    }
    struct stat st {};
    for (auto* ent = readdir(dir); ent != nullptr; ent = readdir(dir)) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0 ||
            ent->d_type == DT_DIR) {
            continue;
        }
        std::string filePath(dirPath + ent->d_name);
        int err = stat(filePath.c_str(), &st);
        if (err != 0) {
            XCOLLIE_LOGW("%{public}s: Get log stat failed, err(%{public}d).", filePath.c_str(), err);
        } else {
            FileInfo fileInfo = {
                .filePath = filePath,
                .mtime = st.st_mtime
            };
            fileInfos.push_back(fileInfo);
        }
    }
    closedir(dir);
    return fileInfos;
}

int ClearOldFiles(const std::string& dirPath)
{
    std::vector<FileInfo> fileInfos = GetFilesByDir(dirPath);
    size_t fileSize = fileInfos.size();
    if (fileSize != 0) {
        std::sort(fileInfos.begin(), fileInfos.end(), [](FileInfo lfile, FileInfo rfile) {
            return lfile.mtime < rfile.mtime;
        });
        int removeFileNumber = static_cast<int>(fileSize - DEFAULT_LOGSTORE_MIN_KEEP_FILE_COUNT);
        if (removeFileNumber < 0) {
            removeFileNumber = static_cast<int>(fileSize / TOTAL_HALF);
        }
        int deleteCount = 0;
        for (auto it = fileInfos.begin(); it != fileInfos.end(); it++) {
            if (deleteCount >= removeFileNumber) {
                break;
            }
            XCOLLIE_LOGD("Remove file %{public}s.", it->filePath.c_str());
            OHOS::RemoveFile(it->filePath);
            deleteCount++;
        }
        XCOLLIE_LOGI("Remove total count=%{public}d, removeFileNumber=%{public}d, file count=%{public}zu",
            deleteCount, removeFileNumber, fileInfos.size());
        return deleteCount;
    }
    return 0;
}

bool WriteStackToFd(int32_t pid, std::string& path, std::string& stack, const std::string& eventName,
    bool& isOverLimit)
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
        isOverLimit = true;
        XCOLLIE_LOGW("CurrentDir is over limit: %{public}d. Will not write to stack file."
            "MainThread fileSize: %{public}" PRIu64 " MAX_FILE_SIZE: %{public}" PRIu64 ".",
            isOverLimit, fileSize, MAX_FILE_SIZE);
        ClearOldFiles(WATCHDOG_DIR);
        XCOLLIE_LOGI("CurrentDir size: %{public}" PRIu64 "", (OHOS::GetFolderSize(realPath) + stackSize));
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

std::string FormatTime(const std::string &format)
{
    auto now = std::chrono::system_clock::now();
    auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    auto timestamp = millisecs.count();
    std::time_t tt = static_cast<std::time_t>(timestamp / SEC_TO_MILLISEC);
    std::tm t = *std::localtime(&tt);
    char buffer[MAX_TIME_BUFF] = {0};
    std::strftime(buffer, sizeof(buffer), format.c_str(), &t);
    return std::string(buffer);
}

int64_t GetTimeStamp()
{
    std::chrono::nanoseconds ms = std::chrono::duration_cast< std::chrono::nanoseconds >(
        std::chrono::system_clock::now().time_since_epoch());
    return ms.count();
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

int32_t GetUidByPid(const int32_t pid)
{
    std::string uidFlag = "Uid:";
    std::string cmdLinePath = "/proc/" + std::to_string(pid) + "/status";
    std::string realPath = "";
    if (!OHOS::PathToRealPath(cmdLinePath, realPath)) {
        XCOLLIE_LOGE("Path to realPath failed.");
        return -1;
    }
    std::ifstream file(realPath);
    if (!file.is_open()) {
        XCOLLIE_LOGE("open realPath failed.");
        return -1;
    }
    int32_t uid = -1;
    std::string line;
    while (std::getline(file, line)) {
        if (line.compare(0, uidFlag.size(), uidFlag) == 0) {
            std::istringstream iss(line);
            std::string temp;
            if (std::getline(iss, temp, ':') && std::getline(iss, line)) {
                std::istringstream(line) >> uid;
                XCOLLIE_LOGI("get uid is %{public}d.", uid);
                break;
            }
        }
    }
    file.close();
    return uid;
}

int64_t GetAppStartTime(int32_t pid, int64_t tid)
{
    static int32_t startTime = -1;
    static int32_t lastTid = -1;
    if (startTime > 0 && lastTid == tid) {
        return startTime;
    }
    char filePath[START_PATH_LEN] = {0};
    if (snprintf_s(filePath, START_PATH_LEN, START_PATH_LEN - 1, "/proc/%d/task/%d/stat", pid, tid) < 0) {
        XCOLLIE_LOGE("failed to build path, tid=%{public}" PRId64, tid);
    }
    std::string realPath = "";
    if (!OHOS::PathToRealPath(filePath, realPath)) {
        XCOLLIE_LOGE("Path to realPath failed.");
        return startTime;
    }
    std::string content = "";
    OHOS::LoadStringFromFile(realPath, content);
    if (!content.empty()) {
        std::vector<std::string> strings;
        SplitStr(content, " ", strings);
        if (strings.size() <= START_TIME_INDEX) {
            XCOLLIE_LOGE("get startTime failed.");
            return startTime;
        }
        content = strings[START_TIME_INDEX];
        if (std::all_of(std::begin(content), std::end(content), [] (const char &c) {
            return isdigit(c);
        })) {
            startTime = std::stoi(content);
            lastTid = tid;
        }
    }
    return startTime;
}

std::map<std::string, int> GetReportTimesMap()
{
    std::map<std::string, int> keyValueMap;
    std::string reportTimes = system::GetParameter(KEY_REPORT_TIMES_TYPE, "");
    XCOLLIE_LOGD("get reporttimes value is %{public}s.", reportTimes.c_str());
    std::stringstream reportParams(reportTimes);
    std::string line;
    while (getline(reportParams, line, ';')) {
        if (!line.empty()) {
            size_t colonPos = line.find(":");
            if (colonPos == std::string::npos) {
                continue;
            }
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            if (std::all_of(std::begin(value), std::end(value), [] (const char &content) {
                return isdigit(content);
            })) {
                keyValueMap[key] = std::atoi(value.c_str());
            }
        }
    }
    return keyValueMap;
}

void UpdateReportTimes(const std::string& bundleName, int32_t& times, int32_t& checkInterval)
{
    std::map<std::string, int> keyValueMap = GetReportTimesMap();
    auto it = keyValueMap.find(bundleName);
    if (it != keyValueMap.end()) {
        times = it->second / MINUTE_TO_S;
        checkInterval = MINUTE_TO_S * SEC_TO_MILLISEC;
        XCOLLIE_LOGI("get reportTimes value is %{public}d, checkInterval is %{public}d.",
            times, checkInterval);
    }
}
} // end of HiviewDFX
} // end of OHOS