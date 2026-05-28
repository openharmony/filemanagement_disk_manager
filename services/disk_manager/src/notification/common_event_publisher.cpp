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

#include "notification/common_event_publisher.h"

#include "common_event_data.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "disk_manager_hilog.h"
#include "disk/disk_manager.h" 
#include "int_wrapper.h"
#include "string_wrapper.h"
#include "want.h"
#include "want_params.h"

namespace OHOS {
namespace DiskManager {

namespace {

struct VolumeStateInfo {
    VolumeState state;
    const char *logMessage;
    std::string eventAction;
};

const VolumeStateInfo STATE_INFOS[] = {
    {REMOVED, "VOLUME_REMOVED", EventFwk::CommonEventSupport::COMMON_EVENT_VOLUME_REMOVED},
    {UNMOUNTED, "VOLUME_UNMOUNTED", EventFwk::CommonEventSupport::COMMON_EVENT_VOLUME_UNMOUNTED},
    {MOUNTED, "VOLUME_MOUNTED", EventFwk::CommonEventSupport::COMMON_EVENT_VOLUME_MOUNTED},
    {BAD_REMOVAL, "VOLUME_BAD_REMOVAL", EventFwk::CommonEventSupport::COMMON_EVENT_VOLUME_BAD_REMOVAL},
    {EJECTING, "VOLUME_EJECT", EventFwk::CommonEventSupport::COMMON_EVENT_VOLUME_EJECT},
    {DAMAGED, "DeskDamaged", EventFwk::CommonEventSupport::COMMON_EVENT_DISK_UNMOUNTABLE},
    {DAMAGED_MOUNTED, "DeskDamagedMounted", EventFwk::CommonEventSupport::COMMON_EVENT_DISK_UNMOUNTABLE},
    {ENCRYPTING, "DeskEncrypting", EventFwk::CommonEventSupport::COMMON_EVENT_DISK_BAD_REMOVAL},
    {ENCRYPTED_AND_LOCKED, "DeskEncryptedAndLocked", EventFwk::CommonEventSupport::COMMON_EVENT_DISK_BAD_REMOVAL},
    {ENCRYPTED_AND_UNLOCKED, "DeskEncryptedAndUnLocked", EventFwk::CommonEventSupport::COMMON_EVENT_DISK_BAD_REMOVAL},
    {DECRYPTING, "DeskDecrypting", EventFwk::CommonEventSupport::COMMON_EVENT_DISK_BAD_REMOVAL},
};

void SetMountedEventParams(AAFwk::WantParams &wantParams, const VolumeExternal &volume)
{
    wantParams.SetParam("path", AAFwk::String::Box(volume.GetPath()));
    wantParams.SetParam("fsType", AAFwk::Integer::Box(volume.GetFsType()));
    if (volume.GetFsType() == FsType::MTP || volume.GetFsType() == FsType::PTP) {
        LOGI("Volume mounted: id=%{public}s, fsType=%{public}d (MTP/PTP device, freeSize not set)",
            volume.GetId().c_str(), volume.GetFsType());
    } else {
        int64_t freeSize = 0;
        int32_t ret = DiskManager::GetInstance().GetFreeSizeOfVolume(volume.GetUuid(), freeSize);
        if (ret == E_OK) {
            if (freeSize < 0) {
                LOGW("Volume mounted: id=%{public}s, invalid freeSize=%{public}lld, skip setting",
                    volume.GetId().c_str(), (long long)freeSize);
            } else {
                wantParams.SetParam("freeSize", AAFwk::Long::Box(freeSize));
                LOGI("Volume mounted: id=%{public}s, fsType=%{public}d, freeSize=%{public}lld",
                    volume.GetId().c_str(), volume.GetFsType(), (long long)freeSize);
            }
        } else {
            LOGW("Volume mounted: id=%{public}s, failed to get freeSize, ret=%{public}d",
                volume.GetId().c_str(), ret);
        }
    }
}

void SetUnmountedEventParams(AAFwk::WantParams &wantParams, const VolumeExternal &volume)
{
    wantParams.SetParam("fsType", AAFwk::Integer::Box(volume.GetFsType()));

    // MTP/PTP devices do not support statvfs, so freeSize is not applicable
    if (volume.GetFsType() == FsType::MTP || volume.GetFsType() == FsType::PTP) {
        LOGI("Volume unmounted: id=%{public}s, fsType=%{public}d (MTP/PTP device, freeSize not set)",
            volume.GetId().c_str(), volume.GetFsType());
    } else {
        int64_t freeSize = volume.GetFreeSize();
        if (freeSize < 0) {
            LOGW("Volume unmounted: id=%{public}s, invalid freeSize=%{public}lld, skip setting",
                volume.GetId().c_str(), (long long)freeSize);
        } else {
            wantParams.SetParam("freeSize", AAFwk::Long::Box(freeSize));
            LOGI("Volume unmounted: id=%{public}s, fsType=%{public}d, freeSize=%{public}lld",
                volume.GetId().c_str(), volume.GetFsType(), (long long)freeSize);
        }
    }
}
} // namespace

void CommonEventPublisher::PublishVolumeChange(VolumeState notifyCode, const VolumeExternal &volume)
{
    AAFwk::Want want;
    AAFwk::WantParams wantParams;
    wantParams.SetParam("id", AAFwk::String::Box(volume.GetId()));
    wantParams.SetParam("diskId", AAFwk::String::Box(volume.GetDiskId()));
    wantParams.SetParam("fsUuid", AAFwk::String::Box(volume.GetUuid()));
    wantParams.SetParam("flags", AAFwk::Integer::Box(volume.GetFlags()));
    wantParams.SetParam("volumeState", AAFwk::Integer::Box(static_cast<int32_t>(notifyCode)));

    bool actionSet = false;
    for (const auto &info : STATE_INFOS) {
        if (info.state == notifyCode) {
            LOGI("CommonEventPublisher volume evt %{public}s id=%{public}s", info.logMessage, volume.GetId().c_str());
            want.SetAction(info.eventAction);
            actionSet = true;
            break;
        }
    }

    if (!actionSet) {
        LOGW("CommonEventPublisher skip publish: no action for volumeState=%{public}d id=%{public}s",
             static_cast<int32_t>(notifyCode), volume.GetId().c_str());
        return;
    }

    if (notifyCode == MOUNTED) {
        SetMountedEventParams(wantParams, volume);
    }
    if (notifyCode == VolumeState::UNMOUNTED) {
 	    SetUnmountedEventParams(wantParams, volume);
 	}
    want.SetParams(wantParams);
    EventFwk::CommonEventData commonData{want};
    EventFwk::CommonEventManager::PublishCommonEvent(commonData);
}

void CommonEventPublisher::PublishDiskChange(DiskEventKind kind, const Disk &disk)
{
    AAFwk::Want want;
    AAFwk::WantParams wantParams;
    wantParams.SetParam("diskId", AAFwk::String::Box(disk.GetDiskId()));
    wantParams.SetParam("sizeBytes", AAFwk::Integer::Box(disk.GetSizeBytes()));
    wantParams.SetParam("sysPath", AAFwk::String::Box(disk.GetSysPath()));
    wantParams.SetParam("flag", AAFwk::Integer::Box(disk.GetDiskType()));

    if (kind == DiskEventKind::MOUNTED) {
        // 与 storage_daemon DiskInfo::DiskState::MOUNTED 一致（枚举首项为 0）
        wantParams.SetParam("diskState", AAFwk::Integer::Box(0));
        want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_DISK_MOUNTED);
        LOGI("CommonEventPublisher DISK_MOUNTED diskId=%{public}s", disk.GetDiskId().c_str());
    } else {
        want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_DISK_REMOVED);
        LOGI("CommonEventPublisher DISK_REMOVED diskId=%{public}s", disk.GetDiskId().c_str());
    }

    want.SetParams(wantParams);
    EventFwk::CommonEventData commonData{want};
    EventFwk::CommonEventManager::PublishCommonEvent(commonData);
}

void SetMountedEventParams(AAFwk::WantParams &wantParams, const VolumeExternal &volume)
{
    wantParams.SetParam("path", AAFwk::String::Box(volume.GetPath()));
    wantParams.SetParam("fsType", AAFwk::Integer::Box(volume.GetFsType()));
    if (volume.GetFsType() == FsType::MTP || volume.GetFsType() == FsType::PTP) {
        LOGI("Volume mounted: id=%{public}s, fsType=%{public}d (MTP/PTP device, freeSize not set)",
            volume.GetId().c_str(), volume.GetFsType());
    } else {
        int64_t freeSize = 0;
        int32_t ret = DiskManager::GetInstance().GetFreeSizeOfVolume(volume.GetUuid(), freeSize);
        if (ret == E_OK) {
            if (freeSize < 0) {
                LOGW("Volume mounted: id=%{public}s, invalid freeSize=%{public}lld, skip setting",
                    volume.GetId().c_str(), (long long)freeSize);
            } else {
                wantParams.SetParam("freeSize", AAFwk::Long::Box(freeSize));
                LOGI("Volume mounted: id=%{public}s, fsType=%{public}d, freeSize=%{public}lld",
                    volume.GetId().c_str(), volume.GetFsType(), (long long)freeSize);
            }
        } else {
            LOGW("Volume mounted: id=%{public}s, failed to get freeSize, ret=%{public}d",
                volume.GetId().c_str(), ret);
        }
    }
}

void SetUnmountedEventParams(AAFwk::WantParams &wantParams, const VolumeExternal &volume)
{
    wantParams.SetParam("fsType", AAFwk::Integer::Box(volume.GetFsType()));

    // MTP/PTP devices do not support statvfs, so freeSize is not applicable
    if (volume.GetFsType() == FsType::MTP || volume.GetFsType() == FsType::PTP) {
        LOGI("Volume unmounted: id=%{public}s, fsType=%{public}d (MTP/PTP device, freeSize not set)",
            volume.GetId().c_str(), volume.GetFsType());
    } else {
        int64_t freeSize = volume.GetFreeSize();
        if (freeSize < 0) {
            LOGW("Volume unmounted: id=%{public}s, invalid freeSize=%{public}lld, skip setting",
                volume.GetId().c_str(), (long long)freeSize);
        } else {
            wantParams.SetParam("freeSize", AAFwk::Long::Box(freeSize));
            LOGI("Volume unmounted: id=%{public}s, fsType=%{public}d, freeSize=%{public}lld",
                volume.GetId().c_str(), volume.GetFsType(), (long long)freeSize);
        }
    }
}
} // namespace DiskManager
} // namespace OHOS
