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

#include "disk/disk_manager.h"

#include "storage_daemon_adapter.h"

#include "disk_manager_errno.h"

namespace OHOS {
namespace DiskManager {

DiskDataManager::DiskDataManager() = default;

DiskDataManager::~DiskDataManager() = default;

DiskDataManager &DiskDataManager::GetInstance()
{
    static DiskDataManager instance;
    return instance;
}

int32_t DiskDataManager::Mount(const std::string &volumeId)
{
    (void)volumeId;
    return DiskManagerErrNo::DISK_MGR_ERR;
}

int32_t DiskDataManager::Unmount(const std::string &volumeId)
{
    (void)volumeId;
    return DiskManagerErrNo::DISK_MGR_ERR;
}

int32_t DiskDataManager::Format(const std::string &volumeId, const std::string &fsType)
{
    (void)volumeId;
    (void)fsType;
    return DiskManagerErrNo::DISK_MGR_ERR;
}

int32_t DiskDataManager::TryToFix(const std::string &volumeId)
{
    (void)volumeId;
    return DiskManagerErrNo::DISK_MGR_ERR;
}

int32_t DiskDataManager::SetVolumeDescription(const std::string &fsUuid, const std::string &description)
{
    (void)fsUuid;
    (void)description;
    return DiskManagerErrNo::DISK_MGR_ERR;
}

int32_t DiskDataManager::Partition(const std::string &diskId, int32_t type)
{
    return StorageDaemonAdapter::GetInstance().Partition(diskId, type, 0);
}

int32_t DiskDataManager::OnDiskCreated(const Disk &disk)
{
    (void)disk;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::UpsertDiskSnapshot(const DiskStoreRecord &rec)
{
    (void)rec;
    return DiskManagerErrNo::E_OK;
}

bool DiskDataManager::HasDisk(const std::string &diskId) const
{
    (void)diskId;
    return false;
}

int32_t DiskDataManager::OnDiskDestroyed(const std::string &diskId)
{
    (void)diskId;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::OnVolumeCreated(const VolumeCore &vc)
{
    (void)vc;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::OnVolumeMounted(const VolumeInfoStr &vis)
{
    (void)vis;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::OnVolumeStateChanged(const std::string &volumeId, uint32_t state)
{
    (void)volumeId;
    (void)state;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::OnVolumeDamaged(const VolumeInfoStr &vis)
{
    (void)vis;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::ReplacePartitionsForDisk(const std::string &diskId,
                                                  const std::vector<PartitionRecord> &partitions)
{
    (void)diskId;
    (void)partitions;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::GetAllDisks(std::vector<Disk> &out) const
{
    out.clear();
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::GetDiskById(const std::string &diskId, Disk &out) const
{
    (void)diskId;
    (void)out;
    return DiskManagerErrNo::E_DISK_NOT_FOUND;
}

int32_t DiskDataManager::GetAllVolumes(std::vector<VolumeExternal> &out) const
{
    out.clear();
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::GetVolumeById(const std::string &volumeId, VolumeExternal &out) const
{
    (void)volumeId;
    (void)out;
    return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
}

int32_t DiskDataManager::GetVolumeByUuid(const std::string &fsUuid, VolumeExternal &out) const
{
    (void)fsUuid;
    (void)out;
    return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
}

int32_t DiskDataManager::UpdateVolumeMetadata(const std::string &volumeId,
                                              const std::string &fsUuid,
                                              const std::string &fsTypeStr,
                                              const std::string &description)
{
    (void)volumeId;
    (void)fsUuid;
    (void)fsTypeStr;
    (void)description;
    return DiskManagerErrNo::E_OK;
}

} // namespace DiskManager
} // namespace OHOS
