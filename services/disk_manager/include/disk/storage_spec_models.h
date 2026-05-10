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

#ifndef OHOS_DISK_MANAGER_DISK_STORAGE_SPEC_MODELS_H
#define OHOS_DISK_MANAGER_DISK_STORAGE_SPEC_MODELS_H

#include <cstdint>
#include <string>
#include <vector>

namespace OHOS {
namespace DiskManager {

/** 与 define.md 5.2.3 MediaType 对齐；块设备及存储模型共用。 */
enum class DiskStoreMediaType : uint8_t {
    SSD = 0,
    HDD = 1,
    UNKNOWN = 2,
    USB = 3,
};

/** 与 define.md 5.2.4 DiskType 对齐 */
enum class DiskStoreDiskType : uint8_t {
    SD_CARD = 0,
    CD_DVD_BD = 1,
    USB_FLASH = 2,
    MTP_PTP = 3,
    UNKNOWN = 255,
};

/** 与 define.md 5.2.5 FsType 对齐 */
enum class DiskStoreFsType : uint8_t {
    HMFS = 0,
    F2FS = 1,
    NTFS = 2,
    EXFAT = 3,
    EXT4 = 4,
    FAT32 = 5,
    UDF = 6,
    ISO9660 = 7,
    UNKNOWN = 255,
};

/** 与 define.md 5.2.10 VolumeState 对齐 */
enum class DiskStoreVolumeState : uint32_t {
    UNFORMATTED = 0,
    ERROR = 1,
    CHECKING = 2,
    MOUNTED = 3,
    UNMOUNTING = 4,
    MOUNTING = 5,
    REPAIRING = 6,
    FORMATTING = 7,
    UNMOUNTED = 8,
    EJECTING = 9,
    REMOVED = 10,
    BAD_REMOVAL = 11,
    DAMAGED = 12,
    FUSE_REMOVED = 13,
    DAMAGED_MOUNTED = 14,
};

struct PartitionRecord {
    std::string diskId;
    uint32_t partitionNumber = 0;
    uint64_t startSector = 0;
    uint64_t endSector = 0;
    uint64_t sizeBytes = 0;
    std::string partitionType;
    std::string fsTypeRaw;
};

/**
 * 块设备详细信息（进程内/解析用；经 storage_daemon GetBlockInfoByType 时一般以字符串载荷由对端约定格式返回）。
 */
struct BlockInfo {
    uint64_t sizeBytes {};
    std::string vendor;
    std::string model;
    std::string interfaceType;
    uint32_t rpm {};
    std::string state;
    DiskStoreMediaType mediaType {DiskStoreMediaType::UNKNOWN};
    bool removable {};
    std::string serialNumber;
    std::string pciePath;
    std::string location;
    std::string diskId;
    uint64_t usedBytes {};
    uint64_t availableBytes {};
    std::string devicePath;
    std::string port;
};

/**
 * 与 define.md DiskInfo 对齐的进程内权威快照（字段逐步填满，未解析项保持默认）。
 */
struct DiskStoreRecord {
    std::string diskId;
    std::string diskName;
    int64_t sizeBytes = 0;
    int64_t usedBytes = 0;
    int64_t availableBytes = 0;
    DiskStoreMediaType mediaType = DiskStoreMediaType::UNKNOWN;
    DiskStoreDiskType diskType = DiskStoreDiskType::UNKNOWN;
    std::vector<std::string> volumeIds;
    bool removable = false;
    std::string sysPath;
};

/**
 * 与 define.md Volume 对齐的进程内权威快照。
 */
struct VolumeStoreRecord {
    std::string id;
    std::string uuid;
    std::string diskId;
    std::string description;
    bool removable = false;
    DiskStoreVolumeState state = DiskStoreVolumeState::UNMOUNTED;
    std::string path;
    DiskStoreFsType fsType = DiskStoreFsType::UNKNOWN;
    std::string extraInfo;
    uint32_t partitionNumber = 0;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_DISK_STORAGE_SPEC_MODELS_H
