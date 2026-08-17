# SA 生命周期、IPC 鉴权与 DFX 可观测性

本文记录 DiskManagerProvider SA 生命周期、空闲卸载、IPC 鉴权、报文长度约束，以及 IpcDfxScope 行为埋点、DiskManagerRadar HiSysEvent 上报与 HiAudit 审计日志。错误码见 `AGENTS.md`，性能边界见 `storage-daemon-adapter.md`。

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

`DiskManagerProvider`（`disk_manager_provider.h`）继承 `SystemAbility` + `DiskManagerStub`，`REGISTER_SYSTEM_ABILITY_BY_ID(DiskManagerProvider, DISK_MANAGER_SA_ID, false)`。

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

## IPC 鉴权

| 方法类别 | 鉴权方式 | 权限 |
|----------|----------|------|
| storage_daemon 回调（OnBlockDiskUevent/NotifyMtpMounted/NotifyMtpUnmounted/ReportVolumeOpDiag） | `VerifyNativeCallerMatches("storage_daemon", STORAGEDAEMON_UID=0)` | — |
| 其他对外方法 | `IsCallingSystemApp()` + `VerifyCallerPermission` 或 `IsStorageManagerCaller` | `PERMISSION_MOUNT_MANAGER`、`PERMISSION_FORMAT_MANAGER`、`PERMISSION_STORAGE_MANAGER` |
| storage_manager 调用 | `IsStorageManagerCaller`（uid=1090） | — |

常量：`STORAGEDAEMON_UID = 0`、`STORAGE_MANAGER_UID = 1090`（`disk_manager_provider.cpp:44-45`）。

## 报文长度约束

| 常量 | 值 | 说明 |
|------|-----|------|
| `UEVENT_RAW_MAX_LEN` | 4096 | uevent 原始报文长度上限，超长拒绝 |
| `OP_DIAG_RAW_MAX_LEN` | 8192 | opDiag 报文长度上限，超长拒绝 |

校验在 provider 层（`disk_manager_provider.cpp:430`）。

## IpcDfxScope

`IpcDfxScope`（`disk_manager_dfx.h`）是 RAII 行为埋点类，用于 IPC 与业务层入口的 START/SUCCESS/FAIL 上报。

| 方法 | 行为 |
|------|------|
| 构造 | 立即 `ReportBehavior(funcName, stage, "START", info, E_OK)` |
| `Finish(ret)` | `ret==E_OK` 上报 SUCCESS，否则 FAIL；幂等（`finished_` 防重入） |
| 析构 | 若未 Finish 则 `Finish(DISK_MGR_ERR)` 兜底 |

**关键约束**：业务层所有提前 return 必须用 `return dfx.Finish(<具体 errno>);` 而非裸 `return errno;`——析构兜底会上报 `DISK_MGR_ERR`，掩盖真实错误码。

### DFX stage 常量

| 常量 | 值 | 业务入口 |
|------|-----|----------|
| `DFX_STAGE_MOUNT` | 41 | Mount |
| `DFX_STAGE_UNMOUNT` | 42 | Unmount/ForceUnmount |
| `DFX_STAGE_FORMAT` | 43 | Format |
| `DFX_STAGE_SET_VOLUME_DESCRIPTION` | 44 | SetVolumeDescription |
| `DFX_STAGE_UEVENT_PARSE` | 45 | OnBlockDiskUevent |
| `DFX_STAGE_GET_PARTITION_TABLE` | 46 | GetPartitionTable |
| `DFX_STAGE_CREATE_PARTITION` | 47 | CreatePartition |
| `DFX_STAGE_DELETE_PARTITION` | 48 | DeletePartition |
| `DFX_STAGE_FORMAT_PARTITION` | 49 | FormatPartition |

定义于 `disk_manager_dfx_types.h:25-33`。

## DiskManagerRadar

`DiskManagerRadar`（`disk_manager_radar.h`）是单例，负责 HiSysEvent 行为/故障事件上报 + 审计日志写入。

### 事件类型

| 事件名 | 参数 | 触发 |
|--------|------|------|
| `FILE_BACKUP_EVENTS` | PROC_NAME/BUNDLENAME/PID/TIME | `ReportBehavior`（IpcDfxScope 构造/Finish） |
| `FILE_STORAGE_FAULT` | ORG_PKG/USER_ID/FUNC/BIZ_SCENE/BIZ_STAGE/KEY_ELX_LEVEL/TO_CALL_PKG/FILE_STATUS/ERROR_CODE | `RecordFault`（adapter 失败路径） |

`RadarParameter` 字段：`orgPkg`、`userId`、`funcName`、`bizScene`、`bizStage`、`keyElxLevel`、`toCallPkg`、`fileStatus`、`errorCode`。**无耗时字段**。

## HiAudit

`HiAudit`（`hi_audit.h`）是单例 + `NoCopyable`，本地审计日志文件写入（CSV 格式 + 滚动）。

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

- 新增 IPC 方法是否正确鉴权？是 storage_daemon 回调还是对外方法？
- 空闲卸载条件是否满足？storage_daemon 回调计数是否正确维护？
- 报文长度是否超限？
- 业务层入口是否用了 IpcDfxScope？提前 return 是否用了 `dfx.Finish(errno)`？
- 新增 DFX stage 是否在 `disk_manager_dfx_types.h` 定义？
- 审计日志字段是否包含敏感信息（uuid/path）？需脱敏。

## 测试指引

- Provider：`disk_manager_provider_test`
- 鉴权：`disk_manager_provider_test`（mock `AccessTokenKit`、`IPCSkeleton`）
- DFX：`disk_manager_dfx_test`、`disk_manager_dfx_types_test`
- Radar：`disk_manager_radar_test`
- HiAudit：`hi_audit_test`
- Fuzz：`diskmanagerprovider_fuzzer`、`diskmanagerstub_fuzzer`
