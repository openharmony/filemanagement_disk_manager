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
#include "message_option.h"
#include "message_parcel.h"

namespace OHOS {
namespace StorageDaemon {

StorageDaemonProxy::StorageDaemonProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IStorageDaemon>(impl) {}

ErrCode StorageDaemonProxy::Mount(const std::string &volId, uint32_t flags)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString16(Str8ToStr16(volId))) {
        return ERR_INVALID_DATA;
    }
    if (!data.WriteUint32(flags)) {
        return ERR_INVALID_DATA;
    }
    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::COMMAND_MOUNT), data, reply, option);
    if (ret != ERR_NONE) {
        return ret;
    }
    return reply.ReadInt32();
}

ErrCode StorageDaemonProxy::UMount(const std::string &volId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString(volId)) {
        return ERR_INVALID_DATA;
    }

    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::COMMAND_UMOUNT), data, reply, option);
    if (ret != ERR_NONE) {
        return ret;
    }
    return reply.ReadInt32();
}

ErrCode StorageDaemonProxy::Check(const std::string &volId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString(volId)) {
        return ERR_INVALID_DATA;
    }

    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::COMMAND_CHECK), data, reply, option);
    if (ret != ERR_NONE) {
        return ret;
    }
    return reply.ReadInt32();
}

ErrCode StorageDaemonProxy::Format(const std::string &volId, const std::string &fsType)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString(volId)) {
        return ERR_INVALID_DATA;
    }
    if (!data.WriteString(fsType)) {
        return ERR_INVALID_DATA;
    }

    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::COMMAND_FORMAT), data, reply, option);
    if (ret != ERR_NONE) {
        return ret;
    }
    return reply.ReadInt32();
}

ErrCode StorageDaemonProxy::Partition(const std::string &diskId, int32_t type)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString(diskId)) {
        return ERR_INVALID_DATA;
    }
    if (!data.WriteInt32(type)) {
        return ERR_INVALID_DATA;
    }

    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::COMMAND_PARTITION), data, reply, option);
    if (ret != ERR_NONE) {
        return ret;
    }
    return reply.ReadInt32();
}

ErrCode StorageDaemonProxy::SetVolumeDescription(const std::string &volId, const std::string &description)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString(volId)) {
        return ERR_INVALID_DATA;
    }
    if (!data.WriteString(description)) {
        return ERR_INVALID_DATA;
    }

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::COMMAND_SET_VOLUME_DESCRIPTION),
                                        data, reply, option);
    if (ret != ERR_NONE) {
        return ret;
    }
    return reply.ReadInt32();
}

ErrCode StorageDaemonProxy::QueryUsbIsInUse(const std::string &diskPath, bool &isInUse)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString(diskPath)) {
        return ERR_INVALID_DATA;
    }

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::COMMAND_QUERY_USB_IS_IN_USE), data,
                                        reply, option);
    if (ret != ERR_NONE) {
        return ret;
    }

    int32_t result = reply.ReadInt32();
    if (result == ERR_NONE) {
        isInUse = reply.ReadBool();
    }
    return result;
}

ErrCode StorageDaemonProxy::TryToFix(const std::string &volId, uint32_t flags)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString(volId)) {
        return ERR_INVALID_DATA;
    }
    if (!data.WriteUint32(flags)) {
        return ERR_INVALID_DATA;
    }

    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::COMMAND_TRY_TO_FIX), data, reply, option);
    if (ret != ERR_NONE) {
        return ret;
    }
    return reply.ReadInt32();
}

ErrCode StorageDaemonProxy::MountUsbFuse(const std::string &volumeId, std::string &fsUuid, int &fuseFd)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(IStorageDaemon::GetDescriptor())) {
        return ERR_TRANSACTION_FAILED;
    }
    if (!data.WriteString(volumeId)) {
        return ERR_INVALID_DATA;
    }

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(IStorageDaemonIpcCode::COMMAND_MOUNT_USB_FUSE), data,
                                        reply, option);
    if (ret != ERR_NONE) {
        return ret;
    }

    int32_t result = reply.ReadInt32();
    if (result == ERR_NONE) {
        fsUuid = reply.ReadString();
        fuseFd = reply.ReadFileDescriptor();
    }
    return result;
}

} // namespace StorageDaemon
} // namespace OHOS
