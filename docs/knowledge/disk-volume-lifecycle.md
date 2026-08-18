# 磁盘与卷生命周期

本文记录卷状态转换、uevent 热插拔流程、公共事件时序、锁模型与分区管理约束。错误码见 `AGENTS.md`，IPC 契约见 `storage-daemon-adapter.md`，性能边界见 `storage-daemon-adapter.md`，约束陷阱汇总见 `constraints-and-traps.md`。

## 状态转换约束

卷状态转换必须经过合法路径，不可跳步（`interfaces/innerkits/include/volume_core.h`，16 态）：

| 操作 | 必经路径 | 禁止跳转 |
|------|----------|----------|
| 挂载 | UNMOUNTED → CHECKING → MOUNTED | 禁止 UNMOUNTED → MOUNTED |
| 卸载 | MOUNTED → EJECTING → REMOVED | 禁止 MOUNTED → REMOVED |
| 异常拔出 | MOUNTED → BAD_REMOVAL | 无需经过 EJECTING |

状态切换由 `SetVolumeStateLocked`（调用方持 `volumeMapMutex_` 锁）统一执行。操作失败时回退到操作前状态，不要停留在中间状态。

## 锁模型

| 锁 | 保护对象 | 类型 | 注意 |
|----|----------|------|------|
| `diskMapMutex_` | `diskMap_` | `std::shared_mutex` | 读写分离 |
| `volumeMapMutex_` | `volumeMap_` | `std::shared_mutex` | 读写分离 |
| `partitionLock_` | `partitionDoneMap_`、`partitionTableMap_`、`partitioningDiskIds_` | `std::mutex` | 配合 `partitionCv_` |
| `oddMutex_` | odd 状态 | `std::shared_mutex` | 光驱特有 |

**锁顺序**：同一流程需 `diskMapMutex_` 和 `volumeMapMutex_` 两把锁时，必须按"先 disk 后 volume"顺序加锁（`disk_manager.h:211-212`），否则死锁。`partitionLock_` 独立于上述两把锁。

## uevent 热插拔流程

入口 `UeventBootstrap::OnBlockDiskUevent`（`uevent_bootstrap.cpp:714`），用 FNV-1a 哈希分发 action：

| action | 处理函数 | 关键流程 |
|--------|----------|----------|
| add | `HandleDiskAdd` | `DiscoverPartitionsAndVolumes`（建节点→sgdisk→blkid→mount） |
| remove | `HandleDiskRemove` | `DestroyALLVolume`（ForceUnmount→DestroyBlockDeviceNode）→ `DestroyALLDisk` |
| change | `HandleDiskChange` | 分区进行中且内置数据盘则跳过（`uevent_bootstrap.cpp:845`）；否则 `DiscoverPartitionsAndVolumes` + `NotifyPartitionDone` |

**diskId 生成**：`DiskIdFrom(maj, min)` → `"disk-{maj}-{min}"`；**volId 生成**：`VolIdFromDev(dev_t)` → `"vol-{maj}-{min}"`。

## 公共事件时序

拔插/安全弹出的公共事件时序不可错（`common_event_publisher.cpp:46-50` 注释）：

| 场景 | 事件序列 |
|------|----------|
| 接入 | `DISK_MOUNTED` → 各卷 `VOLUME_MOUNTED` |
| 安全弹出（Unmount） | `VOLUME_EJECT` → `VOLUME_UNMOUNTED`（设备仍在） |
| 安全弹出后拔出 | 仅 `VOLUME_REMOVED`（随后 `DISK_REMOVED`），不再发 EJECT/UNMOUNTED |
| 未安全弹出直接拔出 | `VOLUME_EJECT` → `VOLUME_UNMOUNTED` → `VOLUME_BAD_REMOVAL`（随后 `DISK_REMOVED`） |

## 分区表解析

`PartitionTableParser::ParseSgdiskDump`（`partition_table_parser.cpp`）解析 storage_daemon 返回的 sgdisk dump 文本，输出 `PartitionRecord` 列表与 tableType（gpt/mbr）。纯字符串解析，无 IPC、无内核调用。

| 项 | 说明 |
|----|------|
| 输入 | `StorageDaemonAdapter::ReadPartitionTable`（IPC code 203）或 `GetPartitionTableInfo`（214）返回的原始文本 |
| 输出 | `std::vector<PartitionRecord>`（partitionNumber、partitionType、fsTypeRaw） |
| MBR 支持 | `IsMbrTypeSupportedForVolume` 白名单判断，与 storage_service `DiskInfo::CreateMBRVolume` 对齐 |

## 分区 diff

`ComputePartitionDiff`（`uevent_bootstrap.cpp:682-706`）在 `diskPartsSnapshot_` 中按 `PartitionKeyEqual`（partitionNumber + partitionType + fsTypeRaw）做差集，持 `diskPartsSnapshotMutex_`，复杂度 O(n×m)。

| diff 结果 | 处理 |
|-----------|------|
| added | `DiscoverSinglePartitionVolume`（建节点→blkid→mount） |
| removed | `DestroyVolumeByDiskIdAndPartNum`（ForceUnmount→DestroyBlockDeviceNode） |
| 全空且 publishNew | `DiscoverWholeDiskVolume`（整盘回退挂载） |

## 分区操作约束

| 操作 | IPC code | 工具 | 约束 |
|------|----------|------|------|
| Partition（全盘重分区） | 212 | parted/sgdisk | 调用后须 `WaitForPartitionDone` 等待 uevent change 唤醒 |
| CreatePartition | 215 | sgdisk | 分区数上限 `PARTITION_COUNT_MAX = 256` |
| DeletePartition | 216 | sgdisk | — |
| FormatPartition | 217 | mkfs | `FormatParams.fsType` 须为合法 FsType |

**分区同步**：`Partition` → `partitionLock_` + `partitionCv_` 等待 `NotifyPartitionDone`（`disk_manager.cpp:1207`，超时 `WAIT_UEVENT_TIMEOUT = 60s`）。`partitioningDiskIds_` 标记进行中磁盘，分区中的内置数据盘 uevent change 会被跳过。

## disk_config 匹配

`etc/disk_config` 每行格式 `sysPattern <pattern> label <label> flag <flag>`（`CONFIG_PARAM_NUM = 6`），安装到 `/system/etc/disk_manager/disk_config`。`MatchConfig`（`uevent_bootstrap.cpp:959-983`）按 devPath 子串匹配，结合 major 调整 flag（`DISK_MMC_MAJOR=179` → SD_FLAG，`DISK_CD_MAJOR=11` → CD_FLAG）。

## 修改前检查

- 卷当前处于什么状态？转换路径是否合法？
- 是否持有了正确的锁？锁顺序是否正确？
- 公共事件时序是否符合上表？
- uevent change 是否在分区进行中？是否应跳过？
- 分区操作是否通过 `partitionLock_` + `partitionCv_` 同步？
- 分区 diff 是否正确处理 added/removed/全空？

## 测试指引

- 状态转换/卷操作：`disk_manager_server_test`（源文件 `disk_manager_test.cpp`）
- uevent 解析：`uevent_bootstrap_test`、`uevent_env_parser_test`
- 公共事件：`disk_manager_common_event_publisher_test`
- 分区表解析：`partition_table_parser_test`、`partition_types_test`
- Fuzz：`partitiontableparser_fuzzer`、`partitiontypes_fuzzer`、`ueventparser_fuzzer`
- 磁盘热插拔/分区操作需板侧验证（真实 SD/USB）
