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

// 卷/磁盘维度的系统公共事件（COMMON_EVENT_*）发布。
// 由卷管理、磁盘管理等模块在整理好 VolumeExternal / Disk 后调用；
// DiskManagerProvider 仅承载 IDL 分发，不应内聚此类实现细节。
class CommonEventPublisher {
public:
    static CommonEventPublisher &GetInstance();

    void PublishVolumeChange(VolumeState notifyCode, const VolumeExternal &volume);
    void PublishDiskChange(DiskEventKind kind, const Disk &disk);

    CommonEventPublisher(const CommonEventPublisher &) = delete;
    CommonEventPublisher &operator=(const CommonEventPublisher &) = delete;

private:
    CommonEventPublisher() = default;
    ~CommonEventPublisher() = default;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_FILEMANAGEMENT_DISK_MANAGER_COMMON_EVENT_PUBLISHER_H
