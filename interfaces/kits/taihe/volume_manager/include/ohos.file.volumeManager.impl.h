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

#ifndef OHOS_FILE_VOLUMEMANAGER_IMPL_H
#define OHOS_FILE_VOLUMEMANAGER_IMPL_H
#include "ohos.file.volumeManager.impl.hpp"
#include "ohos.file.volumeManager.proj.hpp"
#include "taihe/runtime.hpp"

namespace ANI::VolumeManager {
ohos::file::volumeManager::Volume GetVolumeByUuidSync(taihe::string_view uuid);
taihe::array<ohos::file::volumeManager::Volume> GetAllVolumesSync();
ohos::file::volumeManager::Volume GetVolumeByIdSync(taihe::string_view volumeId);
void MountSync(taihe::string_view volumeId);
void UnmountSync(taihe::string_view volumeId);
void FormatSync(taihe::string_view volumeId, taihe::string_view fsType);
void PartitionSync(taihe::string_view diskId, int32_t type);
void SetVolumeDescriptionSync(taihe::string_view uuid, taihe::string_view description);

// Disk management APIs
ohos::file::volumeManager::Disk GetDiskByIdSync(taihe::string_view diskId);
taihe::array<ohos::file::volumeManager::Disk> GetAllDisksSync();

// Partition management APIs (new_api @since 26.0.0)
ohos::file::volumeManager::PartitionTableInfo GetPartitionTableSync(taihe::string_view diskId);
void CreatePartitionSync(taihe::string_view diskId,
                         const ohos::file::volumeManager::PartitionParams &params);
void DeletePartitionSync(taihe::string_view diskId, int32_t partitionNum);
void FormatPartitionSync(taihe::string_view diskId,
                         int32_t partitionNum,
                         const ohos::file::volumeManager::FormatParams &params);

// Optical drive APIs (@since 26.0.0)
void EraseSync(taihe::string_view volumeId);
void EjectSync(taihe::string_view diskId);
void CreateIsoImageSync(taihe::string_view volumeId, taihe::string_view path);
void BurnSync(taihe::string_view volumeId, const ohos::file::volumeManager::BurnOptions &options);
int32_t GetOpProcessSync(taihe::string_view volumeId);
void VerifyBurnDataSync(taihe::string_view volumeId, int32_t verifyType);
bool QueryUsbIsInUseSync(taihe::string_view diskPath);
} // namespace ANI::VolumeManager
#endif // OHOS_FILE_VOLUMEMANAGER_IMPL_H
