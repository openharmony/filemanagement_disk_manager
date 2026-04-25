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

#ifndef OHOS_FILEMANAGEMENT_DISK_MANAGER_USB_FUSE_ADAPTER_H
#define OHOS_FILEMANAGEMENT_DISK_MANAGER_USB_FUSE_ADAPTER_H

#include <cstdint>
#include <string>

#include "nocopyable.h"

namespace OHOS {
namespace DiskManager {

/**
 * 动态加载 libspace_ability_fuse_ext，对外部卷 FUSE 策略扩展进行通知与查询。
 * 语义对齐原 storage_manager 中 VolumeManagerServiceExt。
 */
class UsbFuseAdapter : public NoCopyable {
public:
    static UsbFuseAdapter &GetInstance();

    int32_t NotifyUsbFuseMount(int fuseFd, const std::string &volumeId, const std::string &fsUuid);
    int32_t NotifyUsbFuseUmount(const std::string &volumeId);
    bool IsUsbFuseByType(const std::string &fsType);
    bool IsUsbFuseEnabledForFsType(const std::string &fsType);

private:
    UsbFuseAdapter();
    ~UsbFuseAdapter();

    void Init();
    void UnInit();

    void *handler_{nullptr};
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_FILEMANAGEMENT_DISK_MANAGER_USB_FUSE_ADAPTER_H
