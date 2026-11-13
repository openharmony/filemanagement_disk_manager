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

#ifndef OHOS_DISK_MANAGER_VOLUME_CORE_H
#define OHOS_DISK_MANAGER_VOLUME_CORE_H

#include "parcel.h"

namespace OHOS {
namespace DiskManager {
enum VolumeType {
    EMULATED = 1,
    EXTERNAL,
};
enum VolumeState {
    UNMOUNTED = 0,
    CHECKING,
    MOUNTED,
    EJECTING,
    REMOVED,
    BAD_REMOVAL,
    DAMAGED,
    FUSE_REMOVED,
    ENCRYPTED_AND_LOCKED = 10,
    ENCRYPTED_AND_UNLOCKED,
    REPAIR_FINISH_FAIL = 13,
    REPAIR_FINISH_SUCCESS,
    FORMAT_FINISH_FAIL,
    REPAIR_START,
    REPAIRING,
};

// 异常卷原因码（封闭枚举）：按 U 盘识别顺序编号（分区表 → 卷元数据 → fsType → 挂载），
// 随 COMMON_EVENT_DISK_UNMOUNTABLE 广播携带，取值跨版本稳定；如需变更须走设计文档修订。
// 不对外新增接口，仅内部与广播数据使用。
enum AbnormalReason {
    PARTITION_TABLE_READ_FAILED = 1, // 分区表不可用（sgdisk 查询失败，或内容解析不出可用分区）
    METADATA_READ_FAILED = 2,       // 卷元数据读取失败（uuid/fsType 缺失或读取报错）
    FSTYPE_UNSUPPORTED = 3,         // 文件系统类型不在支持范围
    MOUNT_FAILED = 4,               // 卷挂载失败
};

class VolumeInfoStr : public Parcelable {
public:
    VolumeInfoStr(){};

    VolumeInfoStr(const std::string &volumeId_,
                  const std::string &fsTypeStr_,
                  const std::string &fsUuid_,
                  const std::string &path_,
                  const std::string &description_,
                  bool isDamaged_)
        : volumeId(volumeId_),
          fsTypeStr(fsTypeStr_),
          fsUuid(fsUuid_),
          path(path_),
          description(description_),
          isDamaged(isDamaged_){};

    std::string volumeId;
    std::string fsTypeStr;
    std::string fsUuid;
    std::string path;
    std::string description;
    bool isDamaged = false;

    bool Marshalling(Parcel &parcel) const override;
    static VolumeInfoStr *Unmarshalling(Parcel &parcel);
};

class VolumeCore : public Parcelable {
public:
    VolumeCore();
    VolumeCore(const std::string &id, int32_t type, const std::string &diskId);
    VolumeCore(const std::string &id, int32_t type, const std::string &diskId, int32_t state);
    VolumeCore(const std::string &id,
               int32_t type,
               const std::string &diskId,
               int32_t state,
               const std::string &fsType,
               const std::string &extraInfo = "");

    std::string GetId() const;
    int32_t GetType() const;
    std::string GetDiskId() const;
    int32_t GetState() const;
    std::string GetFsType() const;
    void SetState(int32_t state);
    void SetExtraInfo(const std::string &extraInfo);
    std::string GetExtraInfo() const;

    bool Marshalling(Parcel &parcel) const override;
    static VolumeCore *Unmarshalling(Parcel &parcel);

private:
    std::string id_;
    int32_t type_{};
    std::string diskId_;
    int32_t state_ = UNMOUNTED;
    bool errorFlag_ = false;
    std::string fsType_;
    std::string extraInfo_;
};
} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_VOLUME_CORE_H