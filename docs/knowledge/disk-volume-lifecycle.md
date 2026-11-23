# 磁盘与卷生命周期

本文记录卷状态转换、挂载策略、uevent 热插拔流程、公共事件时序（含异常卷广播）、锁模型与分区管理约束。错误码见 `AGENTS.md`，IPC 契约见 `storage-daemon-adapter.md`，约束陷阱汇总见 `constraints-and-traps.md`。

## 状态转换约束

卷状态转换必须经过合法路径，不可跳步（`interfaces/innerkits/include/volume_core.h` 的 `VolumeState`，10 态；对外公开状态经 `MapToPublicVolumeState` 收敛为 4 态：0=unmounted、1=checking、2=mounted、3=ejecting）：

| 操作 | 必经路径 | 禁止跳转 |
|------|----------|----------|
| 挂载 | UNMOUNTED → CHECKING → MOUNTED | 禁止 UNMOUNTED → MOUNTED |
| 卸载 | MOUNTED → EJECTING → REMOVED | 禁止 MOUNTED → REMOVED |
| 异常拔出 | MOUNTED → BAD_REMOVAL | 无需经过 EJECTING |
| 异常标记 | UNMOUNTED → DAMAGED | `MarkVolumeAbnormal` 专用，幂等（已 DAMAGED 跳过重发布） |

状态切换由 `SetVolumeStateLocked`（调用方持 `volumeMapMutex_` 锁）统一执行。操作失败时回退到操作前状态，不要停留在中间状态。

**DAMAGED 状态定义**：设备接入后自动挂载失败的状态（由 `MarkVolumeAbnormal` 标记，reason 见"异常卷标记与广播"节），此时卷从未挂载成功、无挂载点，本质是 **UNMOUNTED 的特殊版本**。原则：**放行 UNMOUNTED 状态的操作，预期也应该放行 DAMAGED**——新增或修改卷状态限制时按此原则同步评估 DAMAGED 的放行豁免（当前豁免清单见"异常卷标记与广播"节）。

## 锁模型

| 锁 | 保护对象 | 类型 | 注意 |
|----|----------|------|------|
| `diskMapMutex_` | `diskMap_` | `std::shared_mutex` | 读写分离 |
| `volumeMapMutex_` | `volumeMap_` | `std::shared_mutex` | 读写分离 |
| `partitionLock_` | `partitionDoneMap_`、`partitionTableMap_`、`partitioningDiskIds_` | `std::mutex` | 配合 `partitionCv_` |
| `oddMutex_` | odd 状态 | `std::shared_mutex` | 光驱特有 |
| `cryptVolMutex_` | 加密 U 盘六步操作串行化 | `std::mutex` | provider 层，PC_MANAGER |

**锁顺序**：同一流程需 `diskMapMutex_` 和 `volumeMapMutex_` 两把锁时，必须按"先 disk 后 volume"顺序加锁（见 `disk_manager.h` 注释），否则死锁。`partitionLock_` 独立于上述两把锁。

## uevent 热插拔流程

入口 `UeventBootstrap::OnBlockDiskUevent`，用 FNV-1a 哈希分发 action：

| action | 处理函数 | 关键流程 |
|--------|----------|----------|
| add | `HandleDiskAdd` | `DiscoverPartitionsAndVolumes`（建节点→sgdisk→blkid→按策略挂载） |
| remove | `HandleDiskRemove` | `DestroyALLVolume`（ForceUnmount→DestroyBlockDeviceNode）→ `DestroyALLDisk` |

`DestroyALLVolume` 卸载豁免：**UNMOUNTED（已安全弹出）与 DAMAGED（从未挂载成功）卷跳过 ForceUnmount**，直接销毁设备节点并发 REMOVED；其余状态（MOUNTED 等）先强制卸载再销毁、发 BAD_REMOVAL（残留挂载层递归删会穿透源数据，勿对 MOUNTED 卷跳过）。分区 diff 移除（`DestroyVolumeByDiskIdAndPartNum`）遵循同一豁免规则。
| change | `HandleDiskChange` | `IsPartitioning(diskId) && 内置数据盘` 则跳过；否则 `DiscoverPartitionsAndVolumes` + `NotifyPartitionDone` |

**diskId 生成**：`DiskIdFrom(maj, min)` → `"disk-{maj}-{min}"`；**volId 生成**：`VolIdFromDev(dev_t)` → `"vol-{maj}-{min}"`；**加密卷 volId**：`"vol-crypt-{loop设备名}"`（BindBlockLoopDev 创建）。

## 挂载策略（ComputeVolumeMountPolicy）

`DiskManager::ComputeVolumeMountPolicy(diskId, fsType)` 决定卷挂载方式，策略互斥、优先级如下：

| 优先级 | 策略 | 条件 | 挂载路径 |
|--------|------|------|----------|
| 1 | useVoldataPath | 内置数据盘（`IsInternalDataDisk`）+ f2fs | `/mnt/data/voldata/dataX`（VoldataUuidStore 映射） |
| 2 | useDvrPath | 磁盘类型 DVR_USB（车机） | `/mnt/data/dvr/` |
| 3 | useFuseData | 非上述场景 + 非内置盘非 f2fs + `UsbFuseAdapter::IsUsbFuseEnabledForFsType` 允许 | `/mnt/data/external_fuse/`（经 FUSE） |
| 4 | 直挂 | 其余 | `/mnt/data/external/`（`EXTERNAL_MOUNT_ROOT`） |

- **bypassTobForPcNonDataFs**：PC 内置盘 + 非 f2fs 时不走 FUSE 直接挂载。PC 产品分 ToC（消费者）/ToB（政企，更注重安全性）两类，该分支对两者生效。
- **useFuseData 分支不套用 USB 只读持久化参数**（`ReadPersistUsbReadonlyMountFlagBits`），与历史行为保持一致。
- **fsUuid 去重**：`DedupFsUuidUnlocked` 在写 volumeMap_ 时发现 uuid 与其他卷重复，改写为 `原UUID_序号`。
- 格式化后：内置数据盘 f2fs 场景 `UpdateVoldataMappingAfterFormat` 用 `ReplaceFsUuid` 保留 dataX 槽位；非 voldata 场景 `RemoveByFsUuid` 删除旧映射。

## 卸载前置检查（ResolveUnmountForceFlag）

内置数据盘（DATA_DISK_SSD/DATA_DISK_HDD）卸载前必须经 storage_daemon `QueryUsbIsInUse` 查询占用：in use → 返回 `E_UMOUNT_BUSY` 拒绝卸载（防数据损坏，判定依据在 storage_daemon 侧）；查询失败 → 透传错误。外置盘不查（可直接拔）。

## 异常卷标记与广播

发现链路（uevent_bootstrap）中按 U 盘识别顺序逐级检测，命中即调 `DiskManager::MarkVolumeAbnormal(volId, reason)`（仅 UNMOUNTED→DAMAGED，幂等）并终止该卷发现流程：

| AbnormalReason | 值 | 检测点 |
|----------------|----|--------|
| PARTITION_TABLE_READ_FAILED | 1 | sgdisk 查询失败或解析不出可用分区（盘级，无卷载体） |
| METADATA_READ_FAILED | 2 | blkid 元数据读取失败 / uuid、fsType 为空 |
| FSTYPE_UNSUPPORTED | 3 | fsType 不在支持范围 |
| MOUNT_FAILED | 4 | 发现期自动挂载失败 |

- **卷级广播** `PublishVolumeAbnormal`：action `COMMON_EVENT_DISK_UNMOUNTABLE`，字段 id/diskId/fsUuid/flags/volumeState(=DAMAGED)/reasonCode/reason(英文短语)/devPath(`/dev/block/{volId}`)。
- **盘级广播** `PublishDiskAbnormal`：分区表不可用时发布，仅 diskId/volumeState/reasonCode/reason/devPath。
- **reasonCode 是封闭枚举、跨版本稳定**，变更须走设计文档修订。系统广播，任意有权限订阅方可消费（已知：文件管理 APP、系统设置 APP、大桌面 APP、文件管理服务 SA、文件监听服务 SA）。
- `EvaluateVolumeMetadataAbnormal`/`MountDiscoveredVolume` 仅对外置卷生效（isUserData 直接放行）；`IsAbnormalDetectableFsType` 之外的 fsType 不标记。
- 自动挂载门禁：`AUTO_MOUNT_EXTERNAL_VOLUMES = true` 且 uuid/fsType 均非空；跳过时上报 `ReportDiscoverAutoMountSkipFault`（AutoMountSkipReason 区分原因）。
- **DAMAGED 是 UNMOUNTED 的特殊版本**：所有仅针对 UNMOUNTED 的状态限制对 DAMAGED 同等放开——
  - `Mount` 允许重试挂载（MOUNT_FAILED 型可能恢复成功）；挂载失败沿用既有回退路径置回 UNMOUNTED（保守处理：DAMAGED 卷失败后转为 UNMOUNTED，不标记异常、不重发广播，避免兼容性影响）；成功后并发校验同样豁免 DAMAGED。
  - `SetVolumeDescription` 允许修改卷标（label 工具直写元数据区，不依赖挂载）；`LABEL_SUPPORTED_FS_TYPES`（ntfs/exfat/hmfs/f2fs）白名单仍把关，ext4 无 SetLabel 实现（存量缺口）。
  - `UmountVolumeByPath` 与 UNMOUNTED 同等按"已卸载"拒绝（`E_VOL_STATE`），避免空转 `DoUnmountVolume`。
  - 既有豁免：`Unmount` 幂等返回 E_OK；`Format`/`Partition`/`FormatPartition` 操作白名单已含 DAMAGED。

## sysfs 直读（SysfsReader）

`SysfsReader::ReadUsbInfo(sysPath, devName)` 直读 sysfs 获取 USB 设备 vid/pid/serialNumber/busnum/devnum（供 EDM 刻录白名单、Disk 信息上报使用），非 USB 设备返回全空结构体：

- 从 block 设备 sysfs 路径**向上遍历**，每层取 `subsystem`（readlink 取 basename，与 systemd sd-device 一致）和 `devtype`（读 uevent 文件 DEVTYPE= 行）。
- 以 `subsystem=="usb" && devtype=="usb_device"` 为精确判据确认 USB 设备节点后读取属性。
- sysfs USB 属性文件权限 0444，进程可直接读；**只读属性文件，不做任何写操作**。

## 公共事件时序

拔插/安全弹出的公共事件时序不可错（`common_event_publisher.cpp` 注释）：

| 场景 | 事件序列 |
|------|----------|
| 接入 | `DISK_MOUNTED` → 各卷 `VOLUME_MOUNTED` |
| 安全弹出（Unmount） | `VOLUME_EJECT` → `VOLUME_UNMOUNTED`（设备仍在） |
| 安全弹出后拔出 | 仅 `VOLUME_REMOVED`（随后 `DISK_REMOVED`），不再发 EJECT/UNMOUNTED |
| 未安全弹出直接拔出 | `VOLUME_EJECT` → `VOLUME_UNMOUNTED` → `VOLUME_BAD_REMOVAL`（随后 `DISK_REMOVED`） |
| 卷不可识别 | `COMMON_EVENT_DISK_UNMOUNTABLE`（见上文异常卷广播） |
| 卷不可识别后拔出 | 仅 `VOLUME_REMOVED`（随后 `DISK_REMOVED`）：DAMAGED 卷从未挂载，无卸载动作，不发 EJECT/UNMOUNTED/BAD_REMOVAL |

## 分区表解析

`PartitionTableParser::ParseSgdiskDump`（`partition_table_parser.cpp`）解析 storage_daemon 返回的 sgdisk dump 文本，输出 `PartitionRecord` 列表与 tableType（gpt/mbr）。纯字符串解析，无 IPC、无内核调用。

| 项 | 说明 |
|----|------|
| 输入 | `StorageDaemonAdapter::ReadPartitionTable`（IPC code 203）或 `GetPartitionTableInfo`（214）返回的原始文本 |
| 输出 | `std::vector<PartitionRecord>`（partitionNumber、partitionType、fsTypeRaw） |
| MBR 支持 | `IsMbrTypeSupportedForVolume` 白名单判断，与 storage_service `DiskInfo::CreateMBRVolume` 对齐 |

## 分区 diff

`ComputePartitionDiff` 在 `diskPartsSnapshot_` 中按 `PartitionKeyEqual`（partitionNumber + partitionType + fsTypeRaw）做差集，持 `diskPartsSnapshotMutex_`，复杂度 O(n×m)。

| diff 结果 | 处理 |
|-----------|------|
| added | `DiscoverSinglePartitionVolume`（建节点→blkid→mount） |
| removed | `DestroyVolumeByDiskIdAndPartNum`（ForceUnmount→DestroyBlockDeviceNode） |
| 全空且 publishNew | `DiscoverWholeDiskVolume`（整盘回退挂载） |

## 分区操作约束

| 操作 | IPC code | 工具 | 同步方式 |
|------|----------|------|----------|
| Partition（全盘重分区） | 212 | parted/sgdisk | **主动** `RediscoverDiskVolumes`（构造 change env 复用 Discover，不发布新盘事件） |
| CreatePartition | 215 | sgdisk | `WaitForPartitionDone` 等 uevent change，60s 超时 |
| DeletePartition | 216 | sgdisk | 同上 |
| FormatPartition | 217 | mkfs | 无需等待；成功后 `RefreshVolumeAfterFormatPartition` 刷新卷信息 |

- `Partition` 流程：`AddPartitioningDisk` → `PurgeVolumesForDisk`（按 disk.volumeIds 清理 volumeMap_ 与 voldata 映射）→ daemon `Partition(diskPath, "hmfs")` → `RediscoverDiskVolumes` → `RemovePartitioningDisk`。
- `partitioningDiskIds_` 标记进行中磁盘；期间该盘 uevent change 被 `HandleDiskChange` 跳过（由 Rediscover 主动处理，防止竞争）。
- 分区数上限 `PARTITION_COUNT_MAX = 256`；`FormatParams.fsType` 须为合法 FsType。

## disk_config 匹配

`etc/disk_config` 每行格式 `sysPattern <pattern> label <label> flag <flag>`（`CONFIG_PARAM_NUM = 6`），安装到 `/system/etc/disk_manager/disk_config`。`MatchConfig` 按 devPath 子串匹配，结合 major 调整 flag（`DISK_MMC_MAJOR=179` → SD_FLAG，`DISK_CD_MAJOR=11` → CD_FLAG）。

**类型判定按接入总线/桥接方式，不按物理介质**：机器自带卡槽直连的裸 SD 卡为 SD_FLAG（MMC 控制器，major 179）；主板槽位直连的 SSD/HDD 为 DATA_DISK_SSD/DATA_DISK_HDD（仅 PC 形态）。**凡经 USB/Type-C 等外置接口桥接接入的设备——读卡器中的 SD 卡、外接 SSD/HDD 移动硬盘——均呈现为 USB_FLAG**，勿按物理介质理解类型。

## MTP/PTP 注入链路（非 uevent）

MTP/PTP 设备不走 uevent、不在 DiskType 枚举与 disk_config 白名单内。**挂载/卸载逻辑由存储管理部件执行**（daemon 侧 mtpfs/gphotofs FUSE 挂载），完成后通知本仓管理状态：

- `NotifyMtpMounted`（uid=0 daemon 回调）：本仓 getxattr 取设备友好名 → 建 VolumeExternal（`fsType` 按 `mtpfs`→MTP / `gphotofs`→PTP 映射，状态 MOUNTED）→ 入 volumeMap_ → 发 MOUNTED 广播。**本仓不做任何挂载动作**。
- `NotifyMtpUnmounted`：按 `isBadRemove` 发 UNMOUNTED/BAD_REMOVAL 广播并移除卷。
- 应用侧卸载**复用 volumeManager 的 unmount 接口能力**（走通用 Unmount 流程）。

## 修改前检查

- 卷当前处于什么状态？转换路径是否合法？
- 是否持有了正确的锁？锁顺序是否正确？
- 挂载策略四个分支的条件是否被破坏？ToB bypass 逻辑是否影响？
- 公共事件时序是否符合上表？reasonCode 是否复用既有枚举？
- 内置数据盘卸载是否经过 in-use 检查？
- 分区操作同步方式是否正确（Partition 主动 Rediscover / Create、Delete 等 uevent）？

## 测试指引

- 状态转换/卷操作/挂载策略：`disk_manager_server_test`（源文件 `disk_manager_test.cpp`）
- uevent 解析/异常卷检测：`uevent_bootstrap_test`、`uevent_env_parser_test`
- sysfs 直读：`sysfs_reader_test`
- 公共事件（含异常卷广播）：`disk_manager_common_event_publisher_test`
- 分区表解析：`partition_table_parser_test`、`partition_types_test`
- Fuzz：`partitiontableparser_fuzzer`、`partitiontypes_fuzzer`、`ueventparser_fuzzer`
- 磁盘热插拔/分区操作需板侧验证（真实 SD/USB）
