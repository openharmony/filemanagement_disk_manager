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
constexpr uint32_t DISK_VOLUME_IDS_PARCEL_MAX = 256U;

bool IsInternalDataDiskType(int32_t diskType)
{
    return diskType == DATA_DISK_SSD || diskType == DATA_DISK_HDD;
}

bool SysPathHasOpticalBlock(const std::string &p)
{
    return p.find("/block/sr") != std::string::npos;
}

bool SysPathHasNvme(const std::string &p)
{
    return p.find("/nvme") != std::string::npos;
}

bool SysPathHasDirectSata(const std::string &p)
{
    return p.find("/ata/") != std::string::npos || p.find(".sata/") != std::string::npos;
}

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

bool SysPathHasMmcHost(const std::string &p)
{
    return p.find("mmc_host") != std::string::npos || p.find("dwmmc") != std::string::npos ||
        p.find("hi_mci") != std::string::npos || p.find("dw_mmc") != std::string::npos;
}
} // namespace

Disk::Disk()
{
    UpdateRemovableFromDiskType();
}

Disk::Disk(const std::string &diskId,
           int64_t sizeBytes,
           const std::string &blockDevPath,
           int32_t diskType)
    : diskId_(diskId), sizeBytes_(sizeBytes), diskType_(diskType), sysPath_(blockDevPath)
{
    UpdateRemovableFromDiskType();
}

std::string Disk::GetDiskId() const
{
    return diskId_;
}

int64_t Disk::GetSizeBytes() const
{
    return sizeBytes_;
}

void Disk::SetSizeBytes(int64_t sizeBytes)
{
    sizeBytes_ = sizeBytes;
}

int32_t Disk::GetDiskType() const
{
    return diskType_;
}

void Disk::SetDiskType(int32_t diskType)
{
    diskType_ = diskType;
    UpdateRemovableFromDiskType();
}

bool Disk::GetRemovable() const
{
    return removable_;
}

bool Disk::IsRemovable() const
{
    return GetRemovable();
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

void Disk::SetExtraInfo(const std::string &extraInfo)
{
    extraInfo_ = extraInfo;
}

const std::string &Disk::GetExtraInfo() const
{
    return extraInfo_;
}

std::string Disk::GetSysPath() const
{
    return sysPath_;
}

bool Disk::IsInternalDataDisk() const
{
    return IsInternalDataDiskType(diskType_);
}

void Disk::UpdateRemovableFromDiskType()
{
    removable_ = IsInternalDataDiskType(diskType_) ? false : true;
}

void Disk::RefreshClassificationFromSysfs(const std::string &sysfsPath)
{
    if (diskType_ == DVR_USB) {
        UpdateRemovableFromDiskType();
        return;
    }

    const std::string &p = sysfsPath;
    int32_t classifiedType = DISK_TYPE_UNKNOWN;

    if (!p.empty()) {
        if (diskType_ == CD_FLAG || SysPathHasOpticalBlock(p)) {
            classifiedType = CD_FLAG;
        } else if (SysPathHasNvme(p)) {
            classifiedType = DATA_DISK_SSD;
        } else if (SysPathHasDirectSata(p)) {
            classifiedType = DATA_DISK_HDD;
        } else if (SysPathHasUsbTopology(p)) {
            classifiedType = USB_FLAG;
        } else if (SysPathHasMmcHost(p)) {
            classifiedType = SD_FLAG;
        }
    }
    if (classifiedType == DISK_TYPE_UNKNOWN) {
        switch (diskType_) {
            case SD_FLAG:
            case USB_FLAG:
            case CD_FLAG:
                classifiedType = diskType_;
                break;
            default:
                classifiedType = DISK_TYPE_UNKNOWN;
                break;
        }
    }

    diskType_ = classifiedType;
    UpdateRemovableFromDiskType();
}

bool Disk::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteString(diskId_)) {
        return false;
    }
    if (!parcel.WriteInt64(sizeBytes_)) {
        return false;
    }
    if (!parcel.WriteInt32(diskType_)) {
        return false;
    }
    if (!parcel.WriteBool(removable_)) {
        return false;
    }
    const uint32_t nVolumes = static_cast<uint32_t>(volumeIds_.size());
    if (!parcel.WriteUint32(nVolumes)) {
        return false;
    }
    for (const auto &vid : volumeIds_) {
        if (!parcel.WriteString(vid)) {
            return false;
        }
    }
    if (!parcel.WriteString(extraInfo_)) {
        return false;
    }
    return true;
}

Disk *Disk::Unmarshalling(Parcel &parcel)
{
    Disk *obj = new (std::nothrow) Disk();
    if (obj == nullptr) {
        return nullptr;
    }
    obj->diskId_ = parcel.ReadString();
    obj->sizeBytes_ = parcel.ReadInt64();
    obj->diskType_ = parcel.ReadInt32();
    obj->removable_ = parcel.ReadBool();
    const uint32_t nVolumes = parcel.ReadUint32();
    if (nVolumes > DISK_VOLUME_IDS_PARCEL_MAX) {
        delete obj;
        return nullptr;
    }
    obj->volumeIds_.clear();
    obj->volumeIds_.reserve(nVolumes);
    for (uint32_t i = 0; i < nVolumes; ++i) {
        obj->volumeIds_.push_back(parcel.ReadString());
    }
    obj->extraInfo_ = parcel.ReadString();
    return obj;
}
} // namespace DiskManager
} // namespace OHOS
