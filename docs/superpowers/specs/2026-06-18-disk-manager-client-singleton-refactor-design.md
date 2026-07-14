# DiskManagerClient Singleton Refactor Design

## Problem

`DiskManagerClient` uses `DECLARE_DELAYED_SINGLETON` 宏实现单例模式。该宏来自 OpenHarmony `<singleton.h>`，基于 `shared_ptr` + `static mutex`，存在安全风险（实例可被 `DestroyInstance` 销毁、析构时机不安全等）。

## Reference Pattern

`FileCacheAdapter`（`storage_service/services/storage_manager/include/utils/file_cache_adapter.h`）使用 Meyers' singleton（C++11 magic statics），线程安全由标准保证：

```cpp
class FileCacheAdapter final {
public:
    static FileCacheAdapter &GetInstance();
    virtual ~FileCacheAdapter() = default;
    // ...
private:
    FileCacheAdapter() = default;
    FileCacheAdapter(const FileCacheAdapter &) = delete;
    FileCacheAdapter &operator=(const FileCacheAdapter &) = delete;
};

//cpp
FileCacheAdapter &FileCacheAdapter::GetInstance()
{
    static FileCacheAdapter instance_;
    return instance_;
}
```

## Design

### 1. disk_manager_client.h Changes

- Remove `#include <singleton.h>`
- Remove `DECLARE_DELAYED_SINGLETON(DiskManagerClient)`
- Add public `static DiskManagerClient &GetInstance();`
- Add private `DiskManagerClient() = default;`
- Change destructor `~DiskManagerClient()` to virtual
- Remove `DISALLOW_COPY_AND_ASSIGN(DiskManagerClient)` (already inherited from `NoCopyable` base class)

### 2. disk_manager_client.cpp Changes

- Add `GetInstance()` implementation:
```cpp
DiskManagerClient &DiskManagerClient::GetInstance()
{
    static DiskManagerClient instance_;
    return instance_;
}
```
- In DmDeathRecipient::OnRemoteDied, change:
```cpp
DiskManagerClient::GetInstance().ResetProxy();
```
  instead of:
```cpp
DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
client.ResetProxy();
```

### 3. disk_manager 仓调用点 Changes (4 files)

| File | Old | New |
|-----|-----|-----|
| `interfaces/kits/js/volume_manager/src/volumemanager_n_exporter.cpp` | `DelayedSingleton<DiskManagerClient>::GetInstance()->Method()` | `DiskManagerClient::GetInstance().Method()` |
| `test/unittest/disk_manager_client_test.cpp` (30+ occurrences) | `*DelayedSingleton<DiskManagerClient>::GetInstance()` | `DiskManagerClient::GetInstance()` |

### 4. storage_service 仓调用点 Changes (3 files)

| File | Old | New |
|-----|-----|-----|
| `services/storage_manager/ipc/src/storage_manager_provider.cpp` (15 occurrences) | `DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->Method()` | `OHOS::DiskManager::DiskManagerClient::GetInstance().Method()` |
| `services/storage_daemon/mtp/src/mtp_device_manager.cpp` (4 occurrences) | same as above |
| `services/storage_daemon/netlink/src/netlink_handler.cpp` (1 occurrence) | same as above |

### 5. Mock layer (unchanged)

The mock files in `storage_service/services/storage_manager/mock/` and test files are not affected because:
- `disk_manager_client_mock.h` - unchanged (still uses `disk_manager_client.h`)
- `disk_manager_client_mock.cpp` - unchanged (implements `DiskManagerClient` methods directly)
- The mock's `IDiskManagerClientMock::diskManagerClientMock` pointer pattern is independent of the singleton pattern

### 6. NoCopyable Base Class

After removing `DECLARE_DELAYED_SINGLETON` (which makes constructor/destructor private via `friend DelayedSingleton`), we need to ensure constructor/destructor are still private. Since `DiskManagerClient` currently inherits from `NoCopyable`, the constructor was already private in practice. We explicitly mark them private in the class body.

## Impact Summary

- **API change**: `DelayedSingleton<DiskManagerClient>::GetInstance()` → `DiskManagerClient::GetInstance()` (returns reference instead of `shared_ptr`)
- **No binary compatibility break**: All callers must update
- **Mock layer**: No changes needed
- **Thread safety**: Meyers' singleton guarantees thread-safe initialization (C++11 standard)
