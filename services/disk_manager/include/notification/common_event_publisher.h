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

#ifndef OHOS_FILEMANAGEMENT_DISK_MANAGER_COMMON_EVENT_PUBLISHER_H
#define OHOS_FILEMANAGEMENT_DISK_MANAGER_COMMON_EVENT_PUBLISHER_H

#include "disk.h"
#include "volume_external.h"

namespace OHOS {
namespace DiskManager {

enum class DiskEventKind {
    MOUNTED,
    REMOVED,
};

/**
 * @brief 发布卷、磁盘维度的系统公共事件（COMMON_EVENT_*）。
 *
 * 根据 VolumeExternal、Disk 等入参封装 Want，并通过公共事件服务下发。接口均为静态方法。
 */
class CommonEventPublisher {
public:
    static void PublishVolumeChange(VolumeState notifyCode, const VolumeExternal &volume);
    static void PublishDiskChange(DiskEventKind kind, const Disk &disk);

    CommonEventPublisher() = delete;
    ~CommonEventPublisher() = delete;
    CommonEventPublisher(const CommonEventPublisher &) = delete;
    CommonEventPublisher &operator=(const CommonEventPublisher &) = delete;
    CommonEventPublisher(CommonEventPublisher &&) = delete;
    CommonEventPublisher &operator=(CommonEventPublisher &&) = delete;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_FILEMANAGEMENT_DISK_MANAGER_COMMON_EVENT_PUBLISHER_H
