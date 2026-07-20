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
#include "disk.h"
#include "disk_manager_hilog.h"
#include "disk_manager_errno.h"
#include "disk/disk_manager.h"
#include "int_wrapper.h"
#include "long_wrapper.h"
#include "string_wrapper.h"
#include "want.h"
#include "want_params.h"
#ifdef CDC_STORAGE
#include "common_event_publish_info.h"
#endif

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
    {CHECKING, "CHECKING", EventFwk::CommonEventSupport::COMMON_EVENT_DISK_VOLUME_STATE_CHANGE},
    {REPAIR_FINISH_FAIL, "REPAIR_FINISH_FAIL", EventFwk::CommonEventSupport::COMMON_EVENT_DISK_VOLUME_STATE_CHANGE},
    {REPAIR_FINISH_SUCCESS, "REPAIR_FINISH_SUCCESS",
        EventFwk::CommonEventSupport::COMMON_EVENT_DISK_VOLUME_STATE_CHANGE},
    {FORMAT_FINISH_FAIL, "FORMAT_FINISH_FAIL", EventFwk::CommonEventSupport::COMMON_EVENT_DISK_VOLUME_STATE_CHANGE},
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
                    volume.GetId().c_str(), static_cast<long long>(freeSize));
            } else {
                wantParams.SetParam("freeSize", AAFwk::Long::Box64(freeSize));
                LOGI("Volume mounted: id=%{public}s, fsType=%{public}d, freeSize=%{public}lld",
                    volume.GetId().c_str(), volume.GetFsType(), static_cast<long long>(freeSize));
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
                volume.GetId().c_str(), static_cast<long long>(freeSize));
        } else {
            wantParams.SetParam("freeSize", AAFwk::Long::Box64(freeSize));
            LOGI("Volume unmounted: id=%{public}s, fsType=%{public}d, freeSize=%{public}lld",
                volume.GetId().c_str(), volume.GetFsType(), static_cast<long long>(freeSize));
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
    if (notifyCode == UNMOUNTED) {
        SetUnmountedEventParams(wantParams, volume);
    }
    want.SetParams(wantParams);
    EventFwk::CommonEventData commonData{want};
#ifdef CDC_STORAGE
    // for car：DVR USB volume's common event only sent to user 0, other users should not be aware of DVR volume change
    constexpr int32_t DVR_ALLOWED_USER_ID = 0;
    if (volume.GetFlags() == static_cast<int32_t>(DVR_USB)) {
        LOGI("CommonEventPublisher DVR volume, only publish to user 0, id=%{public}s, volumeState=%{public}d",
             volume.GetId().c_str(), static_cast<int32_t>(notifyCode));
        EventFwk::CommonEventManager::PublishCommonEventAsUser(commonData, DVR_ALLOWED_USER_ID);
        return;
    }
#endif
    EventFwk::CommonEventManager::PublishCommonEvent(commonData);
}

void CommonEventPublisher::PublishVolumeResult(VolumeState notifyCode, const VolumeExternal &volume)
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
            LOGI("CommonEventPublisher volume result evt %{public}s id=%{public}s",
                info.logMessage, volume.GetId().c_str());
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
} // namespace DiskManager
} // namespace OHOS
