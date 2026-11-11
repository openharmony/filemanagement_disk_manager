# 磁盘管理服务指引

## 业务定位

OpenHarmony **外置存储磁盘管理服务**（SA 8640，`libdisk_manager_server.z.so`，无 `main()`，由 init 按 `disk_manager.cfg` 按需拉起）：管理可插拔存储设备从"插入"到"可用"的全生命周期——热插拔识别、卷状态机与系统广播、挂载/卸载/格式化/分区/刻录、文件系统检查修复、空闲自动卸载、EDM 企业管控。识别、状态与策略在本仓；挂载/格式化等重活经 IPC 转发 storage_daemon（storage_service 仓）执行，本仓不直接操作块设备。

**支持的硬件**（`disk.h` DiskType 枚举，按数值 1~6 顺序；uevent 经 `disk_config` 白名单匹配）：

| # | 硬件 | 说明 |
|---|------|------|
| 1 | SD 卡（`SD_FLAG`） | 机器自带 SD 卡槽直连（裸卡槽）场景；经读卡器从 USB/Type-C 等外置接口接入的 SD 卡属 `USB_FLAG`，不属本类型 |
| 2 | U 盘（`USB_FLAG`） | 主流场景；PC 形态含加密 U 盘（dm-crypt + loop 设备） |
| 3 | 光驱（`CD_FLAG`） | CD/DVD/BD：挂载、刻录、擦除、弹出 |
| 4 | 内置数据盘 SSD（`DATA_DISK_SSD`） | 主板槽位直连的固态硬盘；USB/Type-C 外接同介质盘属 `USB_FLAG`。仅 PC/2in1/平板（`PC_MANAGER` 宏） |
| 5 | 内置数据盘 HDD（`DATA_DISK_HDD`） | 主板槽位直连的机械硬盘；USB/Type-C 外接同介质盘属 `USB_FLAG`。仅 PC/2in1/平板（`PC_MANAGER` 宏） |
| 6 | 行车记录仪盘（`DVR_USB`） | 仅车机（`CDC_STORAGE` 宏） |

**类型判定看接入总线，不看物理介质**：凡经 USB/Type-C 等外置接口桥接接入的设备（读卡器中的 SD 卡、外接 SSD/HDD 移动硬盘）均呈现为 `USB_FLAG`。

MTP/PTP 设备不在 DiskType 枚举内，不走 uevent：存储管理部件执行挂载/卸载逻辑（mtpfs/gphotofs）后经 innerkits `NotifyMtpMounted`/`NotifyMtpUnmounted` 通知本仓管理状态；应用卸载复用 volumeManager 的 unmount 接口能力。

**对外提供三层出口**：

- **JS API** `@ohos.file.volumeManager`（NAPI + Taihe）：应用查询磁盘/卷、主动挂载/卸载/格式化
- **C++ innerkits** `DiskManagerClient`：storage_daemon 转发 uevent、EDM 管控应用重挂载、系统应用
- **公共事件** `VOLUME_*`/`DISK_*`/`DISK_UNMOUNTABLE` 广播：消费方为文件管理器、系统设置、大桌面等

**对外兼容性（必须遵守）**：凡对外有影响的代码修改，必须先与用户评估确认后再动手，不得自行实施：

- **JS API 对外错误码的变化属非兼容变更，严格禁止**（三方应用按错误码写死分支）。
- C++ innerkits 错误码：确认不影响 JS API 映射时可以修改。
- 公共事件的参数（字段、action、reasonCode 等）修改须与用户确认，避免非兼容变更。

**动手前**：写代码先过 Clean Code 硬规则（门禁 codecheck 必查，违反即返工）；改模块按文末知识索引先读对应文档。

## 目录地图

| 目录 | 职责 |
|------|------|
| `services/disk_manager/disk/` | 核心业务：DiskManager 单例、uevent 解析、分区表解析、挂载策略、异常卷标记、sysfs 直读 |
| `services/disk_manager/ipc/` | SA 入口 DiskManagerProvider、IPC 鉴权、空闲卸载（3 分钟） |
| `services/disk_manager/adapter/` | storage_daemon IPC 适配、加密 U 盘、dlopen 扩展（USB FUSE/PC 加密）、EDM 管控 |
| `services/disk_manager/db/` | BlockInfoTable 内存缓存、VoldataUuidStore 持久化映射 |
| `services/disk_manager/dfx/` | IpcDfxScope 埋点、HiSysEvent、HiAudit |
| `services/disk_manager/notification/` | 公共事件发布（VOLUME_*/DISK_*/DISK_UNMOUNTABLE） |
| `interfaces/innerkits/` | C++ 内部 API：IDiskManager.idl、DiskManagerClient、各 Parcelable |
| `interfaces/kits/js/`、`interfaces/kits/taihe/` | NAPI 与 Taihe 对外 API（`@ohos.file.volumeManager`） |
| `common/include/`、`utils/` | 错误码、日志宏、IPC 鉴权、校验工具 |

OnStart 序列：`UeventBootstrap::Init` → `BlockInfoTable::ReloadFromDaemon` → `VoldataUuidStore::Init` → `Publish` → `StartIdleMonitor`。

## Clean Code 硬规则（门禁 codecheck 必查）

### 行数与格式

| 项 | 限制 |
|----|------|
| **函数** | **≤ 50 有效行**（不含空行和注释） |
| 文件 | ≤ 2000 行 |
| 单行 | ≤ 120 字符，按 `.clang-format` 断行 |

**行数口径**：新写/改动的代码遵守；**存量超限的函数与文件不要求单独修改**；评估后认为拆分会影响可读性的，可以不拆，走报备处理——禁止为过检而对存量代码大规模重构。

需要拆分时的本仓惯用法：抽小辅助函数、持锁变体加 `Locked`/`Unlocked` 后缀、文件内辅助函数放匿名 namespace。

`.clang-format` 要点（提交前格式化）：4 空格缩进禁 Tab、K&R 大括号、`PointerAlignment: Right`、`NamespaceIndentation: None`、`BinPackParameters: false`（形参要么同行要么每行一个）。

### 命名

| 类型 | 规则 | 示例 |
|------|------|------|
| 类/函数 | 大驼峰 | `DiskManager`、`Mount` |
| 变量/参数 | 小驼峰 | `volumeId`、`fsUuid` |
| 成员变量 | 下划线结尾 | `diskMap_`、`partitionLock_` |
| 常量/宏 | UPPER_SNAKE，`constexpr` 优先 | `IDLE_CHECK_INTERVAL_MS` |
| 新枚举 | `enum class`（遗留 `DiskManagerErrNo` 是 unscoped，勿模仿） | `VolumeOpType` |
| 文件名 | 全小写下划线 | `disk_manager.cpp` |

**禁止魔法数字/魔法字符串**：数字、超时、路径、长度上限一律 `constexpr` 具名常量（本仓惯例见 `disk_manager_provider.cpp` 顶部常量区）。

### 写新代码必套的模式

1. **版权头**：新文件必须带 Apache License 头（拷贝任意现有文件头）。
2. **业务入口埋点**：RAII `IpcDfxScope` 构造上报 START，**所有出口必须 `return dfx.Finish(<具体errno>);`**——裸 return 会被析构兜底成 `DISK_MGR_ERR`，掩盖真实错误码。
3. **错误码**：禁止 `DISK_MGR_ERR`/裸 `-1` 对外，按下表选具体码：

| 场景 | 用 |
|------|-----|
| id 不存在 | `E_NON_EXIST` |
| 状态不允许 | `E_VOL_STATE`（`E_BUSY` 用于格式化重入） |
| 参数无效 | `E_PARAMS_INVALID` |
| 不支持 | `E_NOT_SUPPORT` |
| 权限不足 | `E_PERMISSION_DENIED` / `E_SYS_APP_PERMISSION_DENIED` |
| 挂/卸载失败 | `E_VOL_MOUNT_ERR` / `E_VOL_UMOUNT_ERR` / `E_UMOUNT_BUSY`（数据盘被占用） |
| 分区/刻录/加密U盘 | 1739-1751 / 1801-1806 / 1901-1906 段对应码，全表见 `service-and-dfx.md` |

4. **日志**：只用 `LOGI/LOGW/LOGE/LOGD` 宏（勿直接 `HILOG_IMPL`）；uuid/path/sn 等敏感参数必须 `GetAnonyString(...)` 脱敏后以 `%{public}s` 输出（隐私门禁会扫）。**边界日志规则**（外部调用进来的接口、调用外部的接口均适用，格式参考 `storage_daemon_adapter.cpp`、`uevent_bootstrap.cpp`）：
   - 入口日志：`<接口名> enter`，外部调用进来的加 `external=<来源接口>`，后接全部入参；
   - 出口日志：`<接口名> exit ret=<结果>`，失败路径用 LOGE 带错误码。
5. **单例**：`GetInstance()` 静态局部变量模式 + `NoCopyable`，参考 `StorageDaemonAdapter`。
6. **报文长度**：uevent 4096 / opDiag 8192 / Parcel 字符串 4096 / 分区数 256，超限在入口拒绝。

### 平台特性宏（disk_manager.gni，改代码先看编译条件）

- `PC_MANAGER`（pc/2in1/tablet）：加密 U 盘 IPC 15-20、内置盘非 f2fs 直挂
- `CDC_STORAGE`（car）：DVR_USB 事件仅发 user 0
- `HMDFS_FILE_MANAGER`：DFS API（方法数以 `.idl` 为准），宿主在本仓、逻辑归 dfs_service，**勿大改**
- `EDM_ADAPTER_ENABLE`：edm_adapter.cpp 编入

## 构建与验证

在源码根目录（`code/`）执行，编译组件按场景调整：

```sh
./build.sh --product-name rk3568 --ccache --build-target disk_manager disk_manager_test --target-cpu arm --gn-args allow_sanitize_debug=true
```

UT 与 Fuzz 目标清单见 `testing-guide.md`（以 `test/unittest/BUILD.gn`、`test/fuzztest/BUILD.gn` 聚合 group 为准）。提交 `git commit -s`，附 `Co-Authored-By: Agent`。板侧场景（真实磁盘/uevent/刻录/EDM）需证据；无法验证时说明原因并给人工步骤。

## 红线约束（详见 constraints-and-traps.md）

- 锁顺序 `diskMapMutex_` → `volumeMapMutex_`，反序必死锁；长等待（授权弹窗）不得持锁。
- 卷状态机不可跳步；`COMMON_EVENT_DISK_UNMOUNTABLE` 的 reasonCode（1-4）跨版本稳定勿改。
- IDL/Parcel/IPC code/`IStorageDaemonIpcCode` 数值是跨仓兼容契约，禁止擅改；生成代码只改 `.idl`/`.taihe` 源。
- 不得删除 adapter enter/exit 日志（唯一耗时边界）；uevent 发现串行链路禁插额外 IPC。
- `DestroyALLVolume` 必须先 `ForceUnmount` 再 `DestroyBlockDeviceNode`（残留挂载层递归删会穿透源数据）。
- IPC 鉴权五类写死：daemon 回调 uid=0、系统应用+权限、`GetExternal*` 仅权限、加密 U 盘 uid 7060、storage_manager uid 1090。
- 对外兼容：JS API 错误码变化（非兼容变更）严格禁止；innerkits 错误码、公共事件参数修改须先与用户评估确认（详见对外兼容性节与 constraints-and-traps.md）。
- 改业务代码必须评估知识库是否需同步更新；需要则与业务代码同一 commit 上库（见知识索引节）。

## 知识索引（按场景先读再改，改完必须回写）

**知识库同步（必须遵守）**：修改业务代码时，必须评估本次改动是否使知识库文档过时（流程、契约、错误码、宏、测试、陷阱等记录与代码不一致）；需要更新的，知识库修改与业务代码**同一个 commit 一并上库**，禁止"文档以后补"。

| 改什么 | 读什么 |
|--------|--------|
| 挂载/卸载/挂载策略/uevent/异常卷/分区 | `docs/knowledge/disk-volume-lifecycle.md` |
| storage_daemon 对接/加密 U 盘/性能边界 | `docs/knowledge/storage-daemon-adapter.md` |
| SA 生命周期/鉴权/EDM/格式化授权/DFX/错误码全表 | `docs/knowledge/service-and-dfx.md` |
| voldata 持久化/BlockInfo 缓存 | `docs/knowledge/data-persistence.md` |
| 全部陷阱与跨仓契约（EDM 命名勿"修复"等） | `docs/knowledge/constraints-and-traps.md` |
| 测试与 Mock 注入 | `docs/knowledge/testing-guide.md` |

动手前声明："我将修改 X，已读 Y 文档，遵循 Z 约束。"
动手后声明："本次改动需要/不需要同步知识库文档 W"——需要时，文档更新与代码同 commit 上库。
