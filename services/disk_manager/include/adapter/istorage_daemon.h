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

#ifndef OHOS_STORAGEDAEMON_ISTORAGEDAEMON_H
#define OHOS_STORAGEDAEMON_ISTORAGEDAEMON_H

#include "hilog/log.h"
#include <cstdint>
#include <iremote_broker.h>

namespace OHOS {
namespace StorageDaemon {

enum class IStorageDaemonIpcCode {
    COMMAND_MOUNT = 2,
    COMMAND_UMOUNT = 3,
    COMMAND_CHECK = 4,
    COMMAND_FORMAT = 5,
    COMMAND_PARTITION = 6,
    COMMAND_SET_VOLUME_DESCRIPTION = 7,
    COMMAND_QUERY_USB_IS_IN_USE = 45,
    COMMAND_TRY_TO_FIX = 51,
    COMMAND_MOUNT_USB_FUSE = 54,
};

class IStorageDaemon : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.StorageDaemon.IStorageDaemon");

    virtual ErrCode Mount(const std::string &volId, uint32_t flags) = 0;
    virtual ErrCode UMount(const std::string &volId) = 0;
    virtual ErrCode Check(const std::string &volId) = 0;
    virtual ErrCode Format(const std::string &volId, const std::string &fsType) = 0;
    virtual ErrCode Partition(const std::string &diskId, int32_t type) = 0;
    virtual ErrCode SetVolumeDescription(const std::string &volId, const std::string &description) = 0;
    virtual ErrCode QueryUsbIsInUse(const std::string &diskPath, bool &isInUse) = 0;
    virtual ErrCode TryToFix(const std::string &volId, uint32_t flags) = 0;
    virtual ErrCode MountUsbFuse(const std::string &volumeId, std::string &fsUuid, int &fuseFd) = 0;

protected:
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD004301, "StorageDaemon"};
};

} // namespace StorageDaemon
} // namespace OHOS

#endif // OHOS_STORAGEDAEMON_ISTORAGEDAEMON_H
