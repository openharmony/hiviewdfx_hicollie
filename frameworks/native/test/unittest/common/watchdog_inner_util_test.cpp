/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <fcntl.h>
#include "file_ex.h"
#include "directory_ex.h"
#include "watchdog_inner_util_test.h"

namespace OHOS {
namespace HiviewDFX {
static bool g_isSelinuxEnabled = false;
void InitSeLinuxEnabled()
{
    constexpr uint32_t BUF_SIZE_64 = 64;
    char buffer[BUF_SIZE_64] = {'\0'};
    FILE* fp = popen("getenforce", "r");
    if (fp != nullptr) {
        fgets(buffer, sizeof(buffer), fp);
        std::string str = buffer;
        printf("buffer is %s\n", str.c_str());
        if (str.find("Enforcing") != str.npos) {
            printf("Enforcing %s\n", str.c_str());
            g_isSelinuxEnabled = true;
        } else {
            printf("This isn't Enforcing %s\n", str.c_str());
        }
        pclose(fp);
    } else {
        printf("fp == nullptr\n");
    }
    system("setenforce 0");

    constexpr mode_t defaultLogDirMode = 0770;
    std::string path = "/data/test/log";
    if (!OHOS::FileExists(path)) {
        OHOS::ForceCreateDirectory(path);
        OHOS::ChangeModeDirectory(path, defaultLogDirMode);
    }
}

void CancelSeLinuxEnabled()
{
    if (g_isSelinuxEnabled) {
        system("setenforce 1");
        g_isSelinuxEnabled = false;
    }
}
} // namespace HiviewDFX
} // namespace OHOS
