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
    /** 旧版 volId 系 COMMAND(2–7, 51) 已废弃；disk_manager 仅保留下列与 IStorageDaemon.idl 对齐的码。 */
    COMMAND_QUERY_USB_IS_IN_USE = 45,
    COMMAND_MOUNT_USB_FUSE = 54,
    /* IStorageDaemon_addon.idl — OHOS.StorageService.IStorageDaemon，ipccode 201–217 */
    ADDON_CREATE_BLOCK_DEVICE_NODE = 201,
    ADDON_DESTROY_BLOCK_DEVICE_NODE = 202,
    ADDON_READ_PARTITION_TABLE = 203,
    ADDON_READ_VOLUME_META_DATA = 204,
    ADDON_EJECT = 205,
    ADDON_GET_CD_STATUS = 206,
    ADDON_MOUNT = 207,
    ADDON_UNMOUNT = 208,
    ADDON_FORMAT_VOLUME = 209,
    ADDON_CHECK = 210,
    ADDON_REPAIR = 211,
    ADDON_SET_LABEL = 212,
    ADDON_READ_METADATA = 213,
    ADDON_GET_CAPACITY = 214,
    ADDON_OPEN_FUSE_DEVICE = 215,
    ADDON_MOUNT_FUSE_DEVICE = 216,
    ADDON_PARTITION = 217,
};

class IStorageDaemon : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.StorageDaemon.IStorageDaemon");

    virtual ErrCode QueryUsbIsInUse(const std::string &diskPath, bool &isInUse) = 0;
    virtual ErrCode MountUsbFuse(const std::string &volumeId, std::string &fsUuid, int &fuseFd) = 0;

    virtual ErrCode CreateBlockDeviceNode(const std::string &devPath,
                                          uint32_t mode,
                                          int32_t major,
                                          int32_t minor) = 0;
    virtual ErrCode DestroyBlockDeviceNode(const std::string &devPath) = 0;
    virtual ErrCode ReadPartitionTable(const std::string &devPath,
                                       std::string &output,
                                       int32_t &maxVolume) = 0;
    virtual ErrCode ReadVolumeMetaData(const std::string &devPath,
                                       std::string &fsUuid,
                                       std::string &fsType,
                                       std::string &fsLabel) = 0;
    virtual ErrCode Eject(const std::string &devPath) = 0;
    virtual ErrCode GetCDStatus(const std::string &devPath, int32_t &status) = 0;
    virtual ErrCode Mount(const std::string &devPath,
                          const std::string &mountPath,
                          const std::string &fsType,
                          const std::string &mountData) = 0;
    virtual ErrCode Unmount(const std::string &mountPath, bool force) = 0;
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
    virtual ErrCode OpenFuseDevice(int32_t &fuseFd) = 0;
    virtual ErrCode MountFuseDevice(int32_t fuseFd,
                                    const std::string &mountPath,
                                    const std::string &fsUuid,
                                    const std::string &options) = 0;
    virtual ErrCode Partition(const std::string &diskPath, int32_t partitionType, uint32_t partitionFlags) = 0;

protected:
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0xD004301, "StorageDaemon"};
};

} // namespace StorageDaemon
} // namespace OHOS

#endif // OHOS_STORAGEDAEMON_ISTORAGEDAEMON_H
