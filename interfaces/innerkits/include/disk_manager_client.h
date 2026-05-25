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

#include "disk.h"
#include "partition_types.h"
#include "volume_external.h"

namespace OHOS {
namespace DiskManager {

class IDiskManager;

/** disk_manager SA 客户端；对外应用能力见 @ohos.file.volumeManager，进程内 / storage_daemon 专用接口见文末。 */
class DiskManagerClient : public NoCopyable {
    DECLARE_DELAYED_SINGLETON(DiskManagerClient);

public:
    /* ---------- Volume（@ohos.file.volumeManager） ---------- */
    int32_t Mount(const std::string &volumeId);
    int32_t Unmount(const std::string &volumeId);
    int32_t Format(const std::string &volumeId, const std::string &fsType);
    int32_t SetVolumeDescription(const std::string &fsUuid, const std::string &description);
    int32_t GetAllVolumes(std::vector<VolumeExternal> &vecOfVol);
    int32_t GetVolumeByUuid(const std::string &uuid, VolumeExternal &vc);
    int32_t GetVolumeById(const std::string &volumeId, VolumeExternal &vc);

    /** 卷容量查询；JS 声明见其他 .d.ts（非 volumeManager 主模块）。 */
    int32_t GetFreeSizeOfVolume(const std::string &volumeUuid, int64_t &freeSize);
    int32_t GetTotalSizeOfVolume(const std::string &volumeUuid, int64_t &totalSize);

    /* ---------- Disk（@ohos.file.volumeManager） ---------- */
    int32_t GetAllDisks(std::vector<Disk> &vecOfDisk);
    int32_t GetDiskById(const std::string &diskId, Disk &disk);
    int32_t Partition(const std::string &diskId, int32_t type);

    /* ---------- Partition（@ohos.file.volumeManager @since 26.0.0） ---------- */
    int32_t GetPartitionTable(const std::string &diskId, PartitionTableInfo &out);
    int32_t CreatePartition(const std::string &diskId, const PartitionParams &params);
    int32_t DeletePartition(const std::string &diskId, int32_t partitionNum);
    int32_t FormatPartition(const std::string &diskId, int32_t partitionNum, const FormatParams &params);

    /* ---------- Optical Disc / Burn（@ohos.file.volumeManager @since 26.0.0） ---------- */
    int32_t EraseVolume(const std::string &volumeId);
    int32_t EjectVolume(const std::string &volumeId);
    int32_t CreateIsoImage(const std::string &volumeId, const std::string &filePath);
    int32_t BurnVolume(const std::string &volumeId, const std::string &burnOptions);
    int32_t GetVolumeOpProcess(const std::string &volumeId, int32_t &progressPct);
    int32_t VerifyBurnData(const std::string &volumeId, int32_t verifyType);

    /*
     * ---------- Inner-only（IDiskManager / 进程内，不导出 @ohos.file.volumeManager） ----------
     * 含：Client 代理维护、卷修复、USB 占用/FUSE 策略查询。
     */
    int32_t ResetProxy();
    int32_t TryToFix(const std::string &volumeId);
    int32_t QueryUsbIsInUse(const std::string &diskPath, bool &isInUse);
    int32_t IsUsbFuseByType(int32_t type, bool &isUsbFuse);

    /* ---------- storage_daemon 回调（IDiskManager / 进程内，由 storage_daemon 调用） ---------- */
    int32_t NotifyMtpMounted(const std::string &id, const std::string &path, const std::string &desc,
                             const std::string &uuid, const std::string &fsType);
    int32_t NotifyMtpUnmounted(const std::string &id, const bool isBadRemove);
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
