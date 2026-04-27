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

#ifndef OHOS_DISK_MANAGER_DISK_MANAGER_MOCK_H
#define OHOS_DISK_MANAGER_DISK_MANAGER_MOCK_H

#include <gmock/gmock.h>
#include <string>
#include <vector>

#include "idisk_manager.h"
#include "errors.h"
#include "iremote_stub.h"
#include "volume_external.h"
#include "disk.h"

namespace OHOS {
namespace DiskManager {

class DiskManagerMock final : public IRemoteStub<IDiskManager> {
public:
    MOCK_METHOD(int, SendRequest, (uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option),
                (override));

    MOCK_METHOD(int32_t, Mount, (const std::string &volumeId), (override));
    MOCK_METHOD(int32_t, Unmount, (const std::string &volumeId), (override));
    MOCK_METHOD(int32_t, Format, (const std::string &volumeId, const std::string &fsType), (override));
    MOCK_METHOD(int32_t, TryToFix, (const std::string &volumeId), (override));
    MOCK_METHOD(int32_t, SetVolumeDescription, (const std::string &fsUuid, const std::string &description), (override));
    MOCK_METHOD(int32_t, GetAllVolumes, (std::vector<VolumeExternal> &vecOfVol), (override));
    MOCK_METHOD(int32_t, GetVolumeByUuid, (const std::string &fsUuid, VolumeExternal &vc), (override));
    MOCK_METHOD(int32_t, GetVolumeById, (const std::string &volumeId, VolumeExternal &vc), (override));
    MOCK_METHOD(int32_t, GetFreeSizeOfVolume, (const std::string &volumeUuid, int64_t &freeSize), (override));
    MOCK_METHOD(int32_t, GetTotalSizeOfVolume, (const std::string &volumeUuid, int64_t &totalSize), (override));
    MOCK_METHOD(int32_t, Partition, (const std::string &diskId, int32_t type), (override));
    MOCK_METHOD(int32_t, GetAllDisks, (std::vector<Disk> &vecOfDisk), (override));
    MOCK_METHOD(int32_t, GetDiskById, (const std::string &diskId, Disk &disk), (override));
    MOCK_METHOD(int32_t, QueryUsbIsInUse, (const std::string &diskPath, bool &isInUse), (override));
    MOCK_METHOD(int32_t, IsUsbFuseByType, (int32_t type, bool &isUsbFuse), (override));
    MOCK_METHOD(int32_t, OnBlockDiskUevent, (const std::string &rawUeventMsg), (override));
    MOCK_METHOD(int32_t, NotifyMtpMounted,
                (const std::string &id, const std::string &path, const std::string &desc, const std::string &uuid,
                 const std::string &fsType),
                (override));
    MOCK_METHOD(int32_t, NotifyMtpUnmounted, (const std::string &id, bool isBadRemove), (override));
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_DISK_MANAGER_MOCK_H
