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

#include "disk.h"

namespace OHOS {
namespace DiskManager {
namespace {
/** Parcel 反序列化时 volumeIds 条数上限，防止异常大包拖垮进程。 */
constexpr uint32_t DISK_VOLUME_IDS_PARCEL_MAX = 256U;

/** 光驱：sysfs 多为 /block/srx。 */
bool SysPathHasOpticalBlock(const std::string &p)
{
    return p.find("/block/sr") != std::string::npos;
}

/** NVMe：约定为 SSD（不读 rotational）。 */
bool SysPathHasNvme(const std::string &p)
{
    return p.find("/nvme") != std::string::npos;
}

/** 内置 SATA 链路：AHCI ataN 或 platform *.sata（不含 USB 转接场景则见 USB 规则）。 */
bool SysPathHasDirectSata(const std::string &p)
{
    return p.find("/ata/") != std::string::npos || p.find(".sata/") != std::string::npos;
}

/**
 * USB 存储拓扑：通用 /usb…、hiusb，及与 etc/disk_config 相近的 host 控制器关键字。
 * 须在 SATA 直判之后，避免误判；且应在 MMC 之前，以便 U 盘读卡器走 USB 而非 SD。
 */
bool SysPathHasUsbTopology(const std::string &p)
{
    if (p.find("/usb") != std::string::npos || p.find("usbhost") != std::string::npos) {
        return true;
    }
    if (p.find("/hiusb/") != std::string::npos || p.find("platform/hiusb") != std::string::npos) {
        return true;
    }
    if (p.find(".ehci") != std::string::npos || p.find("/ehci") != std::string::npos ||
        p.find("ehci/") != std::string::npos) {
        return true;
    }
    if (p.find("xhci") != std::string::npos || p.find(".dwc3") != std::string::npos ||
        p.find("/dwc3/") != std::string::npos) {
        return true;
    }
    return false;
}

/** MMC/SD 主机：与 disk_config 中 dwmmc、hi_mci、mmc_host 同源形态。 */
bool SysPathHasMmcHost(const std::string &p)
{
    return p.find("mmc_host") != std::string::npos || p.find("dwmmc") != std::string::npos ||
        p.find("hi_mci") != std::string::npos || p.find("dw_mmc") != std::string::npos;
}
} // namespace

Disk::Disk() {}
Disk::Disk(const std::string &diskId,
           int64_t sizeBytes,
           const std::string &sysPath,
           const std::string &vendor,
           int32_t flag)
    : diskId_(diskId), sizeBytes_(sizeBytes), sysPath_(sysPath), vendor_(vendor), flag_(flag)
{
}

std::string Disk::GetDiskId() const
{
    return diskId_;
}

int64_t Disk::GetSizeBytes() const
{
    return sizeBytes_;
}

std::string Disk::GetSysPath() const
{
    return sysPath_;
}

std::string Disk::GetVendor() const
{
    return vendor_;
}

int32_t Disk::GetFlag() const
{
    return flag_;
}

void Disk::SetFlag(int32_t flag)
{
    flag_ = flag;
}

void Disk::SetVolumeIds(const std::vector<std::string> &volumeIds)
{
    volumeIds_ = volumeIds;
}

void Disk::SetVolumeIds(std::vector<std::string> &&volumeIds)
{
    volumeIds_ = std::move(volumeIds);
}

const std::vector<std::string> &Disk::GetVolumeIds() const
{
    return volumeIds_;
}

void Disk::SetExtInfo(const std::string &extInfo)
{
    extInfo_ = extInfo;
}

const std::string &Disk::GetExtInfo() const
{
    return extInfo_;
}

bool Disk::IsRemovable() const
{
    return flag_ == SD_FLAG || flag_ == USB_FLAG || flag_ == CD_FLAG;
}

std::string Disk::GetUiType() const
{
    const std::string &p = sysPath_;

    /* 1. CD / sr* */
    if (flag_ == CD_FLAG || SysPathHasOpticalBlock(p)) {
        return "ODD";
    }
    /* 2. NVMe -> SSD */
    if (SysPathHasNvme(p)) {
        return "SSD";
    }
    /* 3. 直出 SATA -> HDD（含 SATA 固态盘，本阶段不按 rotational 细分） */
    if (SysPathHasDirectSata(p)) {
        return "HDD";
    }
    /* 4. USB 拓扑 */
    if (SysPathHasUsbTopology(p)) {
        return "USB";
    }
    /* 5. SD / MMC 卡类 */
    if (SysPathHasMmcHost(p)) {
        return "SD";
    }
    /* 6. 无可靠路径时沿用 uevent/MatchConfig flag */
    switch (flag_) {
        case SD_FLAG:
            return "SD";
        case USB_FLAG:
            return "USB";
        case CD_FLAG:
            return "ODD";
        default:
            return "Unknown";
    }
}
bool Disk::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteString(diskId_)) {
        return false;
    }

    if (!parcel.WriteInt64(sizeBytes_)) {
        return false;
    }

    if (!parcel.WriteString(sysPath_)) {
        return false;
    }

    if (!parcel.WriteString(vendor_)) {
        return false;
    }

    if (!parcel.WriteInt32(flag_)) {
        return false;
    }

    uint32_t nVolumes = volumeIds_.size();
    if (!parcel.WriteUint32(nVolumes)) {
        return false;
    }
    for (const auto &vid : volumeIds_) {
        if (!parcel.WriteString(vid)) {
            return false;
        }
    }
    if (!parcel.WriteString(extInfo_)) {
        return false;
    }

    return true;
}

Disk *Disk::Unmarshalling(Parcel &parcel)
{
    Disk *obj = new (std::nothrow) Disk();
    if (!obj) {
        return nullptr;
    }
    obj->diskId_ = parcel.ReadString();
    obj->sizeBytes_ = parcel.ReadInt64();
    obj->sysPath_ = parcel.ReadString();
    obj->vendor_ = parcel.ReadString();
    obj->flag_ = parcel.ReadInt32();
    uint32_t nVolumes = parcel.ReadUint32();
    if (nVolumes > DISK_VOLUME_IDS_PARCEL_MAX) {
        delete obj;
        return nullptr;
    }
    obj->volumeIds_.clear();
    obj->volumeIds_.reserve(nVolumes);
    for (uint32_t i = 0; i < nVolumes; i++) {
        obj->volumeIds_.push_back(parcel.ReadString());
    }
    obj->extInfo_ = parcel.ReadString();
    return obj;
}
} // namespace DiskManager
} // namespace OHOS
