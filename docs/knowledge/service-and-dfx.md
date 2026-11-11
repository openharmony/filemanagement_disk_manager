# SA 生命周期、IPC 鉴权与 DFX 可观测性

本文记录 DiskManagerProvider SA 生命周期、空闲卸载、IPC 鉴权（含 EDM 管控与数据盘格式化授权）、报文长度约束、错误码范围全表，以及 IpcDfxScope 行为埋点、DiskManagerRadar HiSysEvent 上报与 HiAudit 审计日志。选用规则见 `AGENTS.md`，性能边界见 `storage-daemon-adapter.md`。

## 错误码范围

`DISK_MANAGER_SYS_CAP_TAG = 13610000`（SA 能力标签预留，尚未在平台错误码分配表固化）。实际枚举值以 `13600000` 为基址（与 storage_service 对齐）。

| 范围 | 类别 |
|------|------|
| 0 / -1 | `E_OK` / `DISK_MGR_ERR`（禁对外） |
| 10-13 | SA/IPC 软引用失败 |
| 20-27 | 磁盘/卷内部 |
| 13600001-13600025 | 通用错误（权限/参数/不存在，与 storage 对齐） |
| 13601204 | statvfs |
| 13601701-13601715 | 卷状态/挂载/卸载/其他挂载 |
| 13601730-13601731 | 卷修复 |
| 13601739-13601751 | 分区操作（含 1751 `E_QUERY_VOLUME_IN_USE_ERROR`） |
| 13601801-13601806 | 光盘/刻录 |
| 13601901-13601907 | 加密 U 盘（bind loop / dm-crypt / by-path 挂卸载）+ 数据盘格式化重入（1907） |

NAPI/JS 层错误码在 `common/include/disk_manager_napi_errno.h`（基址 `13600000`），含 `DiskManagerNativeErr`、`DiskManagerJsErr` 两个 enum class 及 `E_PERMISSION=201` 等 JS 错误码。

## SA 配置

| 项 | 值 |
|----|-----|
| SA ID | 8640（`sa_profile/8640.json`） |
| 进程名 | `disk_manager` |
| SA 库 | `libdisk_manager_server.z.so` |
| run-on-create | false（按需加载） |
| 进程入口 | `/system/bin/sa_main /system/profile/disk_manager.json` |
| uid/gid | disk_manager（1091） |
| ondemand | true |
| apl | system_basic |

`DiskManagerProvider` 继承 `SystemAbility` + `DiskManagerStub`，`REGISTER_SYSTEM_ABILITY_BY_ID(DiskManagerProvider, DISK_MANAGER_SA_ID, false)`。

## OnStart 序列

```
UeventBootstrap::Init → BlockInfoTable::ReloadFromDaemon → VoldataUuidStore::Init → SystemAbility::Publish → StartIdleMonitor
```

每步均有 LOGI 记录 ret。OnStop 仅 `StopIdleMonitor`。

## 空闲卸载

`CheckAndUnloadIfIdle` 每 `IDLE_CHECK_INTERVAL_MS = 3*60*1000`（3 分钟）检查一次。

| 卸载条件 | 说明 |
|----------|------|
| `HasManagedResources() == false` | 无磁盘/卷在管理 |
| `pendingStorageDaemonCallbackCount_ == 0` | 无 storage_daemon 回调进行中 |

storage_daemon 回调进行中时通过 `BeginPendingStorageDaemonCallback`/`EndPendingStorageDaemonCallback` 维护原子计数，避免卸载时机错误。

注意：diskMap_/volumeMap_ 全内存不落盘，SA 卸载即清空，再插盘靠 storage_daemon 重放 uevent 重建。

## IPC 鉴权

`IDiskManager.idl` 的 IPC 方法（方法数以 `.idl` 源为准，动态增长）按调用方分五类：

| 方法类别 | 鉴权方式 | 常量/权限 |
|----------|----------|-----------|
| storage_daemon 回调（OnBlockDiskUevent/NotifyMtpMounted/NotifyMtpUnmounted/ReportVolumeOpDiag） | `VerifyNativeCallerMatches("storage_daemon", uid=0)` | `STORAGEDAEMON_UID = 0` |
| 普通系统方法（Mount/Unmount/Format/GetAllDisks 等） | `IsCallingSystemApp()` + 权限，或 `IsStorageManagerCaller` | `PERMISSION_MOUNT_MANAGER`/`PERMISSION_FORMAT_MANAGER`/`PERMISSION_STORAGE_MANAGER` |
| 外部存储查询（GetExternalDiskInfos/GetExternalVolumeInfos，code 51/52） | **仅** `VerifyCallerPermission(PERMISSION_GET_STORAGE_VOLUME_INFO)`，不查系统应用 | `ohos.permission.GET_STORAGE_VOLUME_INFO`（normal，三方可用） |
| 加密 U 盘（code 15-20，PC_MANAGER） | uid == 7060（外部 crypto_usb_mgr 服务）+ `PERMISSION_MOUNT_MANAGER` | `CRYPTO_USB_MGR_SERVICE_UID = 7060` |
| QueryUsbIsInUse（code 14） | `IsCallingSystemApp()` + `PERMISSION_MOUNT_MANAGER` | — |

storage_manager 调用放行：`IsStorageManagerCaller`（`VerifyNativeCallerMatches("storage_manager", STORAGE_MANAGER_UID=1090)`）。

## EDM 企业管控（EdmAdapter，EDM_ADAPTER_ENABLE）

三个系统参数（均要求企业设备 `const.edm.is_enterprise_device == "true"`）：

| 参数 | 作用 |
|------|------|
| `persist.edm.enable_external_storage_mount_intercept` | 开启外置存储挂载拦截 |
| `persist.edm.disable_sata_odd_burn` | 禁止内置光驱（非 USB 总线）刻录 |
| （EDM 白名单） | 外置 USB 光驱刻录经 `UsbManagerProxy::IsAllowedOddBurn` 查询 |

- **挂载拦截**（`IsEdmControlMountEnabled`，返回 true=拦截跳过挂载）：企业设备 + 拦截开启 + 非 EDM 自身发起（`MountParam.fromEdmMount`） + 受管盘型（SD/USB/CD）→ 先 `NotifyExternalStorageDeviceAdd` 通知 EDM，通知成功才拦截；通知失败放行挂载。
- **重挂载闭环在管控应用**：收到通知的企业管控应用通过 innerkits `DiskManagerClient` 发起 `Mount(fromEdmMount=true)` 绕过拦截。
- **刻录管控**（`IsEdmEnableOddBurn`）：内置光驱看参数；外置光驱查 EDM 白名单（vid/pid/sn 来自 sysfs/BlockInfo）。
- **命名陷阱**：`NotifyExternalStorageDeviceAdd` 调用的是 EDM 侧 `EnterpriseDeviceMgrProxy::NotifyUnmountExternalStorageDeviceInfo` —— EDM 侧接口就这样命名，语义是"上报外置存储设备信息"，**不是 bug，不要"修复"**。
- 判内置光驱用 `!disk.IsUsbBus()`（不能用 vendorId==0，外置光驱未上报 vid 时也为 0）。
- proxy 为 null 时放行（fail-open），不阻断正常流程。

## 数据盘格式化授权（CheckAdminAuthorization）

Format/FormatPartition 目标为内置数据盘（`IsInternalDataDisk`）时，需交互式授权：

- 走 `AccountSA::AuthorizationClient::AcquireAuthorization`，权限名 `ohos.privilege.format_data_disk`。
- 等待用户在弹窗确认，`AUTH_WAIT_TIMEOUT_SEC = 3600`；超时/拒绝 → `E_PERMISSION_DENIED`。
- 等待期间**不阻塞其他 IPC**（SA 有线程池），但调用方请求会挂起；勿在持锁状态调用（授权回调 `FormatAuthCallback` 用 cv+mutex 同步）。
- 并发重入防护：格式化进行中再次发起返回 `E_BUSY`（1907）。

## 报文长度约束

| 常量 | 值 | 说明 |
|------|-----|------|
| `UEVENT_RAW_MAX_LEN` | 4096 | uevent 原始报文长度上限，超长拒绝 |
| `OP_DIAG_RAW_MAX_LEN` | 8192 | opDiag 报文长度上限，超长拒绝 |

校验在 provider 层。`ReportVolumeOpDiag`（oneway，code 50）解析为 `OpDiagReport`（`ParseOpDiagReport`），失败项经 `ApplyVolumeOpDiagReport` 落 DFX/HiAudit；文本截断上限 `DFX_TRUNCATE_MAX_LEN = 2048`。

## IpcDfxScope

RAII 行为埋点类，用于 IPC 与业务层入口的 START/SUCCESS/FAIL 上报。

| 方法 | 行为 |
|------|------|
| 构造 | 立即 `ReportBehavior(funcName, stage, "START", info, E_OK)` |
| `Finish(ret)` | `ret==E_OK` 上报 SUCCESS，否则 FAIL；幂等（`finished_` 防重入） |
| 析构 | 若未 Finish 则 `Finish(DISK_MGR_ERR)` 兜底 |

**关键约束**：业务层所有提前 return 必须用 `return dfx.Finish(<具体 errno>);` 而非裸 `return errno;`——析构兜底会上报 `DISK_MGR_ERR`，掩盖真实错误码。

### DFX stage 常量（disk_manager_dfx_types.h）

| 常量 | 值 | 常量 | 值 |
|------|-----|------|-----|
| `DFX_STAGE_MOUNT` | 41 | `DFX_STAGE_UNMOUNT` | 42 |
| `DFX_STAGE_FORMAT` | 43 | `DFX_STAGE_SET_VOLUME_DESCRIPTION` | 44 |
| `DFX_STAGE_UEVENT_PARSE` | 45 | `DFX_STAGE_GET_PARTITION_TABLE` | 46 |
| `DFX_STAGE_CREATE_PARTITION` | 47 | `DFX_STAGE_DELETE_PARTITION` | 48 |
| `DFX_STAGE_FORMAT_PARTITION` | 49 | `DFX_STAGE_BIND_BLOCK_LOOP_DEV` | 50 |
| `DFX_STAGE_CREATE_DM_CRYPT_VOLUME` | 51 | `DFX_STAGE_DESTROY_DM_CRYPT_VOLUME` | 52 |
| `DFX_STAGE_UNBIND_BLOCK_LOOP_DEV` | 53 | `DFX_STAGE_MOUNT_VOL_BY_PATH` | 54 |
| `DFX_STAGE_UMOUNT_VOL_BY_PATH` | 55 | `DFX_STAGE_NOTIFY_MTP_MOUNTED` | 56 |
| `DFX_STAGE_NOTIFY_MTP_UNMOUNTED` | 57 | `DFX_STAGE_ERASE` | 60 |
| `DFX_STAGE_EJECT` | 61 | `DFX_STAGE_BURN` | 62 |
| `DFX_STAGE_CREATE_ISO_IMAGE` | 63 | `DFX_STAGE_CDROM_DISCOVER` | 64 |

同文件还定义 `VolumeOpType`（操作类型枚举）、`AutoMountSkipReason`、`VolumeReportInfo`（埋点附加信息）。

## DiskManagerRadar

单例，负责 HiSysEvent 行为/故障事件上报 + 审计日志写入。

| 事件名 | 参数 | 触发 |
|--------|------|------|
| `FILE_BACKUP_EVENTS` | PROC_NAME/BUNDLENAME/PID/TIME | `ReportBehavior`（IpcDfxScope 构造/Finish） |
| `FILE_STORAGE_FAULT` | ORG_PKG/USER_ID/FUNC/BIZ_SCENE/BIZ_STAGE/KEY_ELX_LEVEL/TO_CALL_PKG/FILE_STATUS/ERROR_CODE | `RecordFault`（adapter 失败路径） |

`RadarParameter` 字段：`orgPkg`、`userId`、`funcName`、`bizScene`、`bizStage`、`keyElxLevel`、`toCallPkg`、`fileStatus`、`errorCode`。**无耗时字段**。

## HiAudit

单例 + `NoCopyable`，本地审计日志文件写入（CSV 格式 + 滚动）。

| 项 | 值 |
|----|-----|
| 目录 | `/data/log/hiaudit/disk_manager/` |
| 文件名 | `disk_manager_audit.csv` |
| 单文件上限 | 3MB |
| 保留文件数 | 10 |
| 写入缓冲 | 2KB |
| 表头 | `happenTime, packageName, isForeground, cause, isUserBehavior, operationType, operationScenario, operationStatus, operationCount, extend` |

`AuditLog` 字段：`isUserBehavior`、`operationCount`、`cause`、`operationType`、`operationScenario`、`operationStatus`、`extend`。**无耗时字段**。

## 修改前检查

- 新增 IPC 方法鉴权归入哪一类（上表五类）？三方可用接口必须仅权限校验。
- 空闲卸载条件是否满足？storage_daemon 回调计数是否维护？
- 报文长度是否超限？
- 业务层入口是否用了 IpcDfxScope？提前 return 是否用了 `dfx.Finish(errno)`？
- 新增 DFX stage/VolumeOpType 是否在 `disk_manager_dfx_types.h` 定义？
- 审计日志字段是否包含敏感信息（uuid/path/sn）？需脱敏。

## 测试指引

- Provider/鉴权：`disk_manager_provider_test`（mock `AccessTokenKit`、`IPCSkeleton`）
- EDM：`edm_adapter_test`（mock `mock_parameters.cpp` 替换 system::GetParameter）
- DFX：`disk_manager_dfx_test`、`disk_manager_dfx_types_test`
- Radar：`disk_manager_radar_test`；HiAudit：`hi_audit_test`
- Fuzz：`diskmanagerprovider_fuzzer`、`diskmanagerstub_fuzzer`、`diskmanagerutils_fuzzer`
