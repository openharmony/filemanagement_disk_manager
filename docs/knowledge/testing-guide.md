# 测试指南

本文记录 disk_manager 的测试覆盖、Mock 策略与 Fuzz 目标。构建命令见 `AGENTS.md`。

## 单元测试

`test/unittest/BUILD.gn` 聚合 group `disk_manager_ut`，目标数以 BUILD.gn 为准（随需求动态变化，勿在文档写死）。

### 测试目标与覆盖模块

| 目标 | 覆盖模块 |
|------|----------|
| `block_info_test` / `block_info_table_test` | BlockInfo / BlockInfoTable |
| `disk_config_test` | DiskConfig |
| `disk_manager_client_test` | DiskManagerClient |
| `disk_manager_common_event_publisher_test` | CommonEventPublisher（含异常卷广播字段断言） |
| `disk_manager_dfx_test` / `disk_manager_dfx_types_test` | DFX 聚合 / DFX 类型（含 opDiag 解析） |
| `disk_manager_provider_test` | DiskManagerProvider（鉴权、弹窗授权） |
| `disk_manager_radar_test` / `hi_audit_test` | Radar + HiAudit |
| `disk_manager_server_test` | DiskManager（源文件 disk_manager_test.cpp，覆盖挂载策略/分区/加密卷） |
| `disk_test` / `volume_core_test` / `volume_external_test` | 基础 Parcelable |
| `crypt_param_test` / `mount_param_test` | CryptParam / MountParam |
| `external_disk_info_test` / `external_volume_info_test` | 外部存储查询 Parcelable |
| `partition_table_parser_test` / `partition_types_test` | 分区表解析 / PartitionInfo 等 |
| `pc_encryption_adapter_test` / `usb_fuse_adapter_test` | dlopen 适配器 |
| `edm_adapter_test` | EdmAdapter（挂载拦截/刻录管控） |
| `sysfs_reader_test` | sysfs 直读 |
| `storage_daemon_adapter_test` / `storage_daemon_proxy_test` | StorageDaemonAdapter / Proxy |
| `uevent_bootstrap_test` / `uevent_env_parser_test` | UeventBootstrap / UeventEnvParser |
| `voldata_uuid_store_test` | VoldataUuidStore |

### Mock 注入技巧

`disk_manager_server_test` 通过 `defines` 做符号替换注入 mock：

```
defines = [
  "StorageDaemonAdapter=MockStorageDaemonAdapter",
  "UsbFuseAdapter=MockUsbFuseAdapter",
  "UeventBootstrap=MockUeventBootstrap",
  "dlopen=MockDlopen",
  "dlsym=MockDlsym",
  "dlclose=MockDlclose",
  "dlerror=MockDlerror",
]
```

`disk_manager_provider_test` 额外定义 `IPCSkeleton=MockIPCSkeleton`；`edm_adapter_test` 用 `mock_parameters.cpp` 替换 `system::GetParameter`（配合 mock_disk_manager）。部分测试用 `private = public` define 访问私有成员。

### 平台宏与测试

| define | 何时启用 | 测试影响 |
|--------|----------|----------|
| `PC_MANAGER` | target pc/2in1/tablet；unittest/BUILD.gn 按需追加 | 加密 U 盘用例须在该 define 下编译 |
| `CDC_STORAGE` | target car | DVR_USB 卷公共事件仅发布给 user 0 的用例 |
| `EDM_ADAPTER_ENABLE` | 部件含 customization_enterprise_device_management | edm_adapter_test 编译 |

## Fuzz 测试

`test/fuzztest/` 聚合 group `disk_manager_fuzztest`，目标数以 BUILD.gn 为准（随需求动态变化，勿在文档写死）。

| Fuzzer | 目标 |
|--------|------|
| `cryptparam_fuzzer` | CryptParam Parcelable |
| `disk_fuzzer` | Disk setter/getter + Marshalling/Unmarshalling |
| `diskmanagerprovider_fuzzer` | DiskManagerProvider::OnRemoteRequest（IPC code 随机） |
| `diskmanagerstub_fuzzer` | DiskManagerStub 全量 IPC code |
| `diskmanagerutils_fuzzer` | GetAnonyString/IsVolumeIdValid/IsDiskIdValid 等 |
| `externaldiskinfo_fuzzer` / `externalvolumeinfo_fuzzer` | 外部存储查询 Parcelable |
| `mountparam_fuzzer` | MountParam Parcelable |
| `partitiontableparser_fuzzer` | ParseSgdiskDump |
| `partitiontypes_fuzzer` | PartitionInfo 等 Parcelable |
| `ueventparser_fuzzer` | UeventEnvParser::Parse |
| `volumecore_fuzzer` / `volumeexternal_fuzzer` | VolumeCore + VolumeInfoStr / VolumeExternal |
| `dfxtypes_fuzzer` | DFX 类型（ParseOpDiagText/ParseOpDiagReport/DfxTruncate/跳过原因推导） |
| `voldatauuidstore_fuzzer` | VoldataUuidEntry::FromJson/ToJson 往返 + 只读查询 |
| `blockinfotable_fuzzer` | BlockInfoTable::ToJsonStringWithExtras / BlockInfo 序列化 / TryCopyByDiskId |
| `diskconfig_fuzzer` | DiskConfig::IsMatch（fnmatch 模式匹配） |
| `getdisksysinfo_fuzzer` | IPC code 54/55/56（sysPath 查询：序列号/通电次数/通电时长） |
| `volumebasicop_fuzzer` | IPC code 1-5（Mount/Unmount/Format/TryToFix/SetVolumeDescription，含 MountParam） |
| `cryptvolume_fuzzer` | IPC code 15-20（加密卷全流程：Bind/Create/Destroy/Unbind/MountVolumeByPath/UmountVolumeByPath，含 CryptParam） |
| `discop_fuzzer` | IPC code 40-44（光盘/刻录：Erase/Eject/CreateIsoImage/Burn/GetVolumeOpProcess） |
| `partitionop_fuzzer` | IPC code 46-49（分区管理：GetPartitionTable/Create/Delete/FormatPartition，含 PartitionParams/FormatParams） |)

统一配置：`cflags = ["-fno-lto", "-g", "-O0", "-Wno-unused-variable", "-fno-omit-frame-pointer"]`。

## Mock 层

`test/mock/` 提供两类 mock：

| 类型 | 示例 | 模式 |
|------|------|------|
| gmock 风格 | MockDiskManager、MockStorageDaemonAdapter、MockUsbFuseAdapter、MockBlockInfoTable、MockVoldataUuidStore、MockUeventBootstrap、MockPcEncryptionAdapter 等 | `MOCK_METHOD` + `GetInstance()` 单例 + 静态转发到 `*Impl` |
| 手写桩 | MockIPCSkeleton、MockDlfcn、MockParameters（system::GetParameter）、MockAccessTokenKit、MockSystemAbilityManager 等 | 静态成员变量注入返回值 |

`test/mock/uevent_bootstrap_mock/` 提供三个重定向头（db/disk/notification），供 `uevent_bootstrap_test` 编译时替换依赖。

## 修改前检查

- 新增的类/函数是否有对应 UT？
- Mock 注入是否正确（defines 符号替换 vs 重定向头 vs 手写桩）？
- Fuzz 是否覆盖了新的 Parcelable 或解析函数？
- 平台特有逻辑（PC_MANAGER/CDC_STORAGE/EDM）是否在对应 define 下测试？

## 构建命令

```sh
# 标准构建（源码根目录执行，含 disk_manager 与 disk_manager_test 目标）
./build.sh --product-name rk3568 --ccache --build-target disk_manager disk_manager_test --target-cpu arm --gn-args allow_sanitize_debug=true
# 构建单个测试目标（快捷方式）
prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 disk_manager_server_test
prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 DiskFuzzTest
```
