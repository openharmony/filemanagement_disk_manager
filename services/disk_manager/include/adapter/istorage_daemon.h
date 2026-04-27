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
#include <string>

namespace OHOS {
namespace StorageDaemon {

enum class IStorageDaemonIpcCode {
    COMMAND_QUERY_USB_IS_IN_USE = 45,
    /* IStorageDaemon.idl addon ipccode 201–212 */
    ADDON_CREATE_BLOCK_DEVICE_NODE = 201,
    ADDON_DESTROY_BLOCK_DEVICE_NODE = 202,
    ADDON_READ_PARTITION_TABLE = 203,
    ADDON_MOUNT = 204,
    ADDON_UNMOUNT = 205,
    ADDON_FORMAT_VOLUME = 206,
    ADDON_CHECK = 207,
    ADDON_REPAIR = 208,
    ADDON_SET_LABEL = 209,
    ADDON_READ_METADATA = 210,
    ADDON_MOUNT_FUSE_DEVICE = 211,
    ADDON_PARTITION = 212,

    /** 上段 addon 未列方法，自 251 起（须与 storage_daemon 侧一致）。 */
    ADDON_GET_CAPACITY = 251,
    ADDON_EJECT = 252,
    ADDON_GET_CD_STATUS = 253,
};

class IStorageDaemon : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.StorageDaemon.IStorageDaemon");

    virtual ErrCode QueryUsbIsInUse(const std::string &diskPath, bool &isInUse) = 0;

    virtual ErrCode CreateBlockDeviceNode(const std::string &devPath,
                                          uint32_t mode,
                                          int32_t major,
                                          int32_t minor) = 0;
    virtual ErrCode DestroyBlockDeviceNode(const std::string &devPath) = 0;
    virtual ErrCode ReadPartitionTable(const std::string &devPath,
                                       std::string &output,
                                       int32_t &maxVolume) = 0;
    virtual ErrCode Eject(const std::string &devPath) = 0;
    virtual ErrCode GetCDStatus(const std::string &devPath, int32_t &status) = 0;
    virtual ErrCode Mount(const std::string &devPath,
                          const std::string &mountPath,
                          const std::string &fsType,
                          const std::string &mountData) = 0;
    virtual ErrCode Unmount(const std::string &mountPath, const std::string &fsType, bool force) = 0;
    virtual ErrCode FormatVolume(const std::string &devPath, const std::string &fsType) = 0;
    virtual ErrCode Check(const std::string &devPath, const std::string &fsType, bool autoFix) = 0;
    virtual ErrCode Repair(const std::string &devPath, const std::string &fsType) = 0;
    virtual ErrCode SetLabel(const std::string &devPath,
                             const std::string &fsType,
                             const std::string &label) = 0;
    virtual ErrCode ReadMetadata(const std::string &devPath,
                                 std::string &uuid,
                                 std::string &type,
                                 std::string &label) = 0;
    virtual ErrCode GetCapacity(const std::string &mountPath, int64_t &totalSize, int64_t &freeSize) = 0;
    virtual ErrCode MountFuseDevice(const std::string &mountPath, int32_t &fuseFd) = 0;
    virtual ErrCode Partition(const std::string &diskPath, int32_t partitionType, uint32_t partitionFlags) = 0;

protected:
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD004301, "StorageDaemon"};
};

} // namespace StorageDaemon
} // namespace OHOS

#endif // OHOS_STORAGEDAEMON_ISTORAGEDAEMON_H
