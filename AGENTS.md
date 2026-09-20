# HiCollie 组件指引

## 项目定位

本仓库对应 OpenHarmony `base/hiviewdfx/hicollie`，属 DFX 子系统下的软件看门狗组件。提供统一的故障检测与故障日志生成框架，用于定位系统服务死锁、应用主线程阻塞、业务流程超时等软件超时故障。优先按这些目录定位问题：

- `frameworks/native/`：核心实现。看门狗线程、任务优先队列、XCollie 定时器、jank/scroll 检测、IPC-full 检测、freeze 栈采样、FFRT 看门狗、trace 采集。
- `frameworks/native/thread_sampler/`：独立动态库 `libthread_sampler.z.so`，基于信号的主线程栈采样引擎。
- `frameworks/app/`：应用侧状态镜像（`AppWatchdog`），不启动看门狗线程，仅持有 bundle/foreground/systemApp/scroll 等状态。
- `interfaces/native/innerkits/include/xcollie/`：Native 内部 API 头文件（`XCollie`、`Watchdog`、`IpcFull`、`process_kill_reason.h`）。
- `interfaces/app/include/`：应用内部 API 头文件（`AppWatchdog`）。
- `interfaces/ndk/`：NDK 公共 C ABI（`OH_HiCollie_*` 系列，`@kit PerformanceAnalysisKit`）。
- `interfaces/rust/`：Rust 绑定（转调 `libhicollie` 的 `SetTimerRust/CancelTimerRust`）。
- `hisysevent.yaml`、`bundle.json`、`hicollie.gni`：事件定义、组件清单、GN 构建开关。

### 按任务类型定位代码

| 任务类型 | 首选目录/文件 | 关键文件 |
| --- | --- | --- |
| 修改 XCollie 一次性定时器 API | `frameworks/native/` | `xcollie.cpp`, `watchdog_inner.cpp` (`RunXCollieTask`/`RemoveXCollieTask`) |
| 修改看门狗主循环/任务调度 | `frameworks/native/` | `watchdog_inner.h/cpp` (`Start`, `FetchNextTask`, `checkerQueue_`) |
| 修改 Watchdog 事件处理器检查（`AddThread`） | `frameworks/native/` | `handler_checker.h/cpp`, `watchdog_inner.cpp` (`AddThread`) |
| 修改主线程 jank / scroll jank 检测 | `frameworks/native/` | `watchdog_inner.cpp` (`StartProfileMainThread`, `StartScrollProfile`, `DistributeStart/End`) |
| 修改主线程 freeze 栈采样 | `frameworks/native/` | `watchdog_inner.cpp` (`StartSample`, `StopSample`, `SaveFreezeStackToFile`), `xcollie_mgr.cpp` |
| 修改 trace 采集（hiview 联动） | `frameworks/native/` | `xcollie_ffrt_task.h/cpp`, `watchdog_inner.cpp` (`CollectTraceDetect`) |
| 修改 FFRT 看门狗 | `frameworks/native/` | `watchdog_inner.cpp` (`InitFfrtWatchdog`, `FfrtCallback`) |
| 修改 IPC-full / 异步 binder 空间检测 | `frameworks/native/` | `watchdog_inner.cpp` (`IpcCheck`, `AddIpcFull`), `watchdog_task.cpp` (`AsyncBinderSpace`), `ipc_full.cpp` |
| 修改 kill 进程 / 退出流程 | `frameworks/native/` | `watchdog_inner.cpp` (`KillPeerBinderProcess`, `LeftTimeExitProcess`), `xcollie_utils.cpp` (`KillProcessByPid`) |
| 修改栈采样引擎 | `frameworks/native/thread_sampler/` | `thread_sampler.cpp`, `thread_sampler_utils.cpp`, `thread_sampler_api.cpp` |
| 修改应用侧状态镜像 | `frameworks/app/` | `app_watchdog_inner.cpp`, `app_watchdog_utils.cpp` |
| 修改进程查杀原因映射 | `frameworks/native/` | `process_kill_reason.cpp` 及对应头文件 |
| 修改 sample stack 缓存 | `frameworks/native/` | `sample_stack_map.h/cpp` |
| 修改通用工具/proc 解析/binder 信息 | `frameworks/native/` | `xcollie_utils.h/cpp` |
| 修改 Native 内部 API 签名 | `interfaces/native/innerkits/include/xcollie/` | `xcollie.h`, `xcollie_define.h`, `watchdog.h`, `ipc_full.h` |
| 修改 NDK 公共 API 实现 | `interfaces/ndk/` | `hicollie.cpp`, `include/hicollie.h` |
| 修改 Rust 绑定 | `interfaces/rust/src/` | `lib.rs` |
| 修改 HiSysEvent 事件定义 | 根目录 | `hisysevent.yaml` |
| 修改构建开关/特性宏 | 根目录 | `hicollie.gni`, `bundle.json` |
| 修改动态库导出符号 | 各 `*.map` 文件 | `libhicollie.map`, `libapp_hicollie.map`, `libohhicollie.map`, `libthread_sampler.map` |

### 嵌套指引

本仓库无目录级别的嵌套指引。所有任务级指导均通过本文件及源码注释提供。

## 构建和验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

```sh
# 编译整个 hicollie 组件
./build.sh --product-name rk3568 --build-target hicollie --ccache

# 单独构建产物（来自 bundle.json build.sub_component）
#   interfaces/app:libapp_hicollie
#   interfaces/native/innerkits:libhicollie
#   frameworks/native/thread_sampler:libthread_sampler
#   interfaces/rust:hicollie_rust
#   interfaces/ndk:ohhicollie
prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 libhicollie
```

### 测试

测试目标定义于 `bundle.json` 的 `build.test`：
- `frameworks/app/test/unittest:unittest`
- `frameworks/native/test/unittest/common:unittest`
- `interfaces/ndk/test/unittest:unittest`

```sh
prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 WatchdogInnerUnitTest
prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 AppWatchdogUnitTest
prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 HiCollieNDKUnitTest
```

测试使用 GoogleTest 框架，命名约定 `HWTEST_F(<SuiteName>, <Suite>_<Case>_<序号>, TestSize.Level1)`，注释块带 `@tc.name/@tc.desc/@tc.type`。部分测试通过 `#define private public` 访问内部成员。

### 完成标准

任务被认为完成，当且仅当：

1. **代码改动已提交** - 使用 `git commit -s`，多代理协作时添加 `Co-Authored-By: Agent`
2. **本地构建通过** - 执行上述构建命令
3. **相关单元测试通过** - 对应 `unittest` 目标通过
4. **板侧验证（如适用）** - 涉及看门狗触发、进程退出、jank 上报、栈采样、trace 采集的改动需提供板侧证据（日志、hdc 输出、HiSysEvent 事件）
5. **文档/事件定义更新（如适用）** - 新增/修改事件需同步 `hisysevent.yaml`；新增 kill 原因需同步 `process_kill_reason.cpp` 的 `KILL_REASON_CONFIG` 表

### 如果无法运行验证

明确说明无法运行的原因，列出推荐的验证步骤供人工执行，标记需要人工验证的部分。

### 完成报告格式

报告应包含：改动摘要（文件列表、改动点）、验证结果（构建/测试输出）、风险评估（API 兼容性、稳定性、性能风险）、未完成事项。

## 知识索引

本仓库无独立 `docs/knowledge/` 目录。稳定背景知识分散于源码注释与下列核心实现文件，改动前应按场景读取对应文件：

### 场景与路径路由

| 场景 | 修改目录 | 先读文件 |
| --- | --- | --- |
| XCollie 定时器生命周期（注册/触发/取消/回调限频/恢复退出） | `frameworks/native/` | `xcollie.cpp`, `watchdog_inner.cpp` (`RunXCollieTask`/`DoCallback`/`RemoveXCollieTask`), `watchdog_task.cpp` (`DoCallback`, `SendXCollieEvent`) |
| 看门狗主线程调度、优先队列、周期/一次性任务 | `frameworks/native/` | `watchdog_inner.cpp` (`Start`, `FetchNextTask`, `ReInsertTaskIfNeed`, `InsertWatchdogTaskLocked`), `watchdog_task.h/cpp` |
| 事件处理器检查（`HandlerChecker` 三态机：COMPLETED/WAITED_HALF/WAITING） | `frameworks/native/` | `handler_checker.h/cpp`, `watchdog_task.cpp` (`EvaluateCheckerState`, `HandleWaitedHalfState`, `HandleWaitedFullState`) |
| 主线程 jank / scroll jank 检测、Looper Watcher | `frameworks/native/` | `watchdog_inner.cpp` (`InitMainLooperWatcher`, `DistributeStart/End`, `StartProfileMainThread`, `StartScrollProfile`, `SampleStackDetect`) |
| 主线程 freeze 栈采样、freeze 回调 | `frameworks/native/` | `watchdog_inner.cpp` (`StartSample`/`StopSample`), `xcollie_mgr.cpp` (`ReadDataFromBuffer`, `CheckCallDuration`) |
| 栈采样信号引擎、unwind、unique-stack 表 | `frameworks/native/thread_sampler/` | `thread_sampler.cpp` (`Init`, `Sample`, `WriteContext`, `ProcessStackBuffer`, `CollectStack`), `thread_sampler_utils.cpp` |
| FFRT 看门狗、trace 采集 | `frameworks/native/` | `watchdog_inner.cpp` (`InitFfrtWatchdog`, `FfrtCallback`), `xcollie_ffrt_task.h/cpp` |
| IPC-full / 异步 binder 空间满检测 | `frameworks/native/` | `watchdog_inner.cpp` (`IpcCheck`, `AddIpcFull`, `AsyncBinderSpaceFull`), `watchdog_task.cpp` (`AsyncBinderSpace`, `IsBinderSpaceInsufficient`) |
| 进程查杀、`LeftTimeExitProcess`、peer binder 查杀 | `frameworks/native/` | `watchdog_inner.cpp` (`KillPeerBinderProcess`, `LeftTimeExitProcess`), `xcollie_utils.cpp` (`KillProcessByPid`, `GetBinderPeerPids`, `ParseBinderCallChain`) |
| 进程查杀原因码与上报 | `frameworks/native/` | `process_kill_reason.cpp` 及头文件 (`KILL_REASON_CONFIG`, `FindEntry`, `GetKillReason/GetAppExitReason`) |
| NDK 上报（`OH_HiCollie_Report` 等）、`AppMgrClient::NotifyAppFault` | `interfaces/ndk/` | `hicollie.cpp` (`Report`, `ReportInputBlock`, `OH_HiCollie_SetTimer` 等) |
| HiSysEvent 上报、事件名选择、栈裁剪重试 | `frameworks/native/` | `watchdog_task.cpp` (`SendEvent`, `SendXCollieEvent`, `SendHisyseventEvent`), 根目录 `hisysevent.yaml` |
| 构建开关与特性宏 | 根目录 | `hicollie.gni`, 各 `BUILD.gn` 中的 `defines`/`cflags` |

### 开始编辑前

在修改代码前，按以下顺序确认：
1. 确认任务类别（参考上面的任务类型表）
2. 根据上表确定需要阅读的源码文件
3. 根据"项目约束"确认不违反任何约束
4. 声明："我将修改 X，已阅读 Y 文件，遵循 Z 约束"

## 项目约束

### 性能约束

- `DistributeStart`/`DistributeEnd` 是主线程事件分发的高频回调，不要在其中增加全量扫描、字符串格式化、`INFO` 级日志或同步磁盘 I/O。jank 判定阈值默认 150 ms（`SAMPLE_DEFAULT_INTERVAL`），任何附加开销都可能造成误报或漏报。
- 栈采样信号处理函数 `ThreadSamplerSignalHandler`/`WriteContext` 在异步信号上下文执行，禁止调用非异步信号安全函数（如 `malloc`、`std::mutex`、`HiLog`、`std::string` 构造）。相关实现已用 `__attribute__((no_sanitize("address","hwaddress")))` 标注，改动时需保持。
- 看门狗主循环 `Start()` 的 `condition_.wait_for` 精度依赖任务 `nextTickTime`，不要在循环内引入未受限的阻塞调用。
- XCollie 定时器单进程上限 `MAX_WATCH_NUM = 128`，超出直接返回 `INVALID_ID`。新增默认定时器任务时评估对配额的占用。

### 架构约束

- 单例门面（facade）分层：`XCollie`/`Watchdog`/`IpcFull`（`interfaces/native/innerkits`）→ `WatchdogInner::GetInstance()`；`AppWatchdog` → `AppWatchdogInner`；`OH_HiCollie_*` NDK → `Watchdog`/`WatchdogInner` 或 `AppMgrClient`。所有定时器、周期任务、handler 检查最终汇入同一个 `WatchdogInner` 的 `checkerQueue_`。新增入口 API 时遵循同样的门面→Inner 分层，不要在门面层做业务逻辑。
- `WatchdogInner` 的看门狗线程只通过 `std::call_once(flag_, ...)` 创建一次。不要新增并行看门狗线程；如需并行，使用 FFRT 队列（参考 `XCollieFfrtTask`）。
- `AppWatchdog` 是**仅状态**镜像，不启动看门狗线程；应用进程通过 NDK + `AppMgrClient::NotifyAppFault` 上报。不要在 `AppWatchdogInner` 中引入定时逻辑。
- `libthread_sampler.z.so` 与 `libasync_stack.z.so` 通过 `dlopen`/`dlsym` 懒加载（`InitThreadSamplerFuncs`、`InitAsyncStackIfNeed`），缺失时需优雅降级。新增依赖必须保留 `FunctionOpen` 的判空路径。
- `WatchdogTask` 的 `operator<` 是**反序**实现以适配 `std::priority_queue` 的小顶堆语义，修改时勿改比较方向。
- `CORE_PROCS`（`anco_service_broker`、`aptouch_daemon`、`foundation`、`init`、`multimodalinput`、`com.ohos.sceneboard`、`render_service`）的 `XCOLLIE_FLAG_RECOVERY` 定时器上报 `SERVICE_TIMEOUT` 并退出，非核心进程上报 `APP_HICOLLIE`/`SERVICE_TIMEOUT_WARNING`。修改事件名选择逻辑时需保持这一区分。
- IPC-full 任务仅对白名单 UID `{AUDIO_SERVER_UID=1041, DATA_MANAGE_SERVICE_UID=3012, FOUNDATION_UID=5523, RENDER_SERVICE_UID=1003}` 或进程名 `com.ohos.sceneboard` 注册（`JOIN_IPC_FULL_UIDS`）。不要扩展白名单而不评估影响。
- `KICK_WATCHDOG` 默认仅 `FOUNDATION_UID=5523`，`KICK_WATCHDOG_ENABLE` 时追加 `MEDIA_SERVICE_UID=1013`。写入 `/sys/kernel/hungtask/userlist` 或 `/proc/sys/hguard/user_list`。

### 编码约定

- C++ 改动优先复用项目内的 `XCOLLIE_LOGF/E/W/I/D` 宏（定义于 `xcollie_utils.h`、`thread_sampler_utils.h`、`app_watchdog_utils.h`，domain `0xD002D06`，tag `"XCollie"`）。`XCOLLIE_KLOGI/E` 同时写 hilog 与内核 log，仅限需要内核侧可见的日志。
- 日志参数必须使用 HiLog 隐私过滤占位符（`%{public}s`、`%{public}d`、`%{public}zu` 等），否则会被掩码。涉及 PID/UID/进程名/bundle 名需谨慎评估是否标 `public`。
- 返回值约定：NDK C ABI 返回 `HiCollie_ErrorCode`（`HICOLLIE_SUCCESS=0`，其余 `401`/`29800001..29800007`）；Native 内部 API 返回 `int` 定时器 id（`>0` 成功，`INVALID_ID=-1` 参数非法，`0` 配额满或重名），或 `bool`/`std::string`。新增 API 需遵循同套约定。
- 参数校验模式：`name.empty() || handler == nullptr`、`timeout == 0`、`priority < PRIORITY_MIN || priority > PRIORITY_MAX`、`interval < MIN_IPC_CHECK_INTERVAL || interval > MAX_IPC_CHECK_INTERVAL` 等就近判空后 `XCOLLIE_LOGE` + 提前返回。新增 API 必须在入口校验。
- 内存安全：`new(std::nothrow)` + 判空；`strncpy_s`/`snprintf_s`/`memset_s` 后检查 `ret != EOK`（bounds_checking_function）；fd 使用 `fdsan_exchange_owner_tag`/`fdsan_close_with_tag` 标注归属。
- C++ 改动进行指针判空时，不要使用 `CHKPV*`、`CHKPR*` 等会改变控制流的宏，使用显式 `if (ptr == nullptr) { ... }`。
- 异步信号安全函数 `WriteContext` 必须保留 `NO_SANITIZER` 标注和 `release`/`acquire` 内存序；不要在信号处理中调用 `XCOLLIE_LOG*` 或 `malloc`。
- 命名空间统一 `namespace OHOS { namespace HiviewDFX { ... } }`，文件内常量放匿名 `namespace { ... }`，结束加 `// end of namespace HiviewDFX` / `// end of namespace OHOS`。头文件保护宏形如 `RELIABILITY_<NAME>_H`。

### 公共 API 约束

**Do not（禁止）：**
- 修改已发布的 NDK 接口（`interfaces/ndk/include/hicollie.h` 中 `OH_HiCollie_*`）的签名、参数类型、返回值类型、错误码枚举值
- 修改 NDK 接口的 `@since` 版本语义或行为语义（如同步变异步、阻塞变非阻塞）
- 修改 Native 内部 API（`interfaces/native/innerkits/include/xcollie/*`）已导出符号的签名（`libhicollie.map` 锁定的导出名）
- 删除或重命名已有公共 API；修改已有 API 的错误码（`HiCollie_ErrorCode` 枚举值）
- 修改 `libhicollie.map`/`libohhicollie.map`/`libapp_hicollie.map`/`libthread_sampler.map` 已导出符号
- 修改 `process_kill_reason.cpp` 中 `KILL_REASON_CONFIG` 已有 id 的映射字符串（影响上游依赖的故障归类）

**Ask before（修改前必须确认）：**
- 新增 NDK 公共 API：确认是否需要 `HICOLLIE_ENABLE_API_METRICS` 直方图埋点、是否需要 `@since` 版本、是否需要新增错误码
- 新增 Native 内部 API：评估是否需要写入 `.map` 导出，是否影响跨模块兼容性
- 新增/修改 HiSysEvent 事件：必须同步 `hisysevent.yaml`，评估是否影响下游（hiview）解析
- 修改 NDK 接口的错误处理逻辑：确认是否影响应用层错误码兼容性
- 新增 kill 原因 id：必须在对应分段区间内（App Exit 1–1000、User 1000–2000、Crash 2000–3000、System 3000–4000、Kernel 4000–5000），且保持 `KILL_REASON_CONFIG` 升序，否则 `static_assert(IsConfigSorted())` 会编译失败

### 安全与权限边界

**Do not（禁止）：**
- 绕过 `IsProcessDebug(pid)`（读取 `hiviewdfx.appfreeze.filter_bundle_name`）的过滤逻辑，导致调试态进程被误上报
- 绕过 `IsInAppspwan()` 的 spawn 进程早退过滤
- 在未验证的情况下直接使用跨进程传递的 fd、共享内存（freeze 回调 mmap 的 64 KB 缓冲、`/proc/transaction_proc` 解析结果）
- 将进程敏感信息（PID/UID/bundle 名/进程名/启动时间）写入非安全日志或非 `public` 占位符的日志字段
- 修改 `KillProcessByPid`/`LeftTimeExitProcess` 的触发条件或退出码而不经安全评审；当前仅对 `uid >= APP_MIN_UID (20000)` 的 peer binder 进程 `kill(SIGKILL)`

**Ask before（修改前必须确认）：**
- 修改 `KillProcessByPid` 的 peer binder 选择策略
- 修改 `IPCDfx::SetIPCProxyLimit` 阈值或 `IPCDfx::BlockUntilThreadAvailable` 行为
- 修改 `IsOversea()` 分支（海外版本跳过 binder info 采集，涉及隐私合规）
- 修改 `/proc/transaction_proc`、`/proc/<pid>/unexpected_die_catch`、`/sys/kernel/hungtask/userlist`、`/proc/sys/hguard/user_list`、`/dev/bbox` 等节点读写
- 修改 `CreateDir` 的 ACL 设置（`StorageDaemon::AclSetAccess(dirPath, "g:1201:rwx")`）

### 协议与数据格式兼容性

**Do not（禁止）：**
- 修改 `hisysevent.yaml` 中已有事件的字段名、字段类型、字段顺序
- 修改 `hisysevent.yaml` 已有事件的 `__BASE` 的 `type`/`level`/`tag`（FAULT/CRITICAL/STABILITY）
- 修改 `WatchdogTask::SendEvent`/`SendXCollieEvent` 中已有事件的字段写入顺序与字段集
- 修改 `MUSL_SIGNAL_SAMPLE_STACK` 信号编号或 `ThreadSamplerSignalHandler` 的入参约定（与 musl 约定）

**Ask before（修改前必须确认）：**
- 新增 HiSysEvent 事件：必须同步 `hisysevent.yaml`，确认是否影响下游 hiview 的事件订阅
- 修改事件 payload：确认 `ERR_OVER_SIZE` 重试裁剪逻辑是否仍适用
- 新增 IPC 检查白名单 UID：评估对系统稳定性监控的影响
- 修改 `sample_interval`/`sample_count`/`report_times` 的合法区间（当前 50–500 / 1..(MAX/interval-4) / 1–3）

### 生成代码边界

本仓库不使用 IDL 编译器生成代码，无生成代码边界。但存在动态加载边界：

- `libthread_sampler.z.so`、`libasync_stack.z.so` 通过 `dlopen`/`dlsym` 加载，符号名变更必须同步加载侧（`InitThreadSamplerFuncs`/`InitAsyncStackIfNeed`）的 `FunctionOpen` 调用与 `libthread_sampler.map` 导出表。
- `SetThreadInfoCallback`、`DfxNotifyWatchdogThreadStart` 以 `__attribute__((weak))` 引用 faultloggerd 的弱符号，修改其签名需同步 faultloggerd 侧。

### 设备操作与进程退出约束

**涉及真实设备/进程退出时的注意事项：**
- `XCOLLIE_FLAG_RECOVERY` 路径会 `alarm(11)` + `LeftTimeExitProcess` → `sleep(10 s)`（等待 hiview 落盘）→ 写 `/proc/<pid>/unexpected_die_catch` → `_exit(0)`。修改该路径必须保留 hiview 落盘窗口，否则故障日志丢失。
- `KillPeerBinderProcess` 仅在 `FOUNDATION_UID` 时调用 `KillProcessByPid`；其它进程走 `LeftTimeExitProcess`。修改时勿混淆两类语义。
- `KillProcessByPid` 只对 `uid >= APP_MIN_UID (20000)` 的 peer binder 进程 `SIGKILL`，不得扩大至系统 UID。
- `AddKickWatchdog` 写 `/sys/kernel/hungtask/userlist` 与 `/proc/sys/hguard/user_list`，涉及内核 hung task 机制，修改前需经内核侧评审。
- 单元测试通过 `WatchdogInner::isTestExist_` 抑制 `LeftTimeExitProcess` 的实际 `_exit(0)`；新增退出路径测试必须同步设置该标志，否则测试进程会被 `_exit`。
- 栈采样依赖 `SYS_rt_tgsigqueueinfo` + `MUSL_SIGNAL_SAMPLE_STACK` 与 musl 约定，仅在 `__aarch64__`/`__loongarch_lp64` 生效；其它架构需保留降级路径。
- `AppMgrClient::NotifyAppFault` 上报 `APP_FREEZE` 会触发应用冻结日志收集，非应用主线程调用将返回 `HICOLLIE_WRONG_THREAD_CONTEXT`，修改 `IsAppMainThread`（`pid == tid && uid >= MIN_APP_UID`）需同步评估。
