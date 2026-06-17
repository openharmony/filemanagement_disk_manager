/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ohos.file.volumeManager.impl.h"
#include "disk_manager_client.h"
#include "disk_manager_hilog.h"
#include "disk_manager_napi_errno.h"
#include "partition_types.h"
#include "storageStatistics_taihe_error.h"

#include <sstream>

namespace {
ohos::file::volumeManager::DiskType ToTaiheDiskType(int32_t diskType)
{
    return ohos::file::volumeManager::DiskType::from_value(diskType);
}

ohos::file::volumeManager::Disk MakeEmptyTaiheDisk()
{
    return {"", 0, ToTaiheDiskType(OHOS::DiskManager::DISK_TYPE_UNKNOWN), false,
            taihe::array<taihe::string>::make(0, taihe::string{}), ""};
}
} // namespace

namespace ANI::VolumeManager {

ohos::file::volumeManager::Volume GetVolumeByUuidSync(taihe::string_view uuid)
{
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return {"", "", "", "", true, 0, "", "", "", 0};
    }
    auto volumeInfo = std::make_shared<OHOS::DiskManager::VolumeExternal>();
    int32_t errNum = instance->GetVolumeByUuid(uuid.c_str(), *volumeInfo);
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return {"", "", "", "", true, 0, "", "", "", 0};
    }
    return {volumeInfo->GetId(),
            volumeInfo->GetUuid(),
            volumeInfo->GetDiskId(),
            volumeInfo->GetDescription(),
            true,
            volumeInfo->GetState(),
            volumeInfo->GetPath(),
            volumeInfo->GetFsTypeString(),
            volumeInfo->GetExtraInfo(),
            volumeInfo->GetPartitionNum()};
}

taihe::array<ohos::file::volumeManager::Volume> GetAllVolumesSync()
{
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return taihe::array<ohos::file::volumeManager::Volume>::make(0, ohos::file::volumeManager::Volume{});
    }
    auto volumeInfo = std::make_shared<std::vector<OHOS::DiskManager::VolumeExternal>>();
    int32_t errNum = instance->GetAllVolumes(*volumeInfo);
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return taihe::array<ohos::file::volumeManager::Volume>::make(0, ohos::file::volumeManager::Volume{});
    }
    auto result =
        taihe::array<ohos::file::volumeManager::Volume>::make(volumeInfo->size(), ohos::file::volumeManager::Volume{});
    auto volumeTransformer = [](auto &vol) -> ohos::file::volumeManager::Volume {
        return {vol.GetId(),    vol.GetUuid(), vol.GetDiskId(),       vol.GetDescription(), true,
                vol.GetState(), vol.GetPath(), vol.GetFsTypeString(), vol.GetExtraInfo(),   vol.GetPartitionNum()};
    };
    std::transform(volumeInfo->begin(), volumeInfo->end(), result.begin(), volumeTransformer);
    return taihe::array<ohos::file::volumeManager::Volume>(taihe::copy_data_t{}, result.data(), result.size());
}

void FormatSync(::taihe::string_view volumeId, ::taihe::string_view fsType)
{
    std::string volumeIdString = std::string(volumeId);
    std::string fsTypeString = std::string(fsType);
    if (volumeIdString.empty() || fsTypeString.empty()) {
        LOGE("Invalid parameter, volumeId or fsType is empty");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_PARAMS);
        return;
    }
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return;
    }
    int32_t result = instance->Format(volumeIdString, fsTypeString);
    if (result != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(result);
        return;
    }
}

ohos::file::volumeManager::Volume GetVolumeByIdSync(::taihe::string_view volumeId)
{
    std::string volumeIdString = std::string(volumeId);
    if (volumeIdString.empty()) {
        LOGE("Invalid volumeId parameter, volumeId is empty");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_PARAMS);
        return {"", "", "", "", true, 0, "", "", "", 0};
    }
    auto volumeInfo = std::make_shared<OHOS::DiskManager::VolumeExternal>();
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return {"", "", "", "", true, 0, "", "", "", 0};
    }
    int32_t errNum = instance->GetVolumeById(volumeIdString, *volumeInfo);
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return {"", "", "", "", true, 0, "", "", "", 0};
    }
    return {volumeInfo->GetId(),
            volumeInfo->GetUuid(),
            volumeInfo->GetDiskId(),
            volumeInfo->GetDescription(),
            true,
            volumeInfo->GetState(),
            volumeInfo->GetPath(),
            volumeInfo->GetFsTypeString(),
            volumeInfo->GetExtraInfo(),
            volumeInfo->GetPartitionNum()};
}

void MountSync(::taihe::string_view volumeId)
{
    std::string volumeIdString = std::string(volumeId);
    if (volumeIdString.empty()) {
        LOGE("Invalid volumeId parameter, volumeId is empty");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_PARAMS);
        return;
    }
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return;
    }

    int32_t errNum = instance->Mount(volumeIdString);
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return;
    }
}

void UnmountSync(::taihe::string_view volumeId)
{
    std::string volumeIdString = std::string(volumeId);
    if (volumeIdString.empty()) {
        LOGE("Invalid volumeId parameter, volumeId is empty");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_PARAMS);
        return;
    }
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return;
    }
    int32_t errNum = instance->Unmount(volumeIdString);
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return;
    }
}

void PartitionSync(::taihe::string_view diskId, int32_t type)
{
    std::string diskIdString = std::string(diskId);
    if (diskIdString.empty()) {
        LOGE("Invalid parameter, diskId is empty");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_PARAMS);
        return;
    }
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return;
    }
    int32_t errNum = instance->Partition(diskIdString, type);
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return;
    }
}

void SetVolumeDescriptionSync(::taihe::string_view uuid, ::taihe::string_view description)
{
    std::string uuidString = std::string(uuid);
    std::string descStr = std::string(description);
    if (uuidString.empty() || descStr.empty()) {
        LOGE("Invalid parameter, uuid or description is empty");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_PARAMS);
        return;
    }
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return;
    }
    int32_t errNum = instance->SetVolumeDescription(uuidString, descStr);
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return;
    }
}

// ---------- Disk management APIs ----------

ohos::file::volumeManager::Disk GetDiskByIdSync(::taihe::string_view diskId)
{
    auto diskInfo = std::make_shared<OHOS::DiskManager::Disk>();
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return MakeEmptyTaiheDisk();
    }
    int32_t errNum = instance->GetDiskById(std::string(diskId), *diskInfo);
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return MakeEmptyTaiheDisk();
    }
    auto &volumeIds = diskInfo->GetVolumeIds();
    std::vector<taihe::string> volIdStrs;
    for (const auto &id : volumeIds) {
        volIdStrs.push_back(taihe::string(id));
    }
    auto volIdArray = taihe::array<taihe::string>(taihe::copy_data_t{}, volIdStrs.data(), volIdStrs.size());
    return {diskInfo->GetDiskId(),
            diskInfo->GetSizeBytes(),
            ToTaiheDiskType(diskInfo->GetDiskType()),
            diskInfo->GetRemovable(),
            volIdArray,
            diskInfo->GetExtraInfo()};
}

taihe::array<ohos::file::volumeManager::Disk> GetAllDisksSync()
{
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return taihe::array<ohos::file::volumeManager::Disk>::make(0, MakeEmptyTaiheDisk());
    }
    auto disks = std::make_shared<std::vector<OHOS::DiskManager::Disk>>();
    int32_t errNum = instance->GetAllDisks(*disks);
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return taihe::array<ohos::file::volumeManager::Disk>::make(0, MakeEmptyTaiheDisk());
    }

    std::vector<ohos::file::volumeManager::Disk> taiheDisks;
    for (const auto &disk : *disks) {
        auto &volumeIds = disk.GetVolumeIds();
        std::vector<taihe::string> volIdStrs;
        for (const auto &id : volumeIds) {
            volIdStrs.push_back(taihe::string(id));
        }
        auto volIdArray = taihe::array<taihe::string>(taihe::copy_data_t{}, volIdStrs.data(), volIdStrs.size());
        ohos::file::volumeManager::Disk taiheDisk{disk.GetDiskId(),    disk.GetSizeBytes(),
                                                  ToTaiheDiskType(disk.GetDiskType()),
                                                  disk.GetRemovable(), volIdArray,          disk.GetExtraInfo()};
        taiheDisks.push_back(taiheDisk);
    }
    return taihe::array<ohos::file::volumeManager::Disk>(taihe::copy_data_t{}, taiheDisks.data(), taiheDisks.size());
}

// ---------- Partition management APIs (@since 26.0.0) ----------

ohos::file::volumeManager::PartitionTableInfo GetPartitionTableSync(::taihe::string_view diskId)
{
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return {};
    }
    auto tableInfo = std::make_shared<OHOS::DiskManager::PartitionTableInfo>();
    int32_t errNum = instance->GetPartitionTable(std::string(diskId), *tableInfo);
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return {};
    }
    auto &partitions = tableInfo->GetPartitions();
    std::vector<ohos::file::volumeManager::PartitionInfo> taihePartitions;
    for (const auto &part : partitions) {
        taihePartitions.push_back({part.GetPartitionNum(), part.GetDiskId(), part.GetStartSector(), part.GetEndSector(),
                                   part.GetSizeBytes(), part.GetFsType()});
    }
    auto partitionArray = taihe::array<ohos::file::volumeManager::PartitionInfo>(
        taihe::copy_data_t{}, taihePartitions.data(), taihePartitions.size());
    return {tableInfo->GetDiskId(),
            tableInfo->GetTableType(),
            tableInfo->GetPartitionCount(),
            tableInfo->GetTotalSector(),
            tableInfo->GetSectorSize(),
            tableInfo->GetAlignSector(),
            partitionArray};
}

void CreatePartitionSync(::taihe::string_view diskId, const ohos::file::volumeManager::PartitionParams &params)
{
    OHOS::DiskManager::PartitionParams nativeParams;
    nativeParams.SetPartitionNum(params.partitionNum);
    nativeParams.SetStartSector(params.startSector);
    nativeParams.SetEndSector(params.endSector);
    nativeParams.SetTypeCode(std::string(params.typeCode));
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return;
    }
    int32_t errNum = instance->CreatePartition(std::string(diskId), nativeParams);
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return;
    }
}

void DeletePartitionSync(::taihe::string_view diskId, int32_t partitionNum)
{
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return;
    }
    int32_t errNum = instance->DeletePartition(std::string(diskId), partitionNum);
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return;
    }
}

void FormatPartitionSync(::taihe::string_view diskId, int32_t partitionNum,
                         const ohos::file::volumeManager::FormatParams &params)
{
    OHOS::DiskManager::FormatParams nativeParams;
    nativeParams.SetFsType(std::string(params.fsType));
    if (params.quickFormat.has_value()) {
        nativeParams.SetQuickFormat(params.quickFormat.value());
    }
    if (params.volumeName.has_value()) {
        std::string volName = std::string(params.volumeName.value());
        nativeParams.SetVolumeName(volName);
    }
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return;
    }
    int32_t errNum = instance->FormatPartition(std::string(diskId), partitionNum, nativeParams);
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return;
    }
}

// ---------- Optical drive APIs (@since 26.0.0) ----------

void EraseSync(::taihe::string_view volumeId)
{
    std::string volumeIdString = std::string(volumeId);
    if (volumeIdString.empty()) {
        LOGE("Invalid parameter, volumeId is empty");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_PARAMS);
        return;
    }
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return;
    }
    int32_t errNum = instance->Erase(volumeIdString);
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return;
    }
}

void EjectSync(::taihe::string_view diskId)
{
    std::string diskIdString = std::string(diskId);
    if (diskIdString.empty()) {
        LOGE("Invalid parameter, diskId is empty");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_PARAMS);
        return;
    }
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return;
    }
    int32_t errNum = instance->Eject(diskIdString);
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return;
    }
}

void CreateIsoImageSync(::taihe::string_view volumeId, ::taihe::string_view path)
{
    std::string volumeIdString = std::string(volumeId);
    std::string pathString = std::string(path);
    if (volumeIdString.empty() || pathString.empty()) {
        LOGE("Invalid parameter, volumeId or path is empty");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_PARAMS);
        return;
    }
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return;
    }
    int32_t errNum = instance->CreateIsoImage(volumeIdString, pathString);
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return;
    }
}

void BurnSync(::taihe::string_view volumeId, const ohos::file::volumeManager::BurnOptions &options)
{
    std::string volumeIdString = std::string(volumeId);
    if (volumeIdString.empty()) {
        LOGE("Invalid parameter, volumeId is empty");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_PARAMS);
        return;
    }

    // Build burn options string
    std::ostringstream oss;
    oss << "diskName=" << std::string(options.diskName) << '\n';
    oss << "burnPath=" << std::string(options.burnPath) << '\n';
    oss << "isIsoImage=" << (options.isIsoImage ? "true" : "false") << '\n';
    oss << "burnSpeed=" << options.burnSpeed << '\n';
    oss << "fsType=" << std::string(options.fsType) << '\n';
    oss << "isIncBurnSupport=" << (options.isIncBurnSupport ? "true" : "false") << '\n';

    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return;
    }
    int32_t errNum = instance->Burn(volumeIdString, oss.str());
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return;
    }
}

int32_t GetOpProcessSync(::taihe::string_view volumeId)
{
    std::string volumeIdString = std::string(volumeId);
    if (volumeIdString.empty()) {
        LOGE("Invalid parameter, volumeId is empty");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_PARAMS);
        return -1;
    }
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return -1;
    }
    int32_t progress = 0;
    int32_t errNum = instance->GetVolumeOpProcess(volumeIdString, progress);
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return -1;
    }
    return progress;
}

void VerifyBurnDataSync(::taihe::string_view volumeId, int32_t verifyType)
{
    std::string volumeIdString = std::string(volumeId);
    if (volumeIdString.empty()) {
        LOGE("Invalid parameter, volumeId is empty");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_PARAMS);
        return;
    }
    auto instance = OHOS::DelayedSingleton<OHOS::DiskManager::DiskManagerClient>::GetInstance();
    if (instance == nullptr) {
        LOGE("Get DiskManagerClient instance failed");
        OHOS::StorageTaiheError::SetStorageTaiheError(OHOS::E_IPCSS);
        return;
    }
    int32_t vType = static_cast<int32_t>(verifyType);
    int32_t errNum = instance->VerifyBurnData(volumeIdString, vType);
    if (errNum != OHOS::E_OK) {
        OHOS::StorageTaiheError::SetStorageTaiheError(errNum);
        return;
    }
}

} // namespace ANI::VolumeManager

// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_GetVolumeByUuidSync(ANI::VolumeManager::GetVolumeByUuidSync);
TH_EXPORT_CPP_API_GetAllVolumesSync(ANI::VolumeManager::GetAllVolumesSync);
TH_EXPORT_CPP_API_FormatSync(ANI::VolumeManager::FormatSync);
TH_EXPORT_CPP_API_GetVolumeByIdSync(ANI::VolumeManager::GetVolumeByIdSync);
TH_EXPORT_CPP_API_MountSync(ANI::VolumeManager::MountSync);
TH_EXPORT_CPP_API_UnmountSync(ANI::VolumeManager::UnmountSync);
TH_EXPORT_CPP_API_PartitionSync(ANI::VolumeManager::PartitionSync);
TH_EXPORT_CPP_API_SetVolumeDescriptionSync(ANI::VolumeManager::SetVolumeDescriptionSync);

// Disk management exports
TH_EXPORT_CPP_API_GetAllDisksSync(ANI::VolumeManager::GetAllDisksSync);
TH_EXPORT_CPP_API_GetDiskByIdSync(ANI::VolumeManager::GetDiskByIdSync);

// Partition management exports
TH_EXPORT_CPP_API_GetPartitionTableSync(ANI::VolumeManager::GetPartitionTableSync);
TH_EXPORT_CPP_API_CreatePartitionSync(ANI::VolumeManager::CreatePartitionSync);
TH_EXPORT_CPP_API_DeletePartitionSync(ANI::VolumeManager::DeletePartitionSync);
TH_EXPORT_CPP_API_FormatPartitionSync(ANI::VolumeManager::FormatPartitionSync);

// Optical drive exports (@since 26.0.0)
TH_EXPORT_CPP_API_EraseSync(ANI::VolumeManager::EraseSync);
TH_EXPORT_CPP_API_EjectSync(ANI::VolumeManager::EjectSync);
TH_EXPORT_CPP_API_CreateIsoImageSync(ANI::VolumeManager::CreateIsoImageSync);
TH_EXPORT_CPP_API_BurnSync(ANI::VolumeManager::BurnSync);
TH_EXPORT_CPP_API_GetOpProcessSync(ANI::VolumeManager::GetOpProcessSync);
TH_EXPORT_CPP_API_VerifyBurnDataSync(ANI::VolumeManager::VerifyBurnDataSync);
// NOLINTEND
