# disk_manager.cpp 全覆盖 UT 测试设计

## 目标

为 `disk_manager.cpp` 中每个 public 和 private 方法编写全面覆盖所有分支路径的单元测试用例，确保所有用例能编译通过并运行成功。

## Mock 策略

采用 **源码直接编译 + Mock 依赖注入** 方案：

1. `disk_manager.cpp` 等源码直接编译进 UT 目标
2. 创建 Mock 类替代依赖单例，通过编译时链接替换注入
3. Mock 类覆盖所有 IPC 调用，使测试不依赖真实系统服务

### Mock 依赖清单

| 依赖 | Mock 方式 | 说明 |
|------|----------|------|
| StorageDaemonAdapter | 源码替换编译 | 创建 mock_storage_daemon_adapter.cpp 替代真实实现 |
| UsbFuseAdapter | 源码替换编译 | 创建 mock_usb_fuse_adapter.cpp 替代真实实现 |
| VoldataUuidStore | 源码替换编译 | 创建 mock_voldata_uuid_store.cpp 替代真实实现 |
| BlockInfoTable | 源码替换编译 | 创建 mock_block_info_table.cpp 替代真实实现 |
| CommonEventPublisher | 源码链接 | 不 mock，使用 EXPECT_NO_THROW 验证 |
| parameter (FindParameter/GetParameterValue) | 源码链接或 stub | ReadPersistUsbReadonlyMountFlagBits 依赖 |

### DiskManager 单例状态管理

DiskManager 是单例，测试间需要清理内部状态。方案：
- 在每个 TestFixture::SetUp 中通过 OnDiskDestroyed/OnVolumeDestroyed 清理残留数据
- 对于无法通过 public API 清理的 partitionTableMap_，通过测试顺序设计避免冲突

## 测试用例分组

### 1. Disk CRUD（约8个用例）
- OnDiskCreated: 正常创建、重复创建(E_DISK_HAS_EXIST)
- HasDisk: 存在/不存在
- OnDiskDestroyed: 正常销毁、不存在销毁(E_DISK_NOT_FOUND)
- GetDiskById: 存在(附带volumeIds)/不存在
- GetAllDisks: 空map/有多个disk

### 2. Volume CRUD（约12个用例）
- OnVolumeCreated: 正常创建(附带flags解析)
- OnVolumeDestroyed: 正常销毁/不存在(E_NON_EXIST)
- GetVolumeById: 存在/不存在
- GetVolumeByUuid: 存在/不存在
- GetAllVolumes: 空map/有普通卷/有CD卷生成虚拟DVD卷
- UpdateVolumeMetadata: 存在更新/不存在(E_NON_EXIST)

### 3. Mount 系列（约10个用例）
- Mount: volume不存在(E_NON_EXIST)、状态不允许(E_VOL_MOUNT_ERR)、正常挂载
- MountVolumeEntry: uuid获取失败、unsafe uuid、正常流程
- MountVolumeFilesystem: fuse挂载失败、Mount失败(voldata清理)、正常挂载(各policy路径)
- BuildMountDataPath: useVoldataPath/useDvrPath/useFuseData/默认路径

### 4. Unmount 系列（约6个用例）
- Unmount: volume不存在(E_NON_EXIST)、状态不允许(E_VOL_UMOUNT_ERR)、正常卸载
- ResolveUnmountForceFlag: 非内部盘(force=true)、内部盘inUse(E_UMOUNT_BUSY)、正常
- UnmountVolumeMountPoints: fuse路径卸载、普通路径卸载

### 5. Format 系列（约5个用例）
- Format: MTP不支持(E_NOT_SUPPORT)、CD不支持(E_NOT_SUPPORT)、状态不对(E_VOL_STATE)、正常格式化
- UpdateVoldataMappingAfterFormat: 各种 fsUuid 组合

### 6. TryToFix（约4个用例）
- volume不存在(E_NON_EXIST)
- mounted状态下unmount成功+repair+remount
- mounted状态下unmount失败
- repair失败

### 7. Partition/PurgeVolumes（约4个用例）
- PurgeVolumesForDisk: disk不存在(E_DISK_NOT_FOUND)、正常清理
- Partition: CD不支持(E_NOT_SUPPORT)、盘有mounted卷(E_VOL_STATE)、正常分区

### 8. SetVolumeDescription（约5个用例）
- UUID不存在(E_NON_EXIST)
- 状态不对(E_VOL_STATE)
- disk不支持(CD E_NOT_SUPPORT)
- fsType不支持(E_NOT_SUPPORT)
- 正常设置

### 9. 分区表操作（约10个用例）
- GetPartitionTable: disk不存在、IPC失败、解析失败、正常
- CreatePartition: disk不存在、类型不支持、参数无效、正常
- DeletePartition: disk不存在、类型不支持、partition不存在、正常
- FormatPartition: disk不存在、类型不支持、partition不存在、正常
- IsParamsValid: 各种sector范围/typeCode组合
- ParsePartitionInfo: 正常/空字符串/格式错误
- SetSectorSize/SetAlignSector/SetUsableSector: 各种解析情况

### 10. Misc 辅助方法（约10个用例）
- IsSafeFsUuid: 正常/含../含/
- IsOddFsType: udf/iso9660/其他
- IsPathMounted: 空path/存在路径/不存在路径
- EnsureFsUuidReady: uuid已存在/ReadMetadata失败/正常
- IsDiskNotReady: disk不存在/volume已mount/volume全unmount
- IsVolumeMounted: disk不存在/mounted/unmounted
- LookupVolumeByUuidUnlocked: 存在/不存在
- ComputeVolumeMountPolicy: 各种disk+fsType组合
- ShouldUseVoldataMountPathForDiskUnlocked: 各种组合

### 11. 空壳/简单方法（约7个用例）
- EraseVolume/EjectVolume/CreateIsoImage/BurnVolume: 返回E_OK
- GetVolumeOpProcess: progressPct=0
- VerifyBurnData: 返回E_OK
- ReplacePartitionsForDisk: 返回E_OK

### 12. NotifyMtp（约4个用例）
- NotifyMtpMounted: mtpfs类型/gphotofs类型/其他类型
- NotifyMtpUnmounted: 正常卸载/bad removal/volume不存在

### 13. 容量查询（约6个用例）
- GetFreeSizeOfVolume: volume不存在/path空/statvfs失败/正常/oddFsType
- GetTotalSizeOfVolume: volume不存在/path空/statvfs失败/正常/oddFsType

## 文件结构

新增文件：
- `test/unittest/disk_manager_test.cpp`
- `test/mock/mock_storage_daemon_adapter.h`
- `test/mock/mock_storage_daemon_adapter.cpp`
- `test/mock/mock_usb_fuse_adapter.h`
- `test/mock/mock_usb_fuse_adapter.cpp`

修改文件：
- `test/unittest/BUILD.gn`（新增 disk_manager_test target）
- `test/mock/BUILD.gn`（新增 mock 源文件）

## BUILD.gn 设计

新增 `ohos_unittest("disk_manager_test")` target：
- sources: disk_manager_test.cpp + disk_manager.cpp + 其他必要源码 + mock 替代源码
- configs: disk_manager_unittest_common
- external_deps: 同 common_event_publisher_test，额外加 gmock_main