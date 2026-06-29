/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_DISK_MANAGER_DISK_H
#define OHOS_DISK_MANAGER_DISK_H

#include "parcel.h"

#include <cstdint>
#include <string>
#include <vector>

namespace OHOS {
namespace DiskManager {
/** 与 uevent/disk_config、@ohos.file.volumeManager DiskType（new_api）一致。 */
enum DiskType : int32_t {
    SD_FLAG = 1,
    USB_FLAG = 2,
    CD_FLAG = 3,
    DATA_DISK_SSD = 4,
    DATA_DISK_HDD = 5,
    DVR_USB = 6,
    DISK_TYPE_UNKNOWN = 255,
};

class Disk : public Parcelable {
public:
    Disk();
    /** blockDevPath 为 /dev/block/{diskId}，不导出 JS。 */
    Disk(const std::string &diskId, int64_t sizeBytes, const std::string &devName, int32_t diskType);

    // --- volumeManager.Disk（new_api）---
    std::string GetDiskId() const;
    int64_t GetSizeBytes() const;
    void SetSizeBytes(int64_t sizeBytes);
    int32_t GetDiskType() const;
    void SetDiskType(int32_t diskType);
    bool GetRemovable() const;
    bool IsRemovable() const;
    void SetVolumeIds(const std::vector<std::string> &volumeIds);
    void SetVolumeIds(std::vector<std::string> &&volumeIds);
    const std::vector<std::string> &GetVolumeIds() const;
    void SetExtraInfo(const std::string &extraInfo);
    const std::string &GetExtraInfo() const;
    void SetVendor(const std::string &vendor);
    std::string GetVendor() const;

    // --- 进程内 ---
    /** 块设备节点路径，如 /dev/block/sda。 */
    std::string GetSysPath() const;
    std::string GetDevName() const;
    bool IsInternalDataDisk() const;
    /** 据 uevent 的 /sys{DEVPATH} 刷新 diskType，不持久化 sysfs 路径。 */
    void RefreshClassificationFromSysfs(const std::string &sysfsPath);

    bool Marshalling(Parcel &parcel) const override;
    static Disk *Unmarshalling(Parcel &parcel);

private:
    void UpdateRemovableFromDiskType();

    // volumeManager.Disk（new_api，与 JS 字段顺序一致）
    std::string diskId_;                         // diskId
    int64_t sizeBytes_ {};                       // sizeBytes
    int32_t diskType_ {DISK_TYPE_UNKNOWN};       // diskType
    bool removable_ {true};                      // removable，默认 true；仅 HDD/SSD 为 false
    std::vector<std::string> volumeIds_;          // volumeIds
    std::string extraInfo_;                        // extraInfo

    // 进程内扩展（CommonEvent），不导出 JS
    std::string sysPath_;    // /dev/block/{diskId}
    std::string devName_;
    std::string vendor_;
};
} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_DISK_H
