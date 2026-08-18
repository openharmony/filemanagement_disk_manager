# 测试指南

本文记录 disk_manager 的测试覆盖、Mock 策略与 Fuzz 目标。构建命令见 `AGENTS.md`。

## 单元测试

`test/unittest/BUILD.gn` 聚合 group `disk_manager_ut`，共 23 个 `ohos_unittest` 目标。

### 测试目标与覆盖模块

| 目标 | 覆盖模块 |
|------|----------|
| `block_info_test` | BlockInfo |
| `block_info_table_test` | BlockInfoTable |
| `disk_config_test` | DiskConfig |
| `disk_manager_client_test` | DiskManagerClient |
| `disk_manager_common_event_publisher_test` | CommonEventPublisher |
| `disk_manager_dfx_test` | DFX 聚合 |
| `disk_manager_dfx_types_test` | DFX 类型 |
| `disk_manager_provider_test` | DiskManagerProvider |
| `disk_manager_radar_test` | Radar + HiAudit |
| `disk_manager_server_test` | DiskManager（源文件 disk_manager_test.cpp） |
| `disk_test` | Disk |
| `hi_audit_test` | HiAudit |
| `partition_table_parser_test` | PartitionTableParser |
| `partition_types_test` | PartitionInfo 等 |
| `pc_encryption_adapter_test` | PcEncryptionAdapter |
| `storage_daemon_adapter_test` | StorageDaemonAdapter |
| `storage_daemon_proxy_test` | StorageDaemonProxy |
| `uevent_bootstrap_test` | UeventBootstrap |
| `uevent_env_parser_test` | UeventEnvParser |
| `usb_fuse_adapter_test` | UsbFuseAdapter |
| `voldata_uuid_store_test` | VoldataUuidStore |
| `volume_core_test` | VolumeCore |
| `volume_external_test` | VolumeExternal |

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
]
```

`disk_manager_provider_test` 额外定义 `IPCSkeleton=MockIPCSkeleton`。

部分测试用 `private = public` define 访问私有成员。

### 车机编译

`car_device_enable`（`target_platform == "car"`）为 true 时追加 `CDC_STORAGE` define，DVR_USB 卷公共事件仅发布给 user 0。

## Fuzz 测试

`test/fuzztest/` 聚合 group `disk_manager_fuzztest`，共 9 个 fuzzer。

| Fuzzer | 目标 |
|--------|------|
| `disk_fuzzer` | Disk setter/getter + Marshalling/Unmarshalling |
| `diskmanagerprovider_fuzzer` | DiskManagerProvider::OnRemoteRequest（IPC code 随机） |
| `diskmanagerstub_fuzzer` | DiskManagerStub 全量 IPC code |
| `diskmanagerutils_fuzzer` | GetAnonyString/IsVolumeIdValid/IsDiskIdValid 等 |
| `partitiontableparser_fuzzer` | ParseSgdiskDump |
| `partitiontypes_fuzzer` | PartitionInfo 等 4 个 Parcelable |
| `ueventparser_fuzzer` | UeventEnvParser::Parse |
| `volumecore_fuzzer` | VolumeCore + VolumeInfoStr |
| `volumeexternal_fuzzer` | VolumeExternal |

统一配置：`cflags = ["-fno-lto", "-g", "-O0", "-Wno-unused-variable", "-fno-omit-frame-pointer"]`。

## Mock 层

`test/mock/` 提供两类 mock：

| 类型 | 示例 | 模式 |
|------|------|------|
| gmock 风格 | MockDiskManager、MockStorageDaemonAdapter、MockUsbFuseAdapter 等 | `MOCK_METHOD` + `GetInstance()` 单例 + 静态转发到 `*Impl` |
| 手写桩 | MockIPCSkeleton、MockDlfcnConfig、MockFindParameter 等 | 静态成员变量注入返回值 |

`test/mock/uevent_bootstrap_mock/` 提供三个重定向头（db/disk/notification），供 `uevent_bootstrap_test` 编译时替换依赖。

## 修改前检查

- 新增的类/函数是否有对应 UT？
- Mock 注入是否正确（defines 符号替换 vs 重定向头）？
- Fuzz 是否覆盖了新的 Parcelable 或解析函数？
- 车机特有逻辑是否在 `CDC_STORAGE` 下测试？

## 构建命令

```sh
# 单元测试
prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 disk_manager_server_test
# Fuzz
prebuilts/build-tools/linux-x86/bin/ninja -C out/rk3568 DiskFuzzTest
```
