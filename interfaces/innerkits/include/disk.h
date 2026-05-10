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

#include <string>
#include <vector>

namespace OHOS {
namespace DiskManager {
enum {
    SD_FLAG = 1,
    USB_FLAG = 2,
    CD_FLAG = 3,
};
class Disk : public Parcelable {
public:
    Disk();
    Disk(const std::string &diskId, int64_t sizeBytes, const std::string &sysPath,
         const std::string &vendor, int32_t flag);

    std::string GetDiskId() const;
    int64_t GetSizeBytes() const;
    std::string GetSysPath() const;
    std::string GetVendor() const;
    int32_t GetFlag() const;
    void SetFlag(int32_t flag);
    void SetVolumeIds(const std::vector<std::string> &volumeIds);
    void SetVolumeIds(std::vector<std::string> &&volumeIds);
    const std::vector<std::string> &GetVolumeIds() const;
    void SetExtInfo(const std::string &extInfo);
    const std::string &GetExtInfo() const;

    /** volumeManager.Disk.removable：是否与 SD/USB/光驱等可移动盘 flag 一致（由 flag_ 推导）。 */
    bool IsRemovable() const;
    /** volumeManager.Disk.type：据 sysPath_ 子串优先级（CD>NVMe(SSD)>SATA(HDD)>USB>SD）再结合 flag_ 回退。 */
    std::string GetUiType() const;

    bool Marshalling(Parcel &parcel) const override;
    static Disk *Unmarshalling(Parcel &parcel);

private:
    /*
     * 与 ArkTS volumeManager.Disk 各属性的对应（实现侧见 volumemanager_n_exporter BuildDiskJSObject）：
     *   Disk.id          -> diskId_            / GetDiskId()
     *   Disk.description -> vendor_           / GetVendor()
     *   Disk.type       -> （无单独成员）       / GetUiType()，见 disk.cpp：sysPath 规则 + flag_ 回退，含 SSD|HDD|SD|USB|ODD|Unknown
     *   Disk.capacity    -> sizeBytes_        / GetSizeBytes()
     *   Disk.removable   -> （无单独成员）       / IsRemovable()，由 flag_（SD/USB/CD）推导
     *   Disk.deviceNode  -> sysPath_          / GetSysPath()（通常为 sysfs 路径 /sys+DEVPATH，非挂载目录）
     *   Disk.volumeIds   -> volumeIds_        / GetVolumeIds()
     *   Disk.extInfo     -> extInfo_          / GetExtInfo()
     * IPC Parcel 序列化字段仅含磁盘侧持久数据；flag_ 同时为 GetUiType/IsRemovable 的推导依据。
     */
    std::string diskId_; ///< JS Disk.id
    int64_t sizeBytes_ {}; ///< JS Disk.capacity
    std::string sysPath_; ///< JS Disk.deviceNode（内容实为 sysfs 设备路径语义）
    std::string vendor_; ///< JS Disk.description
    int32_t flag_ {}; ///< 与 disk_config/uevent MatchConfig 等一致；推导 JS type、removable
    std::vector<std::string> volumeIds_; ///< JS Disk.volumeIds
    /** JS Disk.extInfo；由 storage_daemon ADDON_GET_BLOCK_INFO_BY_TYPE(213) GetBlockInfoByType 填入（见磁盘创建路径）。 */
    std::string extInfo_;
};
} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_DISK_H