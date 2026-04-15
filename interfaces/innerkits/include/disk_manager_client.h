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

#ifndef OHOS_FILEMANAGEMENT_DISK_MANAGER_CLIENT_H
#define OHOS_FILEMANAGEMENT_DISK_MANAGER_CLIENT_H

#include <mutex>
#include <string>
#include <vector>

#include <iremote_object.h>
#include <nocopyable.h>
#include <refbase.h>
#include <singleton.h>

#include "volume_external.h"

namespace OHOS {
namespace DiskManager {

class IDiskManager;

class DiskManagerClient : public NoCopyable {
    DECLARE_DELAYED_SINGLETON(DiskManagerClient);

public:
    int32_t ResetProxy();

    int32_t Mount(const std::string &volumeId);
    int32_t Unmount(const std::string &volumeId);
    int32_t Format(const std::string &volumeId, const std::string &fsType);
    int32_t SetVolumeDescription(const std::string &fsUuid, const std::string &description);
    int32_t GetAllVolumes(std::vector<VolumeExternal> &vecOfVol);
    int32_t GetVolumeByUuid(const std::string &uuid, VolumeExternal &vc);
    int32_t GetVolumeById(const std::string &volumeId, VolumeExternal &vc);
    int32_t GetFreeSizeOfVolume(const std::string &volumeUuid, int64_t &freeSize);
    int32_t GetTotalSizeOfVolume(const std::string &volumeUuid, int64_t &totalSize);
    int32_t Partition(const std::string &diskId, int32_t type);
    int32_t OnBlockDiskUevent(const std::string &rawUeventMsg);

private:
    int32_t Connect(sptr<IDiskManager> &proxy);

    sptr<IDiskManager> diskManager_;
    sptr<IRemoteObject::DeathRecipient> deathRecipient_;
    std::mutex mutex_;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_FILEMANAGEMENT_DISK_MANAGER_CLIENT_H
