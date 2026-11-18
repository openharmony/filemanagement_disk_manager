# 架构约束与陷阱

本文汇总 disk_manager 的架构约束与常见陷阱。各模块详细约束见对应知识文档。

## 锁顺序陷阱

`diskMapMutex_` 与 `volumeMapMutex_` 相互独立。同一流程需两把锁时必须按"先 disk 后 volume"顺序加锁，否则死锁。

**陷阱**：带 `Unlocked`/`Locked` 后缀的方法假定调用方已持锁，不要再在其内部反向获取另一把锁（如 `LookupVolumeByUuidUnlocked` 持 volumeMapMutex_ 读锁时内部再取 diskMapMutex_）。`ComputeVolumeMountPolicy`/`UpdateVoldataMappingAfterFormat` 已按序持双读锁，调用它们前不要再持锁。

## 卷状态跳转陷阱

`MOUNTED → EJECTING → REMOVED` 必须经过 EJECTING，不可直接置 REMOVED。状态切换由 `SetVolumeStateLocked` 统一执行。

**陷阱**：ForceUnmount 失败后不要停在 EJECTING，须回退到 MOUNTED 或继续到 REMOVED 取决于业务场景。

## 公共事件时序陷阱

见 `disk-volume-lifecycle.md` 公共事件时序表。核心陷阱：

- 安全弹出后再拔出：仅发 `VOLUME_REMOVED`，**不再发 EJECT/UNMOUNTED**
- 未安全弹出直接拔出：必须发完整的 `EJECT → UNMOUNTED → BAD_REMOVAL` 链
- `COMMON_EVENT_DISK_UNMOUNTABLE` 的 reasonCode 是封闭枚举（1-4）且跨版本稳定，订阅方（文件管理/设置/大桌面 APP 等）依赖取值，**不得随意新增/改动数值**

## 分区同步陷阱

`Partition` 走主动 `RediscoverDiskVolumes`；`CreatePartition`/`DeletePartition` 走 `partitionLock_` + `partitionCv_` 等待 change uevent 的 `NotifyPartitionDone` 唤醒（60s 超时）。`partitioningDiskIds_` 标记进行中磁盘。

**陷阱**：
- 分区进行中的内置数据盘 uevent change 会被 `HandleDiskChange` 跳过。如果在此跳过逻辑中添加阻塞处理，会导致 Partition 流程卡死。
- `Partition` 前有 `PurgeVolumesForDisk` 清理旧卷；改清理顺序须保证 daemon Partition 失败时卷可恢复。
- `WaitForPartitionDone` 返回后 uevent 可能尚未完全落盘（只是超时兜底），不要假设分区表必然刷新。

## 空闲卸载陷阱

`CheckAndUnloadIfIdle` 每 3 分钟检查，需同时满足 `HasManagedResources()==false` 且 `pendingStorageDaemonCallbackCount_==0`。

**陷阱**：storage_daemon 回调（OnBlockDiskUevent/NotifyMtpMounted 等）进行中时，必须通过 `BeginPendingStorageDaemonCallback`/`EndPendingStorageDaemonCallback` 维护计数，否则 SA 可能在回调进行中被卸载。

## EDM 命名与契约陷阱

`EdmAdapter::NotifyExternalStorageDeviceAdd` 调用 EDM 侧 `NotifyUnmountExternalStorageDeviceInfo`——**命名相反但契约如此**（EDM 侧接口语义是"上报外置存储设备信息"），不是 bug，不要"修复"。挂载拦截的通知失败时放行挂载（fail-open），勿改为拒绝。重挂载闭环在企业管控应用（innerkits Mount + fromEdmMount=true），本仓只认标志位。

## 对外接口兼容性陷阱

本仓对外有三层出口，兼容性约束各不相同；**凡对外有影响的修改，必须先与用户评估确认后再动手**：

| 出口 | 兼容性约束 | 原因 |
|------|-----------|------|
| JS API（`@ohos.file.volumeManager`）错误码 | **非兼容变更，严格禁止**（新增枚举值需用户确认） | 三方应用按错误码写死分支处理，改码即破坏所有调用方 |
| C++ innerkits 错误码 | 确认**不影响 JS API 映射**时可修改 | innerkits 供系统内部（daemon/EDM/系统应用）使用，错误码在 NAPI/Taihe 层映射后才对外；映射层不透出的可改 |
| 公共事件参数（字段/action/reasonCode） | 修改须与用户确认 | 文件管理器、系统设置、大桌面等消费方按字段解析，删改字段即非兼容变更 |

**陷阱**：JS 错误码与 innerkits 错误码分属两层（`disk_manager_napi_errno.h` 与 `DiskManagerErrNo`），改 innerkits 码时容易漏看 NAPI 映射——**改前先查 `napi_errno` 映射表**；公共事件的 reasonCode（如 DISK_UNMOUNTABLE 1-4）跨版本稳定，勿扩列复用。

## 数据盘格式化授权陷阱

`CheckAdminAuthorization` 最长等 3600s 用户弹窗确认。等待不阻塞其他 IPC（线程池），但：

- **勿在持 diskMapMutex_/volumeMapMutex_ 等锁的状态下调用**，否则授权期间锁被长期占用。
- 并发重入返回 `E_BUSY`（1907），勿移除该防护。

## 加密 U 盘串行陷阱

IPC 15-20 共享 `cryptVolMutex_`（provider 层）：同一时间只允许一个加密操作。**陷阱**：在该互斥区内不得再调用需同一互斥量的接口（如 MountVolumeByPath 内部触发回调链），会自锁死。整个特性仅 PC_MANAGER 平台编译，非 PC 平台返回 `E_NOT_SUPPORT`，测试须用 `PC_MANAGER` define。

## sysfs 直读陷阱

`SysfsReader` 直读 sysfs（属性文件 0444）仅限 USB 属性读取：subsystem 判定走 readlink、devtype 走 uevent 文件（无独立属性文件）。**陷阱**：不要扩展为写 sysfs 或读取任意路径（路径来自 uevent env，须防遍历）；遍历必须有层数/边界意识，确认 `subsystem=="usb" && devtype=="usb_device"` 后才读属性。

## VoldataUuidStore 原子写陷阱

写入必须走 `.tmp` → `fsync` → `rename`。写失败回滚内存 map。

**陷阱**：
- 直接写目标文件（不经过 .tmp + rename）会在写一半崩溃时损坏持久化映射
- Slot 满时 `EvictSlotOneIfFullLocked` 优先淘汰 slotIndex==1，不是 LRU
- `ParseVoldataSlotFromMountPath` 校验前缀 `/mnt/data/voldata/data` + 纯数字 1..1000，路径遍历会被拒绝
- voldata 路径仅用于**内置数据盘 + f2fs**（`ShouldUseVoldataMountPathForDiskUnlocked`），勿扩大适用范围

## dlopen 适配器生命周期陷阱

`UsbFuseAdapter`/`PcEncryptionAdapter` 通过 `Init`/`UnInit` 管理 `void *handler_`。

**陷阱**：
- `dlopen` 失败静默处理（返回 false/不支持），不抛异常
- 不得在 handler 未就绪时调用 dlsym 符号（会 segfault）
- `UnInit` 后 handler 置 nullptr，再次使用前必须重新 `Init`

## 高危删除陷阱

对路径执行不可逆删除前，必须确认目标真实后端即预期对象。

**陷阱**：挂载类间接层（设备挂载/绑定挂载/重复或多层挂载等）会使挂载点目录的真实后端≠表面。对残留挂载层的挂载点递归删会穿透到源数据导致数据丢失。FUSE 残留挂载复用死连接曾导致问题——挂载前不得仅凭 `IsPathMounted` 短路判断复用旧挂载。

`UeventBootstrap::DestroyALLVolume` 在销毁卷前必须先 `ForceUnmount`，确认无残留挂载层后再 `DestroyBlockDeviceNode`，否则禁止执行。

## 修改前检查

- 是否违反了锁顺序？授权/挂载等长等待是否在锁外？
- 状态转换是否合法？
- 公共事件时序是否正确？reasonCode 是否复用既有枚举？
- 分区同步方式是否与操作类型匹配（主动 Rediscover vs 等 uevent）？
- 空闲卸载计数是否维护？
- 原子写是否完整？
- dlopen handler 是否就绪？
- 删除前是否确认了挂载层已卸载？
