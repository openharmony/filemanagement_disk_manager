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

#include "diskmanagerclient_fuzzer.h"

#include "disk_manager_client.h"
#include "disk_manager_stub.h"
#include "ipc/disk_manager_provider.h"
#include "system_ability_definition.h"

namespace OHOS {
using namespace DiskManager;

namespace {
constexpr size_t MIN_DATA_SIZE = sizeof(int32_t);
constexpr int32_t SECTOR_MULTIPLIER = 2;
} // namespace

void FuzzClientVolumeApis(const std::string &volumeId, const std::string &fsUuid)
{
    auto &client = DiskManagerClient::GetInstance();
    client.Mount(volumeId);
    client.Unmount(volumeId);
    client.Format(volumeId, "ext4");
    client.SetVolumeDescription(fsUuid, "desc");
    std::vector<VolumeExternal> volumes;
    client.GetAllVolumes(volumes);
    VolumeExternal vc;
    client.GetVolumeByUuid(fsUuid, vc);
    client.GetVolumeById(volumeId, vc);
    int64_t freeSize = 0;
    client.GetFreeSizeOfVolume(volumeId, freeSize);
    int64_t totalSize = 0;
    client.GetTotalSizeOfVolume(volumeId, totalSize);
}

void FuzzClientDiskApis(const std::string &diskId, int32_t flag)
{
    auto &client = DiskManagerClient::GetInstance();
    std::vector<Disk> disks;
    client.GetAllDisks(disks);
    Disk disk;
    client.GetDiskById(diskId, disk);
    client.Partition(diskId, flag);
}

void FuzzClientPartitionApis(const std::string &diskId, int32_t flag)
{
    auto &client = DiskManagerClient::GetInstance();
    PartitionTableInfo tableInfo;
    client.GetPartitionTable(diskId, tableInfo);
    PartitionParams params(flag, flag, flag * SECTOR_MULTIPLIER, "ext4");
    client.CreatePartition(diskId, params);
    client.DeletePartition(diskId, flag);
    FormatParams fmtParams("ext4", true, "testVol");
    client.FormatPartition(diskId, flag, fmtParams);
}

void FuzzClientOpticalApis(const std::string &volumeId, const std::string &diskId)
{
    auto &client = DiskManagerClient::GetInstance();
    client.Erase(volumeId);
    client.Eject(diskId);
    client.CreateIsoImage(volumeId, "/tmp/test.iso");
    client.Burn(volumeId, "{}");
    int32_t progress = 0;
    client.GetVolumeOpProcess(volumeId, progress);
}

void FuzzClientInnerApis(const std::string &volumeId, const std::string &diskId)
{
    auto &client = DiskManagerClient::GetInstance();
    client.ResetProxy();
    client.TryToFix(volumeId);
    bool isInUse = false;
    client.QueryUsbIsInUse("/mnt/data/usb", isInUse);
    client.NotifyMtpMounted("mtp-1", "/mnt/mtp", "desc", "uuid-1", "ext4");
    client.NotifyMtpUnmounted("mtp-1", false);
    client.OnBlockDiskUevent("ACTION=add;DEVPATH=/devices/sda");
}

bool DiskManagerClientFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_DATA_SIZE) {
        return false;
    }

    int32_t flag = *(reinterpret_cast<const int32_t *>(data));
    std::string volumeId = "vol-" + std::to_string(flag);
    std::string diskId = "disk-" + std::to_string(flag);
    std::string fsUuid = "uuid-" + std::to_string(flag);

    FuzzClientVolumeApis(volumeId, fsUuid);
    FuzzClientDiskApis(diskId, flag);
    FuzzClientPartitionApis(diskId, flag);
    FuzzClientOpticalApis(volumeId, diskId);
    FuzzClientInnerApis(volumeId, diskId);

    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::DiskManagerClientFuzzTest(data, size);
    return 0;
}
