# 数据持久化

本文记录 VoldataUuidStore 持久化映射与 BlockInfoTable 内存缓存的约束。性能边界见 `storage-daemon-adapter.md`。

## VoldataUuidStore

`VoldataUuidStore`（`voldata_uuid_store.h`）是单例 `final`，维护卷 fsUuid 与 `/mnt/data/voldata/dataX` 挂载路径的持久化映射。

| 项 | 值 |
|----|-----|
| 持久化文件 | `/data/service/el1/public/disk_manager/voldata_uuid_mapping.json` |
| 格式 | JSON 数组 |
| Slot 上限 | `MAX_VOLDATA_SLOT_COUNT = 1000` |
| 路径前缀 | `/mnt/data/voldata/data` |
| Slot 范围 | 1..1000（纯数字后缀） |

### 原子写约束

写入必须走 `.tmp` → `fsync` → `rename` 覆盖。写失败时回滚内存 map（先备份 `backup = uuidMap_`，失败后 `uuidMap_ = std::move(backup)`）。

### Slot 分配与淘汰

| 场景 | 处理 |
|------|------|
| 新 fsUuid | `FindMinimumFreeSlotLocked` 顺序扫描 1..1000 分配最小空闲 slot |
| 满 1000 | `EvictSlotOneIfFullLocked` 优先淘汰 `slotIndex == 1` 的条目 |
| 格式化后 fsUuid 变化 | `ReplaceFsUuid` 保留原 dataX 槽位，仅更新映射键 |
| 校验 | `ParseVoldataSlotFromMountPath` 校验前缀 + 纯数字 1..1000 |

### 同步原语

| 锁 | 保护 | 类型 |
|----|------|------|
| `initMutex_` | 初始化 | `std::mutex` |
| `dataMutex_` | `uuidMap_` | `std::shared_mutex`（读写分离） |

### 接口

| 方法 | 说明 |
|------|------|
| `Init()` | 读取 JSON 文件填充内存 map |
| `ResolveMountPath(fsUuid, out, outCreated)` | 查询/分配 dataX 路径，仅新建时落盘 |
| `RemoveByFsUuid(fsUuid)` | 删除映射 |
| `ReplaceFsUuid(old, new)` | 格式化后更新映射键 |
| `TryGetMountPath(fsUuid, out)` | 只读查询 |

## BlockInfoTable

`BlockInfoTable`（`block_info_table.h`）是单例，从 storage_daemon 拉取 `BlockInfo` 按 diskId 缓存在内存，**不落盘**。

| 项 | 值 |
|----|-----|
| 缓存类型 | `std::unordered_map<std::string, BlockInfo>` |
| 截断上限 | `BLOCK_INFO_MAX_COUNT = 20` |
| 拉取方式 | IPC `GetBlockInfoByType`（payload type `"data"`） |
| JSON 解析 | nlohmann::json，支持纯数组/`{"blocks":[...]}`/单对象三种形态 |
| 字段数 | 15+（diskId/sizeBytes/vendor/model/devnum/busnum/devNode/scsiBusNum/fwVersion/ODD_INFO/interfaceType/rpm/removable/serialNumber/devicePath/port） |
| 同步 | `std::mutex`，构建在锁外，`std::move` 整表替换持锁 O(1) |

### 调用时机

| 方法 | 时机 | 开销 |
|------|------|------|
| `ReloadFromDaemon` | 仅 `OnStart` 调一次 | 一次 IPC + 全量 JSON 解析 |
| `ReadExtDiskInfoFromDaemon` | 单盘按需 | 一次 IPC + 单 disk JSON 解析 |
| `TryCopyByDiskId` | 热路径内存查 | O(1) |

**禁止在热路径频繁调用 `ReloadFromDaemon`**。

## 修改前检查

- VoldataUuidStore 写入是否走了原子写（.tmp → fsync → rename）？
- 写失败是否回滚了内存 map？
- Slot 分配是否超过 1000 上限？淘汰策略是否正确？
- BlockInfoTable 是否在热路径调用了 `ReloadFromDaemon`？

## 测试指引

- voldata：`voldata_uuid_store_test`
- blockinfo：`block_info_test`、`block_info_table_test`
