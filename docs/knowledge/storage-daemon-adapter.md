# storage_daemon 适配与性能边界

本文记录 disk_manager 与 storage_daemon 的 IPC 契约、纯透传适配层、死亡监听、动态 so 扩展，以及框架层与工具层的性能边界。DFX 见 `service-and-dfx.md`，约束陷阱见 `constraints-and-traps.md`。

## IStorageDaemon IPC 契约

`IStorageDaemon`（`istorage_daemon.h`）定义 27 个方法，描述符 `u"OHOS.StorageDaemon.IStorageDaemon"`。disk_manager 通过 `StorageDaemonAdapter::GetInstance()` 获取 SA 5004（storage_daemon）的代理。

### 重操作（耗时由设备 I/O 决定）

| IPC code | 方法 | 内核工具 |
|----------|------|----------|
| 203 | ReadPartitionTable | sgdisk dump |
| 204 | Mount | mount syscall |
| 205 | Unmount | umount syscall |
| 206 | FormatVolume | mkfs |
| 207 | Check | fsck |
| 208 | Repair | fsck |
| 209 | SetLabel | tune2fs/e2label |
| 210 | ReadMetadata | blkid |
| 212 | Partition | parted/sgdisk |
| 215 | CreatePartition | sgdisk |
| 216 | DeletePartitionInfo | sgdisk |
| 217 | FormatPartition | mkfs |
| 254 | Erase | 擦除光盘 |
| 255 | Eject | eject |
| 256 | CreateIsoImage | mkisofs |
| 257 | Burn | growisofs |

### 轻操作（节点/查询类）

| IPC code | 方法 | 语义 |
|----------|------|------|
| 201 | CreateBlockDeviceNode | mknod |
| 202 | DestroyBlockDeviceNode | unlink |
| 211 | MountFuseDevice | FUSE 挂载 |
| 213 | GetBlockInfoByType | JSON 查询 |
| 251 | GetCapacity | statvfs |
| 253 | QueryCDStatus | ioctl CDROM_DRIVE_STATUS |
| 258 | GetVolumeOpProcess | 异步刻录进度 |
| 261 | GetDiskSize | ioctl |

## 框架层与工具层的时间边界

disk_manager 自身不含 `main()`、不 fork 工具进程，所有重操作经 `StorageDaemonAdapter` IPC 转发给 storage_daemon 执行内核工具。定位性能耗时时必须区分**框架层开销**与**工具执行耗时**。

`StorageDaemonAdapter` 每个方法有 `LOGI("Xxx enter")` / `LOGI("Xxx exit ret=...")` 成对日志：

```
IpcDfxScope START
  │  ← 框架层开销（锁竞争、状态查询、路径构造、voldata 映射、DFX 上报）
  │
adapter LOGI enter
  │  ← 工具执行耗时（IPC 往返 + storage_daemon fork 工具 + 内核 I/O）
  │
adapter LOGI exit
```

| 时间段 | 归属 | 定位方式 |
|--------|------|----------|
| IpcDfxScope START → adapter enter | 框架层 | 排查锁竞争、diskConfigList_/diskPartsSnapshot_ 规模 |
| adapter enter → adapter exit | 工具执行 | 排查设备 I/O、工具进程（storage_daemon 侧） |

**不得删除 adapter 的 enter/exit 日志**——这是当前唯一的耗时边界标记。

## StorageDaemonAdapter 纯透传层

`StorageDaemonAdapter`（`storage_daemon_adapter.h`）是单例 + `NoCopyable`，每个方法结构：`EnsureProxyReady()` → `storageDaemon_->Xxx(...)` → `LOGI enter/exit`。

| 特征 | 说明 |
|------|------|
| 无缓存 | 每次调用都走 IPC |
| 无重试 | 失败即返回 |
| 无超时 | 超时由业务层实现（如 `WaitForPartitionDone` 的 `partitionCv_.wait_until`，`disk_manager.cpp:1207`） |
| 无计时 | 仅 enter/exit LOGI 标记时间边界 |
| 错误映射 | `FinishDaemonOp`/`FinishMetadataOp` 在 `ret != E_OK` 时上报 fault |

## uevent 发现路径串行同步

`DiscoverPartitionsAndVolumes`（`uevent_bootstrap.cpp:763`）中 `ReadPartitionTable` + 每分区 `CreateBlockDeviceNode` + `ReadMetadata` + 可选 `Mount`/`Format` 是**串行同步**调用，无并发/批量化。多分区磁盘发现耗时 ≈ 分区数 × (blkid + mount) 单次耗时。禁止在此串行链路中插入额外的 storage_daemon IPC 或阻塞操作。

## 死亡监听

`SdDeathRecipient`（`storage_daemon_adapter.h`）实现 `IRemoteObject::DeathRecipient`，`OnRemoteDied` 重置 `storageDaemon_` proxy 为 nullptr。storage_daemon 崩溃后，下次调用 `EnsureProxyReady` 会重新 `GetSystemAbility(5004)`。

## 动态 so 扩展

| 适配器 | so 路径 | 接口 | 失败处理 |
|--------|---------|------|----------|
| `PcEncryptionAdapter` | `/system/lib64/libpc_encryption_ext_volume_user_api.z.so` | `QueryEncryptionStatus`、`NotifyVolumeMounted` | 返回 false（静默） |
| `UsbFuseAdapter` | `/system/lib64/libspace_ability_fuse_ext.z.so` | `NotifyUsbFuseMount`、`NotifyUsbFuseUmount`、`IsUsbFuseByType` | 视为不支持（静默） |

通过 `Init`/`UnInit` 管理 `void *handler_`，`dlopen` 失败静默处理。不得在 handler 未就绪时调用 dlsym 符号。

## DFX 上报不含耗时字段

IpcDfxScope/RadarParameter/AuditLog 均无 duration 字段（详见 `service-and-dfx.md`）。当前只能靠 adapter enter/exit LOGI 日志时间戳人工对齐推断耗时。如需新增耗时打点，须在 adapter 调用前后埋点，不可侵入 `IpcDfxScope` 析构路径。

## 框架层自身开销

以下为框架层开销，应保持轻量：

| 操作 | 复杂度 | 说明 |
|------|--------|------|
| uevent 解析（FNV-1a 哈希分发） | O(1) | `uevent_env_parser.cpp` |
| 配置匹配（diskConfigList_ 遍历子串匹配） | O(n) | `uevent_bootstrap.cpp:959-983` |
| 分区 diff（ComputePartitionDiff） | O(n×m) | `uevent_bootstrap.cpp:682-706` |
| 状态更新（std::map 插入持锁） | O(log n) | `disk_manager.cpp` |
| 公共事件发布 | — | `common_event_publisher.cpp` |

## 修改前检查

- 新增的操作是否需要经 storage_daemon 转发？是否属于重操作？
- adapter 方法是否保留了 enter/exit LOGI？
- 是否在串行 uevent 路径中插入了额外 IPC？
- 动态 so 加载失败是否静默处理？
- 死亡监听是否正确重置 proxy？

## 测试指引

- adapter：`storage_daemon_adapter_test`、`storage_daemon_proxy_test`
- PC 加密：`pc_encryption_adapter_test`
- USB FUSE：`usb_fuse_adapter_test`
- Mock：`mock_storage_daemon_adapter.h`、`storage_daemon_mock.h`
