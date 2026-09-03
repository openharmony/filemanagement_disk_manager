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
#ifndef OHOS_FILEMANAGEMENT_DISK_MANAGER_EDM_ADAPTER_H
#define OHOS_FILEMANAGEMENT_DISK_MANAGER_EDM_ADAPTER_H

#include <cstdint>
#include <string>

#include "disk.h"
#include "mount_param.h"
#include "nocopyable.h"
#include "volume_external.h"

namespace OHOS {
namespace DiskManager {

class EdmAdapter : public NoCopyable {
public:
    static EdmAdapter &GetInstance();

    bool IsEdmEnableOddBurn(const std::string &diskId, int32_t callerUserId);

    /**
     * 判断 EDM 是否拦截本次挂载。返回 true 表示应拦截（跳过挂载）。
     * 拦截条件：企业设备 + 拦截策略开启 + 非 EDM 自身发起 + 受管磁盘类型。
     */
    bool IsEdmControlMountEnabled(const VolumeExternal &volume, const MountParam &mountParam);

private:
    EdmAdapter();
    ~EdmAdapter();

    bool IsExternalOddBurnAllowed(int32_t userId, const std::string &pid, const std::string &vid,
                                 const std::string &sn);
#ifdef EDM_ADAPTER_ENABLE
    int32_t NotifyExternalStorageDeviceAdd(const VolumeExternal &volume, const Disk &disk);
#endif
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_FILEMANAGEMENT_DISK_MANAGER_EDM_ADAPTER_H
