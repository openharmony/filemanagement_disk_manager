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

#include "storage_daemon_proxy.h"
#include "errors.h"
#include "message_option.h"
#include "message_parcel.h"
#include "string_ex.h"

#include "disk_manager_hilog.h"
namespace OHOS {
namespace StorageDaemon {

StorageDaemonProxy::StorageDaemonProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IStorageDaemon>(impl) {}

ErrCode StorageDaemonProxy::QueryUsbIsInUse(const std::string &diskPath, bool &isInUse)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(diskPath))) {
        return ERR_INVALID_DATA;
    }

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::COMMAND_QUERY_USB_IS_IN_USE), data,
                                        reply, option);
    if (ret != ERR_OK) {
        return ret;
    }

    int32_t result = reply.ReadInt32();
    if (result != ERR_OK) {
        return result;
    }
    isInUse = reply.ReadBool();
    return result;
}

ErrCode StorageDaemonProxy::MountUsbFuse(const std::string &volumeId, std::string &fsUuid, int &fuseFd)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(volumeId))) {
        return ERR_INVALID_DATA;
    }

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::COMMAND_MOUNT_USB_FUSE), data,
                                        reply, option);
    if (ret != ERR_OK) {
        return ret;
    }

    int32_t result = reply.ReadInt32();
    if (result != ERR_OK) {
        return result;
    }
    fsUuid = Str16ToStr8(reply.ReadString16());
    fuseFd = reply.ReadFileDescriptor();
    return result;
}

ErrCode StorageDaemonProxy::CreateBlockDeviceNode(const std::string &devPath,
                                                  uint32_t mode,
                                                  int32_t major,
                                                  int32_t minor)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(devPath)) || !data.WriteUint32(mode) || !data.WriteInt32(major) ||
        !data.WriteInt32(minor)) {
        return ERR_INVALID_DATA;
    }
    int32_t ret = Remote()->SendRequest(
        static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_CREATE_BLOCK_DEVICE_NODE), data, reply, option);
    if (ret != ERR_OK) {
        return ret;
    }
    return reply.ReadInt32();
}

ErrCode StorageDaemonProxy::DestroyBlockDeviceNode(const std::string &devPath)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(devPath))) {
        return ERR_INVALID_DATA;
    }
    int32_t ret = Remote()->SendRequest(
        static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_DESTROY_BLOCK_DEVICE_NODE), data, reply, option);
    if (ret != ERR_OK) {
        return ret;
    }
    return reply.ReadInt32();
}

ErrCode StorageDaemonProxy::ReadPartitionTable(const std::string &devPath, std::string &output, int32_t &maxVolume)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    output.clear();
    maxVolume = 0;
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(devPath))) {
        return ERR_INVALID_DATA;
    }
    int32_t ret = Remote()->SendRequest(
        static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_READ_PARTITION_TABLE), data, reply, option);
    if (ret != ERR_OK) {
        return ret;
    }
    int32_t res = reply.ReadInt32();
    if (res != ERR_OK) {
        return res;
    }
    output = Str16ToStr8(reply.ReadString16());
    maxVolume = reply.ReadInt32();
    return res;
}

ErrCode StorageDaemonProxy::ReadVolumeMetaData(const std::string &devPath,
                                               std::string &fsUuid,
                                               std::string &fsType,
                                               std::string &fsLabel)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    fsUuid.clear();
    fsType.clear();
    fsLabel.clear();
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(devPath))) {
        return ERR_INVALID_DATA;
    }
    int32_t ret = Remote()->SendRequest(
        static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_READ_VOLUME_META_DATA), data, reply, option);
    if (ret != ERR_OK) {
        return ret;
    }
    int32_t res = reply.ReadInt32();
    if (res != ERR_OK) {
        return res;
    }
    fsUuid = Str16ToStr8(reply.ReadString16());
    fsType = Str16ToStr8(reply.ReadString16());
    fsLabel = Str16ToStr8(reply.ReadString16());
    return res;
}

ErrCode StorageDaemonProxy::Eject(const std::string &devPath)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(devPath))) {
        return ERR_INVALID_DATA;
    }
    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_EJECT), data, reply, option);
    if (ret != ERR_OK) {
        return ret;
    }
    return reply.ReadInt32();
}

ErrCode StorageDaemonProxy::GetCDStatus(const std::string &devPath, int32_t &status)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    status = 0;
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(devPath))) {
        return ERR_INVALID_DATA;
    }
    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_GET_CD_STATUS), data, reply, option);
    if (ret != ERR_NONE) {
        return ret;
    }
    int32_t res = reply.ReadInt32();
    if (res != ERR_OK) {
        return res;
    }
    status = reply.ReadInt32();
    return res;
}

ErrCode StorageDaemonProxy::Mount(const std::string &devPath,
                                  const std::string &mountPath,
                                  const std::string &fsType,
                                  const std::string &mountData)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(devPath)) || !data.WriteString16(Str8ToStr16(mountPath)) ||
        !data.WriteString16(Str8ToStr16(fsType)) || !data.WriteString16(Str8ToStr16(mountData))) {
        return ERR_INVALID_DATA;
    }
    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_MOUNT), data, reply, option);
    if (ret != ERR_OK) {
        return ret;
    }
    return reply.ReadInt32();
}

ErrCode StorageDaemonProxy::Unmount(const std::string &mountPath, const std::string &fsType, bool force)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(mountPath)) || !data.WriteString16(Str8ToStr16(fsType)) ||
        !data.WriteBool(force)) {
        return ERR_INVALID_DATA;
    }
    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_UNMOUNT), data, reply, option);
    if (ret != ERR_OK) {
        return ret;
    }
    return reply.ReadInt32();
}

ErrCode StorageDaemonProxy::FormatVolume(const std::string &devPath, const std::string &fsType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(devPath)) || !data.WriteString16(Str8ToStr16(fsType))) {
        return ERR_INVALID_DATA;
    }
    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_FORMAT_VOLUME), data, reply, option);
    if (ret != ERR_OK) {
        return ret;
    }
    return reply.ReadInt32();
}

ErrCode StorageDaemonProxy::Check(const std::string &devPath, const std::string &fsType, bool autoFix)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(devPath)) || !data.WriteString16(Str8ToStr16(fsType)) ||
        !data.WriteBool(autoFix)) {
        return ERR_INVALID_DATA;
    }
    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_CHECK), data, reply, option);
    if (ret != ERR_OK) {
        return ret;
    }
    return reply.ReadInt32();
}

ErrCode StorageDaemonProxy::Repair(const std::string &devPath, const std::string &fsType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(devPath)) || !data.WriteString16(Str8ToStr16(fsType))) {
        return ERR_INVALID_DATA;
    }
    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_REPAIR), data, reply, option);
    if (ret != ERR_OK) {
        return ret;
    }
    return reply.ReadInt32();
}

ErrCode StorageDaemonProxy::SetLabel(const std::string &devPath,
                                     const std::string &fsType,
                                     const std::string &label)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(devPath)) || !data.WriteString16(Str8ToStr16(fsType)) ||
        !data.WriteString16(Str8ToStr16(label))) {
        return ERR_INVALID_DATA;
    }
    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_SET_LABEL), data, reply, option);
    if (ret != ERR_OK) {
        return ret;
    }
    return reply.ReadInt32();
}

ErrCode StorageDaemonProxy::ReadMetadata(const std::string &devPath,
                                         std::string &uuid,
                                         std::string &type,
                                         std::string &label)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    uuid.clear();
    type.clear();
    label.clear();
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(devPath))) {
        return ERR_INVALID_DATA;
    }
    int32_t ret = Remote()->SendRequest(
        static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_READ_METADATA), data, reply, option);
    if (ret != ERR_OK) {
        return ret;
    }
    int32_t res = reply.ReadInt32();
    if (res != ERR_OK) {
        return res;
    }
    uuid = Str16ToStr8(reply.ReadString16());
    type = Str16ToStr8(reply.ReadString16());
    label = Str16ToStr8(reply.ReadString16());
    return ERR_OK;
}

ErrCode StorageDaemonProxy::GetCapacity(const std::string &mountPath, int64_t &totalSize, int64_t &freeSize)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    totalSize = 0;
    freeSize = 0;
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(mountPath))) {
        return ERR_INVALID_DATA;
    }
    int32_t ret = Remote()->SendRequest(
        static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_GET_CAPACITY), data, reply, option);
    if (ret != ERR_OK) {
        return ret;
    }
    int32_t res = reply.ReadInt32();
    if (res != ERR_OK) {
        return res;
    }
    if (!reply.ReadInt64(totalSize) || !reply.ReadInt64(freeSize)) {
        return ERR_INVALID_DATA;
    }
    return ERR_OK;
}

ErrCode StorageDaemonProxy::GetOddCapacity(const std::string &volumeId, int64_t &totalSize, int64_t &freeSize)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    totalSize = 0;
    freeSize = 0;
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(volumeId))) {
        return ERR_INVALID_DATA;
    }
    int32_t ret = Remote()->SendRequest(
        static_cast<uint32_t>(IStorageDaemonIpcCode::COMMAND_GET_ODD_CAPACITY), data, reply, option);
    if (ret != ERR_OK) {
        return ret;
    }
    int32_t res = reply.ReadInt32();
    if (res != ERR_OK) {
        return res;
    }
    if (!reply.ReadInt64(totalSize) || !reply.ReadInt64(freeSize)) {
        return ERR_INVALID_DATA;
    }
    return ERR_OK;
}

ErrCode StorageDaemonProxy::OpenFuseDevice(int32_t &fuseFd)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    fuseFd = -1;
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    int32_t ret = Remote()->SendRequest(
        static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_OPEN_FUSE_DEVICE), data, reply, option);
    if (ret != ERR_OK) {
        return ret;
    }
    int32_t res = reply.ReadInt32();
    if (res != ERR_OK) {
        return res;
    }
    fuseFd = static_cast<int32_t>(reply.ReadFileDescriptor());
    return ERR_OK;
}

ErrCode StorageDaemonProxy::MountFuseDevice(int32_t fuseFd,
                                            const std::string &mountPath,
                                            const std::string &fsUuid,
                                            const std::string &options)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteFileDescriptor(fuseFd) || !data.WriteString16(Str8ToStr16(mountPath)) ||
        !data.WriteString16(Str8ToStr16(fsUuid)) || !data.WriteString16(Str8ToStr16(options))) {
        return ERR_INVALID_DATA;
    }
    int32_t ret = Remote()->SendRequest(
        static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_MOUNT_FUSE_DEVICE), data, reply, option);
    if (ret != ERR_OK) {
        return ret;
    }
    return reply.ReadInt32();
}

ErrCode StorageDaemonProxy::Partition(const std::string &diskPath, int32_t partitionType, uint32_t partitionFlags)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(diskPath)) || !data.WriteInt32(partitionType) ||
        !data.WriteUint32(partitionFlags)) {
        return ERR_INVALID_DATA;
    }
    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_PARTITION), data, reply, option);
    if (ret != ERR_OK) {
        return ret;
    }
    return reply.ReadInt32();
}

ErrCode StorageDaemonProxy::RemoveMountPath(const std::string &mountPath)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        LOGE("StorageDaemonProxy::RemoveMountPath - Failed to write interface token");
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(mountPath))) {
        LOGE("StorageDaemonProxy::RemoveMountPath - Failed to write mountPath");
        return ERR_INVALID_DATA;
    }

    sptr<IRemoteObject> remote = Remote();
    if (!remote) {
        LOGE("StorageDaemonProxy::RemoveMountPath - Remote is nullptr!");
        return ERR_INVALID_DATA;
    }

    int32_t ret = remote->SendRequest(
        static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_REMOVE_MOUNT_PATH), data, reply, option);
    if (ret != ERR_OK) {
        LOGE("StorageDaemonProxy::RemoveMountPath - SendRequest failed, ret: %{public}d", ret);
        return ret;
    }

    ErrCode errCode = reply.ReadInt32();
    if (errCode != ERR_OK) {
        LOGE("StorageDaemonProxy::RemoveMountPath - Remove mount path failed, errCode: %{public}d", errCode);
        return errCode;
    }

    LOGI("StorageDaemonProxy::RemoveMountPath - Success, mountPath: %{public}s", mountPath.c_str());
    return ERR_OK;
}

ErrCode StorageDaemonProxy::EnsureMountPath(const std::string &mountPath)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        LOGE("StorageDaemonProxy::EnsureMountPath - Failed to write interface token");
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(mountPath))) {
        LOGE("StorageDaemonProxy::EnsureMountPath - Failed to write mountPath");
        return ERR_INVALID_DATA;
    }

    sptr<IRemoteObject> remote = Remote();
    if (!remote) {
        LOGE("StorageDaemonProxy::EnsureMountPath - Remote is nullptr!");
        return ERR_INVALID_DATA;
    }

    int32_t ret = remote->SendRequest(
        static_cast<uint32_t>(IStorageDaemonIpcCode::ADDON_ENSURE_MOUNT_PATH), data, reply, option);
    if (ret != ERR_OK) {
        LOGE("StorageDaemonProxy::EnsureMountPath - SendRequest failed, ret: %{public}d", ret);
        return ret;
    }

    ErrCode errCode = reply.ReadInt32();
    if (errCode != ERR_OK) {
        LOGE("StorageDaemonProxy::EnsureMountPath - Remove mount path failed, errCode: %{public}d", errCode);
        return errCode;
    }

    LOGI("StorageDaemonProxy::EnsureMountPath - Success, mountPath: %{public}s", mountPath.c_str());
    return ERR_OK;
}
} // namespace StorageDaemon
} // namespace OHOS
