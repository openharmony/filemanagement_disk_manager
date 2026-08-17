# 磁盘管理服务指引

## 项目定位

本仓库对应 OpenHarmony `foundation/filemanagement/disk_manager`。优先按这些目录定位问题：

- `services/disk_manager/disk/`：核心业务（DiskManager 单例）、uevent 解析、分区表解析（sgdisk dump）、disk_config 匹配。
- `services/disk_manager/ipc/`：SA 入口（DiskManagerProvider，SA ID 8640）、IPC stub、权限校验、空闲卸载（3 分钟）。
- `services/disk_manager/adapter/`：storage_daemon IPC 适配（IStorageDaemon、proxy、死亡监听）；dlopen 加载 USB FUSE 与 PC 加密扩展。
- `services/disk_manager/db/`：BlockInfoTable 内存缓存（不落盘）；VoldataUuidStore 持久化映射（原子写，最多 1000 条）。
- `services/disk_manager/dfx/`：IpcDfxScope 埋点、DiskManagerRadar HiSysEvent、HiAudit 审计日志。
- `services/disk_manager/notification/`：发布 `COMMON_EVENT_VOLUME_*` / `COMMON_EVENT_DISK_*` 公共事件。
- `interfaces/innerkits/`：内部 C++ API（IDiskManager.idl、DiskManagerClient、Disk、VolumeCore、VolumeExternal、PartitionInfo）。
- `interfaces/kits/js/`、`interfaces/kits/taihe/`：NAPI 与 Taihe/ANI 两套对外 JS API（`@ohos.file.volumeManager`）。
- `common/include/`：错误码（disk_manager_errno.h、disk_manager_napi_errno.h）。
- `utils/`：日志宏、IPC 鉴权（ipc_caller_auth）、字符串/UUID/路径校验。
- `etc/disk_config`、`sa_profile/8640.json`、`services/disk_manager/disk_manager.cfg`、`test/`：配置、SA profile、init 配置、测试。

注：进程无 `main()`，由 init 按 `disk_manager.cfg` 拉起 `sa_main` 加载 SA 库；OnStart 依次执行 `UeventBootstrap::Init` → `BlockInfoTable::ReloadFromDaemon` → `VoldataUuidStore::Init` → `SystemAbility::Publish` → `StartIdleMonitor`。

### 按任务类型定位代码

| 任务类型 | 首选目录 | 关键文件 |
|---------|---------|---------|
| 卷挂载/卸载/格式化/刻录、uevent 解析、分区表解析、disk_config | `services/disk_manager/disk/` | `disk_manager.h/cpp`、`uevent_bootstrap.h/cpp`、`uevent_env_parser.h/cpp`、`partition_table_parser.h/cpp`、`disk_config.h/cpp` |
| IPC 权限校验/SA 生命周期/空闲卸载 | `services/disk_manager/ipc/` | `disk_manager_provider.h/cpp` |
| storage_daemon 适配/USB FUSE/PC 加密 | `services/disk_manager/adapter/` | `storage_daemon_adapter.h/cpp`、`storage_daemon_proxy.h/cpp`、`istorage_daemon.h`、`usb_fuse_adapter.h/cpp`、`pc_encryption_adapter.h/cpp` |
| voldata UUID 持久化/BlockInfo 缓存 | `services/disk_manager/db/` | `voldata_uuid_store.h/cpp`、`block_info_table.h/cpp` |
| DFX 埋点/HiSysEvent/审计日志 | `services/disk_manager/dfx/` | `disk_manager_dfx.h`、`disk_manager_dfx_types.h/cpp`、`disk_manager_radar.h/cpp`、`hi_audit.h/cpp` |
| 公共事件发布 | `services/disk_manager/notification/` | `common_event_publisher.h/cpp` |
| Native API/IDL/客户端代理 | `interfaces/innerkits/` | `IDiskManager.idl`、`disk_manager_client.h/cpp`、`disk.h`、`volume_core.h`、`partition_types.h` |
| NAPI 绑定 | `interfaces/kits/js/` | `volumemanager_n_exporter.cpp`、`volumemanager_napi.cpp` |
| Taihe/ANI 接口 | `interfaces/kits/taihe/` | `ohos.file.volumeManager.taihe`、`ohos.file.volumeManager.impl.cpp` |
| IPC 鉴权/工具函数 | `utils/` | `ipc_caller_auth.h/cpp`、`disk_manager_utils.h/cpp` |

### 嵌套指引

本仓库无目录级别的嵌套指引。稳定背景知识放在 `docs/knowledge/`，按"知识索引"节的场景表读取对应文件。

## 构建和验证

构建命令从 OpenHarmony 源码根目录执行，不在本子目录执行。

```sh
./build.sh --product-name rk3568 --build-target disk_manager_server --ccache
./build.sh --product-name rk3568 --build-target disk_manager_innerkits --ccache
prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 disk_manager_server_test
```

单元测试 23 个目标、Fuzz 测试 9 个目标，详见 `docs/knowledge/testing-guide.md`。提交使用 `git commit -s`，保留 `Co-Authored-By: Agent`，变更需通过单元测试和 fuzz 测试。

### 完成标准

1. **代码改动已提交** - `git commit -s`，多代理协作添加 `Co-Authored-By: Agent`
2. **本地构建通过** - 执行上述构建命令
3. **相关测试通过** - 对应单元测试与 fuzz 测试
4. **板侧验证（如适用）** - 涉及真实磁盘/uevent/PC 加密/刻录/分区的改动需提供证据
5. **文档更新（如适用）** - 公共 API（NAPI/Taihe/IDL）修改需更新注释和文档

无法运行验证时，明确说明原因并列出推荐步骤供人工执行。完成报告含：改动摘要、验证结果、风险评估、未完成事项。

## 错误码与代码规范

### 命名规则

| 类型 | 规则 | 注意 |
|------|------|------|
| 类名/函数名 | 大驼峰 | `DiskManager`、`Mount`；`Locked`/`Unlocked` 后缀表示"调用方已持锁" |
| 变量名 | 小驼峰 | `volumeId`、`diskId`、`fsUuid` |
| 成员变量 | 下划线结尾 | `diskMap_`、`volumeMap_`、`partitionLock_` |
| 常量 | `constexpr` + UPPER_SNAKE | `DISK_MANAGER_SYS_CAP_TAG`、`UEVENT_RAW_MAX_LEN`、`IDLE_CHECK_INTERVAL_MS` |
| 枚举 | 遗留用 `enum : int32_t`，新增用 `enum class : int32_t` | `DiskManagerErrNo`（unscoped）；`VolumeOpType`、`DiskManagerNativeErr`（scoped） |
| 命名空间/文件名 | `OHOS::DiskManager` / 全小写下划线 | `napi_errno.h` 例外：在 `OHOS` 暴露裸名 |

### 错误码选用

禁止 `DISK_MGR_ERR`（= -1，仅用于 IpcDfxScope 析构兜底），必须用具体类别码：

| 场景 | 必须使用 |
|------|----------|
| volumeId/diskId 不存在 | `E_NON_EXIST` |
| volume 状态不允许 | `E_VOL_STATE` |
| 参数无效 | `E_PARAMS_INVALID` |
| 不支持操作 | `E_NOT_SUPPORT` |
| 卷挂载/卸载失败 | `E_VOL_MOUNT_ERR`、`E_VOL_UMOUNT_ERR`、`E_UMOUNT_BUSY` |
| 权限不足 | `E_PERMISSION_DENIED`、`E_SYS_APP_PERMISSION_DENIED` |
| SA 未就绪 | `E_SA_IS_NULLPTR`、`E_SERVICE_IS_NULLPTR`、`E_REMOTE_IS_NULLPTR` |
| 磁盘不存在/已存在 | `E_DISK_NOT_FOUND`、`E_DISK_HAS_EXIST` |
| uevent/分区/刻录失败 | `E_UEVENT_PARSE_FAILED`、`E_GET_PARTITION_ERROR`、`E_EMPTY_DISC`、`E_BURN_FAILED` 等 |

adapter 返回 `ERR_OK`/非零在业务层映射：`Mount`/`Unmount` 透传；`Erase`→`E_ERASE_FAILED`、`Eject`→`E_EJECT_FAILED`、`Burn`→`E_BURN_FAILED` 等。

### 错误码范围

`DISK_MANAGER_SYS_CAP_TAG = 13610000`（SA 能力标签，与 storage_service 的 13600000 错开）。实际枚举值仍以 `13600000` 为基址（与 storage_service 对齐），`13610000` 为预留标签待固化。

| 范围 | 类别 |
|------|------|
| 0 / -1 | `E_OK` / `DISK_MGR_ERR`（禁对外） |
| 10-13 | SA/IPC 软引用失败 |
| 20-26 | 磁盘/卷内部 |
| 13600001-13600025 | 通用错误（权限/参数/不存在，与 storage 对齐） |
| 13601204 | statvfs |
| 13601701-13601704 | 卷状态/挂载/卸载（`E_VOL_STATE`、`E_VOL_MOUNT_ERR` 等，后三者定义于 `napi_errno.h`） |
| 13601705 / 13601715 | 无子分区 / 其他挂载 |
| 13601739-13601751 | 分区操作（创建/删除/格式化/超时/查询） |
| 13601801-13601805 | 光盘/刻录 |

NAPI/JS 层错误码在 `common/include/disk_manager_napi_errno.h`（基址 `13600000`），含 `DiskManagerNativeErr`、`DiskManagerJsErr` 两个 enum class 及 `E_PERMISSION=201` 等 JS 错误码。

### 日志必打节点

日志宏 `LOGI/LOGW/LOGE/LOGD`（`utils/include/disk_manager_hilog.h`），domain `0xD00430F`、tag `"DiskManager"`。敏感参数（uuid/path）必须经 `GetAnonyString` 脱敏后用 `%{public}s` 输出。

| 流程 | 必打节点 | 级别 |
|------|----------|------|
| SA 生命周期 | begin、各阶段 ret、end | LOGI/LOGE |
| IPC 客户端/Provider | 入口 LOGI(参数) + 出口 LOGI(err) | LOGI/LOGE |
| 业务层入口 | RAII `IpcDfxScope` 构造上报 START；`dfx.Finish(具体 errno)` | LOGI/LOGE |
| 磁盘/卷事件 | 设备信息、正常/异常移除 | LOGI/LOGW |
| storage_daemon 死亡/uevent 失败 | OnRemoteDied 重置 proxy / `E_UEVENT_PARSE_FAILED` 上报 | LOGW/LOGE |

注意：业务层提前 return 必须用 `return dfx.Finish(<具体 errno>);`，析构兜底会上报 `DISK_MGR_ERR` 掩盖真实错误码。

### 编码约束

| 约束项 | 限制 |
|--------|------|
| 单个函数/文件 | ≤ 50 行 / ≤ 2000 行 |
| `.clang-format` | ColumnLimit=120、IndentWidth=4、UseTab=Never、PointerAlignment=Right、NamespaceIndentation=None |
| Unmarshalling | 字符串上限 `PARCEL_STRING_MAX_LEN = 4096`；分区数上限 `PARTITION_COUNT_MAX = 256` |

## 知识索引

稳定背景知识放在 `docs/knowledge/`。改动前按场景读取对应文件：

| 场景 | 先读文档 |
|------|---------|
| 卷状态转换、uevent 热插拔、公共事件时序、锁模型、分区表解析、分区操作、disk_config | `docs/knowledge/disk-volume-lifecycle.md` |
| IStorageDaemon IPC 契约、纯透传层、重操作清单、框架层/工具层性能边界、自证清白、死亡监听、dlopen 扩展 | `docs/knowledge/storage-daemon-adapter.md` |
| SA 生命周期、空闲卸载、IPC 鉴权、IpcDfxScope 埋点、Radar HiSysEvent、HiAudit 审计 | `docs/knowledge/service-and-dfx.md` |
| VoldataUuidStore 持久化映射、BlockInfoTable 内存缓存 | `docs/knowledge/data-persistence.md` |
| 架构约束汇总与陷阱（锁/状态机/事件时序/分区同步/dlopen/高危删除） | `docs/knowledge/constraints-and-traps.md` |
| 测试覆盖、Mock 策略、Fuzz 目标 | `docs/knowledge/testing-guide.md` |

### 开始编辑前

1. 确认任务类别（参考"按任务类型定位代码"表）
2. 根据上表确定需要阅读的文档
3. 根据"项目约束"确认不违反任何约束
4. 声明："我将修改 X，已阅读 Y 文档，遵循 Z 约束"

## 项目约束

### 性能约束

disk_manager 不 fork 工具进程，所有重操作经 `StorageDaemonAdapter` IPC 转发给 storage_daemon 执行内核工具。定位性能耗时须区分**框架层开销**与**工具执行耗时**，详见 `docs/knowledge/storage-daemon-adapter.md`。

- **不得删除 adapter 的 enter/exit 日志**——框架层与工具层的唯一时间边界。
- **uevent 发现路径串行同步**：`DiscoverPartitionsAndVolumes` 内 `ReadPartitionTable` + 每分区 `ReadMetadata` + `Mount` 串行调用，禁止插入额外 IPC。
- **BlockInfoTable 全量拉取限频**：`ReloadFromDaemon` 仅在 `OnStart` 调一次。
- **DFX 上报不含耗时**：如需耗时打点须在 adapter 前后埋点，不可侵入 `IpcDfxScope` 析构。

### 架构约束

- **锁顺序**：`diskMapMutex_` → `volumeMapMutex_`（先 disk 后 volume）。
- **卷状态**：`MOUNTED → EJECTING → REMOVED`，不可跳转。
- **公共事件时序**：详见 `docs/knowledge/disk-volume-lifecycle.md`。
- **分区同步**：`Partition` 后须 `partitionLock_` + `partitionCv_` 等待 `NotifyPartitionDone`。
- **空闲卸载**：`HasManagedResources()==false` 且 `pendingStorageDaemonCallbackCount_==0`。
- **VoldataUuidStore 原子写**：`.tmp` → `fsync` → `rename`，失败回滚内存 map。
- **dlopen 生命周期**：失败静默处理，不得在 handler 未就绪时调用 dlsym。

详细约束与陷阱见 `docs/knowledge/constraints-and-traps.md`。

### 编码约定

- C++ 改动优先复用 `LOGI/LOGE/LOGW/LOGD` 宏，不要直接调用 `HILOG_IMPL`。
- 业务层入口必须使用 RAII `IpcDfxScope` 埋点，必须 `return dfx.Finish(<具体 errno>)`。
- 错误码必须用具体类别码，禁止 `DISK_MGR_ERR` 对外返回。
- 敏感参数必须经 `GetAnonyString` 脱敏后用 `%{public}s` 输出。
- uevent 报文长度 `UEVENT_RAW_MAX_LEN = 4096`、`OP_DIAG_RAW_MAX_LEN = 8192`，超长拒绝。

### 公共 API 约束

**禁止**：修改已发布 NAPI/Taihe API 签名/错误码/行为语义；删除或重命名已有公共 API。

**修改前确认**：新增公共 API 是否需要 DFX/权限检查；修改 IDL 接口是否影响跨模块兼容性。

### 安全与权限边界

**禁止**：绕过 IPC 鉴权（storage_daemon 回调校验 `VerifyNativeCallerMatches("storage_daemon", uid=0)`；其他方法校验 `IsCallingSystemApp()` + 权限或 `IsStorageManagerCaller`(uid=1090)）；未验证使用跨进程 fd；敏感信息未脱敏入日志。

**修改前确认**：涉及 `AccessTokenKit` 改动；跨用户数据访问；卷加密状态（`PcEncryptionAdapter`）改动。

### 协议与数据格式兼容性

**禁止**：修改 IDL IPC 接口签名（`IDiskManager.idl`，27 个 IPC code）/ Parcel 序列化顺序 / Parcelable 布局（`Disk`/`VolumeCore`/`VolumeExternal`/`PartitionInfo` 等的 `Marshalling`/`Unmarshalling`）/ 已有 IPC code 语义。

**修改前确认**：新增 IPC 接口的跨版本兼容性；uevent 解析格式变更影响 storage_daemon 上报；`voldata_uuid_mapping.json` 格式变更影响存量映射。

### 生成代码边界

**禁止**：直接修改 IDL 生成的 stub/proxy（`idl_gen_interface("disk_manager_interface_native")`）；手动编辑 Taihe 生成代码（`taihe_shared_library` + `generate_static_abc`）。

**正确做法**：修改 `.idl` / `.taihe` 定义文件 → 重新运行编译器生成。

### 设备操作约束

- 不执行破坏性操作（对已挂载卷直接格式化、对在线磁盘强制分区）。
- 真实设备验证（磁盘热插拔/uevent/PC 加密/刻录）须提供板侧证据。
- **高危删除**：不可逆删除前必须确认目标真实后端——挂载类间接层使真实后端≠表面，残留挂载层递归删会穿透到源数据。`DestroyALLVolume` 须先 `ForceUnmount` 再 `DestroyBlockDeviceNode`。
