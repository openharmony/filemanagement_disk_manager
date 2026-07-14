# DiskManagerClient Singleton Refactor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace `DECLARE_DELAYED_SINGLETON` in `DiskManagerClient` with Meyers' singleton pattern (`static local variable`), matching `FileCacheAdapter`'s approach, and update all callers in both `disk_manager` and `storage_service` repos.

**Architecture:** Remove `#include <singleton.h>` and `DECLARE_DELAYED_SINGLETON` macro from `DiskManagerClient`, add explicit `static DiskManagerClient &GetInstance()` method and private constructor/destructor. All callers change from `DelayedSingleton<DiskManagerClient>::GetInstance()->Method()` to `DiskManagerClient::GetInstance().Method()` (reference instead of `shared_ptr` dereference).

**Tech Stack:** C++11 Meyers' singleton, OpenHarmony native build (`hb build`)

---

### Task 1: Modify disk_manager_client.h — Replace singleton pattern

**Files:**
- Modify: `/home/luna/filemanagement_disk_manager/interfaces/innerkits/include/disk_manager_client.h`

- [ ] **Step 1: Edit disk_manager_client.h**

Remove these lines:
- `#include <singleton.h>` (line 26)
- `DECLARE_DELAYED_SINGLETON(DiskManagerClient);` (line 39)

Add these lines in the public section (after the class declaration `{`):
```cpp
    static DiskManagerClient &GetInstance();
    virtual ~DiskManagerClient();
```

Add these lines in the private section (before `int32_t Connect`):
```cpp
    DiskManagerClient() = default;
    DiskManagerClient(const DiskManagerClient &) = delete;
    DiskManagerClient &operator=(const DiskManagerClient &) = delete;
```

The final `disk_manager_client.h` should look like:

```cpp
#ifndef OHOS_FILEMANAGEMENT_DISK_MANAGER_CLIENT_H
#define OHOS_FILEMANAGEMENT_DISK_MANAGER_CLIENT_H

#include <mutex>
#include <string>
#include <vector>

#include <iremote_object.h>
#include <nocopyable.h>
#include <refbase.h>

#include "disk.h"
#include "partition_types.h"
#include "volume_external.h"

namespace OHOS {
namespace DiskManager {

class IDiskManager;

class DiskManagerClient : public NoCopyable {
public:
    static DiskManagerClient &GetInstance();
    virtual ~DiskManagerClient();

    /* ---------- Volume（@ohos.file.volumeManager） ---------- */
    int32_t Mount(const std::string &volumeId);
    int32_t Unmount(const std::string &volumeId);
    int32_t Format(const std::string &volumeId, const std::string &fsType);
    int32_t SetVolumeDescription(const std::string &fsUuid, const std::string &description);
    int32_t GetAllVolumes(std::vector<VolumeExternal> &vecOfVol);
    int32_t GetVolumeByUuid(const std::string &uuid, VolumeExternal &vc);
    int32_t GetVolumeById(const std::string &volumeId, VolumeExternal &vc);

    int32_t GetFreeSizeOfVolume(const std::string &volumeUuid, int64_t &freeSize);
    int32_t GetTotalSizeOfVolume(const std::string &volumeUuid, int64_t &totalSize);

    /* ---------- Disk ---------- */
    int32_t GetAllDisks(std::vector<Disk> &vecOfDisk);
    int32_t GetDiskById(const std::string &diskId, Disk &disk);
    int32_t Partition(const std::string &diskId, int32_t type);

    /* ---------- Partition ---------- */
    int32_t GetPartitionTable(const std::string &diskId, PartitionTableInfo &out);
    int32_t CreatePartition(const std::string &diskId, const PartitionParams &params);
    int32_t DeletePartition(const std::string &diskId, int32_t partitionNum);
    int32_t FormatPartition(const std::string &diskId, int32_t partitionNum, const FormatParams &params);

    /* ---------- Optical Disc / Burn ---------- */
    int32_t Erase(const std::string &volumeId);
    int32_t Eject(const std::string &diskId);
    int32_t CreateIsoImage(const std::string &volumeId, const std::string &filePath);
    int32_t Burn(const std::string &volumeId, const std::string &burnOptions);
    int32_t GetVolumeOpProcess(const std::string &volumeId, int32_t &progressPct);
    int32_t VerifyBurnData(const std::string &volumeId, int32_t verifyType);

    int32_t ResetProxy();
    int32_t TryToFix(const std::string &volumeId);
    int32_t QueryUsbIsInUse(const std::string &mountPath, bool &isInUse);
    int32_t IsUsbFuseByType(int32_t type, bool &isUsbFuse);

    /* ---------- storage_daemon callbacks ---------- */
    int32_t NotifyMtpMounted(const std::string &id, const std::string &path, const std::string &desc,
                             const std::string &uuid, const std::string &fsType);
    int32_t NotifyMtpUnmounted(const std::string &id, const bool isBadRemove);
    int32_t OnBlockDiskUevent(const std::string &rawUeventMsg);

private:
    DiskManagerClient() = default;
    DiskManagerClient(const DiskManagerClient &) = delete;
    DiskManagerClient &operator=(const DiskManagerClient &) = delete;

    int32_t Connect(sptr<IDiskManager> &proxy);

    sptr<IDiskManager> diskManager_;
    sptr<IRemoteObject::DeathRecipient> deathRecipient_;
    std::mutex mutex_;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_FILEMANAGEMENT_DISK_MANAGER_CLIENT_H
```

Note: `NoCopyable` base already provides `DISALLOW_COPY_AND_ASSIGN`, so the explicit `= delete` lines are redundant but kept for clarity and to match `FileCacheAdapter` pattern. The private constructor `= default` is needed because Meyers' singleton requires it to be private (not public).

---

### Task 2: Modify disk_manager_client.cpp — Add GetInstance implementation and update DmDeathRecipient

**Files:**
- Modify: `/home/luna/filemanagement_disk_manager/interfaces/innerkits/src/disk_manager_client.cpp`

- [ ] **Step 1: Add GetInstance() implementation**

Add after `namespace DiskManager {` block (after line 31), before the anonymous namespace:

```cpp
DiskManagerClient &DiskManagerClient::GetInstance()
{
    static DiskManagerClient instance_;
    return instance_;
}
```

- [ ] **Step 2: Fix DmDeathRecipient::OnRemoteDied**

Change line 44 from:
```cpp
DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
client.ResetProxy();
```
to:
```cpp
DiskManagerClient::GetInstance().ResetProxy();
```

- [ ] **Step 3: Remove DelayedSingleton constructor/destructor definitions**

Delete the existing constructor/destructor definitions (lines 129-134):
```cpp
DiskManagerClient::DiskManagerClient() {}

DiskManagerClient::~DiskManagerClient()
{
    ResetProxy();
}
```

These are replaced by `= default` in the header and the `GetInstance()` static local variable. The destructor must remain as `virtual ~DiskManagerClient()` in the header (with `= default` implicitly). However, since the destructor currently calls `ResetProxy()`, we need to keep that behavior. Change the header destructor to:

```cpp
virtual ~DiskManagerClient();
```

And keep the destructor definition in cpp:
```cpp
DiskManagerClient::~DiskManagerClient()
{
    ResetProxy();
}
```

Wait — actually the destructor body should remain in .cpp because it has logic (`ResetProxy()`). So the header declares `virtual ~DiskManagerClient();` (without `= default`), and the .cpp keeps the existing destructor definition. This is correct.

---

### Task 3: Update volumemanager_n_exporter.cpp callers

**Files:**
- Modify: `/home/luna/filemanagement_disk_manager/interfaces/kits/js/volume_manager/src/volumemanager_n_exporter.cpp`

- [ ] **Step 1: Replace DelayedSingleton calls**

5 occurrences need updating. Change all instances of:
```cpp
DelayedSingleton<DiskManagerClient>::GetInstance()->Method(args)
```
to:
```cpp
DiskManagerClient::GetInstance().Method(args)
```

Specific lines:
- Line 683: `DelayedSingleton<DiskManagerClient>::GetInstance()->GetDiskById(diskIdStr, *disk);` → `DiskManagerClient::GetInstance().GetDiskById(diskIdStr, *disk);`
- Line 956: `DelayedSingleton<DiskManagerClient>::GetInstance()->GetPartitionTable(diskIdStr, *tableInfo);` → `DiskManagerClient::GetInstance().GetPartitionTable(diskIdStr, *tableInfo);`
- Line 1061: `DelayedSingleton<DiskManagerClient>::GetInstance()->CreatePartition(diskIdStr, params);` → `DiskManagerClient::GetInstance().CreatePartition(diskIdStr, params);`
- Line 1098: `DelayedSingleton<DiskManagerClient>::GetInstance()->DeletePartition(diskIdStr, partitionNum);` → `DiskManagerClient::GetInstance().DeletePartition(diskIdStr, partitionNum);`
- Line 1185: `DelayedSingleton<DiskManagerClient>::GetInstance()->FormatPartition(diskId, partitionNum, params);` → `DiskManagerClient::GetInstance().FormatPartition(diskId, partitionNum, params);`

Also remove `#include <singleton.h>` if present in this file.

---

### Task 4: Update disk_manager_client_test.cpp callers

**Files:**
- Modify: `/home/luna/filemanagement_disk_manager/test/unittest/disk_manager_client_test.cpp`

- [ ] **Step 1: Replace all DelayedSingleton calls**

30+ occurrences of:
```cpp
DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
```
change to:
```cpp
DiskManagerClient &client = DiskManagerClient::GetInstance();
```

Also remove `#include <singleton.h>` if present.

---

### Task 5: Update storage_manager_provider.cpp callers in storage_service

**Files:**
- Modify: `/home/luna/foundation/filemanagement/storage_service/services/storage_manager/ipc/src/storage_manager_provider.cpp`

- [ ] **Step 1: Replace all DelayedSingleton calls**

15 occurrences of:
```cpp
DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->Method(args)
```
change to:
```cpp
OHOS::DiskManager::DiskManagerClient::GetInstance().Method(args)
```

Also remove `#include <singleton.h>` if present.

---

### Task 6: Update mtp_device_manager.cpp callers in storage_service

**Files:**
- Modify: `/home/luna/foundation/filemanagement/storage_service/services/storage_daemon/mtp/src/mtp_device_manager.cpp`

- [ ] **Step 1: Replace DelayedSingleton calls**

4 occurrences of:
```cpp
DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->Method(args)
```
change to:
```cpp
OHOS::DiskManager::DiskManagerClient::GetInstance().Method(args)
```

Also remove `#include <singleton.h>` if present.

---

### Task 7: Update netlink_handler.cpp caller in storage_service

**Files:**
- Modify: `/home/luna/foundation/filemanagement/storage_service/services/storage_daemon/netlink/src/netlink_handler.cpp`

- [ ] **Step 1: Replace DelayedSingleton call**

1 occurrence on line 76:
```cpp
DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance()->OnBlockDiskUevent(convertedMsg);
```
change to:
```cpp
OHOS::DiskManager::DiskManagerClient::GetInstance().OnBlockDiskUevent(convertedMsg);
```

Also remove `#include <singleton.h>` if present.

---

### Task 8: Remove singleton.h include from all modified files

**Files:**
- All files modified in Tasks 1-7

- [ ] **Step 1: Verify and remove `#include <singleton.h>`**

Check each modified file for `#include <singleton.h>` and remove it. This includes:
- `disk_manager_client.h` (already removed in Task 1)
- `disk_manager_client.cpp` — check and remove if present
- `volumemanager_n_exporter.cpp` — check and remove if present
- `disk_manager_client_test.cpp` — check and remove if present
- `storage_manager_provider.cpp` — check and remove if present
- `mtp_device_manager.cpp` — check and remove if present
- `netlink_handler.cpp` — check and remove if present

---

### Task 9: Compile verification — disk_manager repo

**Files:** None (verification only)

- [ ] **Step 1: Run prebuilts setup**
```bash
bash build/prebuilts_config.sh --download-sdk
```

- [ ] **Step 2: Build disk_manager component**
```bash
hb build disk_manager -i
```

- [ ] **Step 3: Build disk_manager tests**
```bash
hb build disk_manager -t
```

---

### Task 10: Compile verification — storage_service repo

**Files:** None (verification only)

- [ ] **Step 1: Run prebuilts setup** (if not already done)
```bash
bash build/prebuilts_config.sh --download-sdk
```

- [ ] **Step 2: Build storage_service component**
```bash
hb build storage_manager -i
```

- [ ] **Step 3: Build storage_service tests**
```bash
hb build storage_manager -t
```

---

### Task 11: Commit changes in disk_manager repo

- [ ] **Step 1: Commit with DCO-compliant message**
```bash
cd /home/luna/filemanagement_disk_manager
git add interfaces/innerkits/include/disk_manager_client.h interfaces/innerkits/src/disk_manager_client.cpp interfaces/kits/js/volume_manager/src/volumemanager_n_exporter.cpp test/unittest/disk_manager_client_test.cpp
git commit -s -m "refactor: replace DECLARE_DELAYED_SINGLETON with Meyers' singleton in DiskManagerClient" -m "Replace DECLARE_DELAYED_SINGLETON pattern with static local variable singleton (Meyers' singleton) for improved thread safety. Update all callers from DelayedSingleton<T>::GetInstance() to T::GetInstance()." -m "Co-Authored-By: Agent"
```

---

### Task 12: Commit changes in storage_service repo

- [ ] **Step 1: Commit with DCO-compliant message**
```bash
cd /home/luna/foundation/filemanagement/storage_service
git add services/storage_manager/ipc/src/storage_manager_provider.cpp services/storage_daemon/mtp/src/mtp_device_manager.cpp services/storage_daemon/netlink/src/netlink_handler.cpp
git commit -s -m "refactor: update DiskManagerClient callers to use new GetInstance() interface" -m "Change all DiskManagerClient singleton calls from DelayedSingleton<T>::GetInstance()->Method() to DiskManagerClient::GetInstance().Method() following the Meyers' singleton refactor in disk_manager repo." -m "Co-Authored-By: Agent"
```
