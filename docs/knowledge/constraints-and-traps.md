# 架构约束与陷阱

本文汇总 disk_manager 的架构约束与常见陷阱。各模块详细约束见对应知识文档。

## 锁顺序陷阱

`diskMapMutex_` 与 `volumeMapMutex_` 相互独立。同一流程需两把锁时必须按"先 disk 后 volume"顺序加锁（`disk_manager.h:211-212`），否则死锁。

**陷阱**：`LookupVolumeByUuidUnlocked`（调用方持 `volumeMapMutex_` 读锁）内部如果再获取 `diskMapMutex_` 会违反锁顺序。带 `Unlocked`/`Locked` 后缀的方法假定调用方已持锁，不要再在其内部反向获取另一把锁。

## 卷状态跳转陷阱

`MOUNTED → EJECTING → REMOVED` 必须经过 EJECTING，不可直接置 REMOVED。状态切换由 `SetVolumeStateLocked` 统一执行。

**陷阱**：ForceUnmount 失败后不要停在 EJECTING，须回退到 MOUNTED 或继续到 REMOVED 取决于业务场景。

## 公共事件时序陷阱

见 `disk-volume-lifecycle.md` 公共事件时序表。核心陷阱：

- 安全弹出后再拔出：仅发 `VOLUME_REMOVED`，**不再发 EJECT/UNMOUNTED**
- 未安全弹出直接拔出：必须发完整的 `EJECT → UNMOUNTED → BAD_REMOVAL` 链

## 分区同步陷阱

`Partition` 调用后通过 `partitionLock_` + `partitionCv_` 等待 `NotifyPartitionDone` 唤醒。`partitioningDiskIds_` 标记进行中磁盘。

**陷阱**：分区进行中的内置数据盘 uevent change 会被 `HandleDiskChange`（`uevent_bootstrap.cpp:845`）跳过。如果在此跳过逻辑中添加阻塞处理，会导致 Partition 流程卡死。

## 空闲卸载陷阱

`CheckAndUnloadIfIdle` 每 3 分钟检查，需同时满足 `HasManagedResources()==false` 且 `pendingStorageDaemonCallbackCount_==0`。

**陷阱**：storage_daemon 回调（OnBlockDiskUevent/NotifyMtpMounted 等）进行中时，必须通过 `BeginPendingStorageDaemonCallback`/`EndPendingStorageDaemonCallback` 维护计数，否则 SA 可能在回调进行中被卸载。

## VoldataUuidStore 原子写陷阱

写入必须走 `.tmp` → `fsync` → `rename`。写失败回滚内存 map。

**陷阱**：
- 直接写目标文件（不经过 .tmp + rename）会在写一半崩溃时损坏持久化映射
- Slot 满时 `EvictSlotOneIfFullLocked` 优先淘汰 slotIndex==1，不是 LRU
- `ParseVoldataSlotFromMountPath` 校验前缀 `/mnt/data/voldata/data` + 纯数字 1..1000，路径遍历会被拒绝

## dlopen 适配器生命周期陷阱

`UsbFuseAdapter`/`PcEncryptionAdapter` 通过 `Init`/`UnInit` 管理 `void *handler_`。

**陷阱**：
- `dlopen` 失败静默处理（返回 false/不支持），不抛异常
- 不得在 handler 未就绪时调用 dlsym 符号（会 segfault）
- `UnInit` 后 handler 置 nullptr，再次使用前必须重新 `Init`

## 高危删除陷阱

对路径执行不可逆删除前，必须确认目标真实后端即预期对象。

**陷阱**：挂载类间接层（设备挂载/绑定挂载/重复或多层挂载等）会使挂载点目录的真实后端≠表面。对残留挂载层的挂载点递归删会穿透到源数据导致数据丢失。

`UeventBootstrap::DestroyALLVolume`（`uevent_bootstrap.cpp:171`）在销毁卷前必须先 `ForceUnmount`，确认无残留挂载层后再 `DestroyBlockDeviceNode`，否则禁止执行。

## 修改前检查

- 是否违反了锁顺序？
- 状态转换是否合法？
- 公共事件时序是否正确？
- 分区同步是否完整？
- 空闲卸载计数是否维护？
- 原子写是否完整？
- dlopen handler 是否就绪？
- 删除前是否确认了挂载层已卸载？
