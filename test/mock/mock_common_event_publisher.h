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

#ifndef OHOS_DISK_MANAGER_MOCK_COMMON_EVENT_PUBLISHER_H
#define OHOS_DISK_MANAGER_MOCK_COMMON_EVENT_PUBLISHER_H

#include "disk.h"
#include "volume_external.h"

#include <gmock/gmock.h>

namespace OHOS {
namespace DiskManager {

enum class DiskEventKind {
    MOUNTED,
    REMOVED,
};

class CommonEventPublisher {
public:
    CommonEventPublisher() = default;
    ~CommonEventPublisher() = default;

    MOCK_METHOD(void, PublishVolumeChangeImpl, (VolumeState notifyCode, const VolumeExternal &volume));
    MOCK_METHOD(void, PublishDiskChangeImpl, (DiskEventKind kind, const Disk &disk));
    MOCK_METHOD(void, PublishVolumeResultImpl, (VolumeState notifyCode, const VolumeExternal &volume));

    static CommonEventPublisher *mockInstance_;

    static CommonEventPublisher &GetInstance();

    static void PublishVolumeChange(VolumeState notifyCode, const VolumeExternal &volume);
    static void PublishDiskChange(DiskEventKind kind, const Disk &disk);
    static void PublishVolumeResult(VolumeState notifyCode, const VolumeExternal &volume);
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_MOCK_COMMON_EVENT_PUBLISHER_H