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

#ifndef RELIABILITY_THREAD_SAMPLER_H
#define RELIABILITY_THREAD_SAMPLER_H

#include <atomic>
#include <condition_variable>
#include <memory>
#include <queue>
#include <set>
#include <string>

#include <sys/mman.h>

#include "singleton.h"

#include "unwinder.h"
#include "unique_stack_table.h"
#include "dfx_accessors.h"
#include "dfx_maps.h"
#include "unwind_context.h"

namespace OHOS {
namespace HiviewDFX {
constexpr int STACK_BUFFER_SIZE = 16 * 1024;
constexpr uint32_t DEFAULT_UNIQUE_STACK_TABLE_SIZE = 128 * 1024;

struct ThreadUnwindContext {
    uintptr_t pc {0};
    uintptr_t sp {0};
    uintptr_t fp {0};
    uintptr_t lr {0};
    uint64_t requestTime {0}; // begin sample
    uint64_t snapshotTime {0}; // end of stack copy in signal handler
    uint64_t processTime {0}; // end of unwind and unique stack
    uint8_t buffer[STACK_BUFFER_SIZE] {0}; // 16K stack buffer
};

struct UnwindInfo {
    ThreadUnwindContext* context;
    DfxMaps* maps;
};

struct TimeAndFrames {
    uint64_t requestTime {0};
    uint64_t snapshotTime {0};
    std::vector<DfxFrame> frameList;
};

class ThreadSampler : public Singleton<ThreadSampler> {
    DECLARE_SINGLETON(ThreadSampler);
public:
    static const int32_t SAMPLER_MAX_BUFFER_SZ = 11;
    static void ThreadSamplerSignalHandler(int sig, siginfo_t* si, void* context);

    // Initial sampler, include uwinder, recorde buffer etc.
    bool Init();
    int32_t Sample();   // Interface of sample, to send sample request.
    // Collect stack info, can be formed into tree format or not. Unsafe in multi-thread environments
    bool CollectStack(std::string& stack, bool treeFormat = true);
    void Deinit();  // Release sampler
    // use to set the size and mmap buffer name of the uniqueStackTable
    bool SetUniStackTableSize(uint32_t uniStkTableSz);

    bool SetUniStackTableMMapName(const std::string& uniTableMMapName);
    uint32_t GetuniqueStackTableSize()
    {
        return uniqueStackTableSize_;
    }

    std::string GetUniTableMMapName()
    {
        return uniTableMMapName_;
    }

private:
    bool InitRecordBuffer();
    void ReleaseRecordBuffer();
    bool InitUnwinder();
    void DestroyUnwinder();
    bool InitUniqueStackTable();
    void DeinitUniqueStackTable();
    bool InstallSignalHandler();
    void UninstallSignalHandler();
    void SendSampleRequest();
    void ProcessStackBuffer(bool treeFormat);
    int AccessElfMem(uintptr_t addr, uintptr_t *val);

    static int FindUnwindTable(uintptr_t pc, UnwindTableInfo& outTableInfo, void *arg);
    static int AccessMem(uintptr_t addr, uintptr_t *val, void *arg);
    static int GetMapByPc(uintptr_t pc, std::shared_ptr<DfxMap>& map, void *arg);

    ThreadUnwindContext* GetReadContext();
    ThreadUnwindContext* GetWriteContext();
    void WriteContext(void* context);
#if defined(CONSUME_STATISTICS)
    void ResetConsumeInfo();
#endif // #if defined(CONSUME_STATISTICS)
    bool init_ = false;
    uintptr_t stackBegin_ = 0;
    uintptr_t stackEnd_ = 0;
    int32_t pid_ = 0;
    std::atomic<int32_t> writeIndex_ {0};
    std::atomic<int32_t> readIndex_ {0};
    void* mmapStart_ = MAP_FAILED;
    int32_t bufferSize_ = 0;
    std::shared_ptr<Unwinder> unwinder_ = nullptr;
    std::unique_ptr<UniqueStackTable> uniqueStackTable_ = nullptr;
    std::shared_ptr<UnwindAccessors> accessors_ = nullptr;
    std::shared_ptr<DfxMaps> maps_ = nullptr;
    // size of the uniqueStackTableSize, default 128KB
    uint32_t uniqueStackTableSize_ = DEFAULT_UNIQUE_STACK_TABLE_SIZE;
    // name of the mmap of uniqueStackTable
    std::string uniTableMMapName_ = "hicollie_buf";
#if defined(CONSUME_STATISTICS)
    uint64_t threadCount_ = 0;
    uint64_t threadTimeCost_ = 0;
    uint64_t unwinderCount_ = 0;
    uint64_t unwinderTimeCost_ = 0;
    uint64_t overallTimeCost_ = 0;
    uint64_t sampleCount_ = 0;
    uint64_t requestCount_ = 0;
    uint64_t processCount_ = 0;
#endif // #if defined(CONSUME_STATISTICS)

    std::vector<TimeAndFrames> timeAndFrameList_;
    std::map<uint64_t, std::vector<uint64_t>> stackIdTimeMap_;
};
} // end of namespace HiviewDFX
} // end of namespace OHOS
#endif
