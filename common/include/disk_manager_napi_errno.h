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

#ifndef DISK_MANAGER_NAPI_ERRNO_H
#define DISK_MANAGER_NAPI_ERRNO_H

#include <cstdint>

namespace OHOS {

constexpr int32_t STORAGE_SERVICE_SYS_CAP_TAG = 13600000;

/** 与 storage 侧 native 返回值对齐，仅保留当前 volume/encrypted NAPI、DFS、Taihe 映射所需项 */
enum class DiskManagerNativeErr : int32_t {
    E_OK = 0,

    E_PERMISSION_DENIED = STORAGE_SERVICE_SYS_CAP_TAG + 1,
    E_PARAMS_INVALID = STORAGE_SERVICE_SYS_CAP_TAG + 2,
    E_NON_EXIST = STORAGE_SERVICE_SYS_CAP_TAG + 4,
    E_PREPARE_DIR = STORAGE_SERVICE_SYS_CAP_TAG + 5,
    E_DESTROY_DIR = STORAGE_SERVICE_SYS_CAP_TAG + 6,
    E_NOT_SUPPORT = STORAGE_SERVICE_SYS_CAP_TAG + 7,
    E_SYS_KERNEL_ERR = STORAGE_SERVICE_SYS_CAP_TAG + 8,
    E_WRITE_PARCEL_ERR = STORAGE_SERVICE_SYS_CAP_TAG + 9,
    E_WRITE_REPLY_ERR = STORAGE_SERVICE_SYS_CAP_TAG + 10,
    E_WRITE_DESCRIPTOR_ERR = STORAGE_SERVICE_SYS_CAP_TAG + 11,
    E_SA_IS_NULLPTR = STORAGE_SERVICE_SYS_CAP_TAG + 12,
    E_REMOTE_IS_NULLPTR = STORAGE_SERVICE_SYS_CAP_TAG + 13,
    E_SERVICE_IS_NULLPTR = STORAGE_SERVICE_SYS_CAP_TAG + 14,
    E_DEATH_RECIPIENT_IS_NULLPTR = STORAGE_SERVICE_SYS_CAP_TAG + 16,
    E_SYS_APP_PERMISSION_DENIED = STORAGE_SERVICE_SYS_CAP_TAG + 25,

    E_SET_POLICY = STORAGE_SERVICE_SYS_CAP_TAG + 201,
    E_USERID_RANGE = STORAGE_SERVICE_SYS_CAP_TAG + 202,

    E_BUNDLEMGR_ERROR = STORAGE_SERVICE_SYS_CAP_TAG + 1201,
    E_MEDIALIBRARY_ERROR = STORAGE_SERVICE_SYS_CAP_TAG + 1202,
    E_GET_EXT_BUNDLE_STATS_ERROR = STORAGE_SERVICE_SYS_CAP_TAG + 1223,
    E_SET_EXT_BUNDLE_STATS_ERROR = STORAGE_SERVICE_SYS_CAP_TAG + 1224,
    E_GET_ALL_EXT_BUNDLE_STATS_ERROR = STORAGE_SERVICE_SYS_CAP_TAG + 1226,
    E_GET_INODE_ERROR = STORAGE_SERVICE_SYS_CAP_TAG + 1229,
    E_GET_SYSTEM_DATA_SIZE_ERROR = STORAGE_SERVICE_SYS_CAP_TAG + 1232,
    E_GET_BUNDLE_INODES_ERROR = STORAGE_SERVICE_SYS_CAP_TAG + 1233,

    E_VOL_STATE = STORAGE_SERVICE_SYS_CAP_TAG + 1701,
    E_VOL_MOUNT_ERR = STORAGE_SERVICE_SYS_CAP_TAG + 1702,
    E_VOL_UMOUNT_ERR = STORAGE_SERVICE_SYS_CAP_TAG + 1703,
    E_UMOUNT_BUSY = STORAGE_SERVICE_SYS_CAP_TAG + 1704,
    E_NO_CHILD = STORAGE_SERVICE_SYS_CAP_TAG + 1705,
};

/** JS / Taihe 业务错误码（与 storage_service 原 JsErrCode 数值一致） */
enum class DiskManagerJsErr : int32_t {
    E_PERMISSION = 201,
    E_PERMISSION_SYS = 202,
    E_PARAMS = 401,
    E_DEVICENOTSUPPORT = 801,
    E_OSNOTSUPPORT = 901,
    E_IPCSS = STORAGE_SERVICE_SYS_CAP_TAG + 1,
    E_SUPPORTEDFS = STORAGE_SERVICE_SYS_CAP_TAG + 2,
    E_MOUNT_ERR = STORAGE_SERVICE_SYS_CAP_TAG + 3,
    E_UNMOUNT = STORAGE_SERVICE_SYS_CAP_TAG + 4,
    E_VOLUMESTATE = STORAGE_SERVICE_SYS_CAP_TAG + 5,
    E_PREPARE = STORAGE_SERVICE_SYS_CAP_TAG + 6,
    E_DELETE = STORAGE_SERVICE_SYS_CAP_TAG + 7,
    E_NOOBJECT = STORAGE_SERVICE_SYS_CAP_TAG + 8,
    E_OUTOFRANGE = STORAGE_SERVICE_SYS_CAP_TAG + 9,
    E_JS_PARAMS_INVALID = STORAGE_SERVICE_SYS_CAP_TAG + 10,
    E_JS_SET_EXT_BUNDLE_STATS_ERROR = STORAGE_SERVICE_SYS_CAP_TAG + 11,
    E_JS_GET_EXT_BUNDLE_STATS_ERROR = STORAGE_SERVICE_SYS_CAP_TAG + 12,
    E_JS_GET_ALL_EXT_BUNDLE_STATS_ERROR = STORAGE_SERVICE_SYS_CAP_TAG + 13,
    E_JS_RETRY_ERROR = STORAGE_SERVICE_SYS_CAP_TAG + 14,
    E_JS_GET_INODE_ERROR = STORAGE_SERVICE_SYS_CAP_TAG + 16,
    E_JS_GET_BUNDLE_INODES_ERROR = STORAGE_SERVICE_SYS_CAP_TAG + 17,
    E_JS_GET_SYSTEM_DATA_SIZE_ERROR = STORAGE_SERVICE_SYS_CAP_TAG + 18,
};

constexpr int32_t E_OK = static_cast<int32_t>(DiskManagerNativeErr::E_OK);
constexpr int32_t E_PERMISSION_DENIED = static_cast<int32_t>(DiskManagerNativeErr::E_PERMISSION_DENIED);
constexpr int32_t E_PARAMS_INVALID = static_cast<int32_t>(DiskManagerNativeErr::E_PARAMS_INVALID);
constexpr int32_t E_NON_EXIST = static_cast<int32_t>(DiskManagerNativeErr::E_NON_EXIST);
constexpr int32_t E_PREPARE_DIR = static_cast<int32_t>(DiskManagerNativeErr::E_PREPARE_DIR);
constexpr int32_t E_DESTROY_DIR = static_cast<int32_t>(DiskManagerNativeErr::E_DESTROY_DIR);
constexpr int32_t E_NOT_SUPPORT = static_cast<int32_t>(DiskManagerNativeErr::E_NOT_SUPPORT);
constexpr int32_t E_SYS_KERNEL_ERR = static_cast<int32_t>(DiskManagerNativeErr::E_SYS_KERNEL_ERR);
constexpr int32_t E_WRITE_PARCEL_ERR = static_cast<int32_t>(DiskManagerNativeErr::E_WRITE_PARCEL_ERR);
constexpr int32_t E_WRITE_REPLY_ERR = static_cast<int32_t>(DiskManagerNativeErr::E_WRITE_REPLY_ERR);
constexpr int32_t E_WRITE_DESCRIPTOR_ERR = static_cast<int32_t>(DiskManagerNativeErr::E_WRITE_DESCRIPTOR_ERR);
constexpr int32_t E_SA_IS_NULLPTR = static_cast<int32_t>(DiskManagerNativeErr::E_SA_IS_NULLPTR);
constexpr int32_t E_REMOTE_IS_NULLPTR = static_cast<int32_t>(DiskManagerNativeErr::E_REMOTE_IS_NULLPTR);
constexpr int32_t E_SERVICE_IS_NULLPTR = static_cast<int32_t>(DiskManagerNativeErr::E_SERVICE_IS_NULLPTR);
constexpr int32_t E_DEATH_RECIPIENT_IS_NULLPTR =
    static_cast<int32_t>(DiskManagerNativeErr::E_DEATH_RECIPIENT_IS_NULLPTR);
constexpr int32_t E_SYS_APP_PERMISSION_DENIED = static_cast<int32_t>(DiskManagerNativeErr::E_SYS_APP_PERMISSION_DENIED);
constexpr int32_t E_SET_POLICY = static_cast<int32_t>(DiskManagerNativeErr::E_SET_POLICY);
constexpr int32_t E_USERID_RANGE = static_cast<int32_t>(DiskManagerNativeErr::E_USERID_RANGE);
constexpr int32_t E_BUNDLEMGR_ERROR = static_cast<int32_t>(DiskManagerNativeErr::E_BUNDLEMGR_ERROR);
constexpr int32_t E_MEDIALIBRARY_ERROR = static_cast<int32_t>(DiskManagerNativeErr::E_MEDIALIBRARY_ERROR);
constexpr int32_t E_GET_EXT_BUNDLE_STATS_ERROR =
    static_cast<int32_t>(DiskManagerNativeErr::E_GET_EXT_BUNDLE_STATS_ERROR);
constexpr int32_t E_SET_EXT_BUNDLE_STATS_ERROR =
    static_cast<int32_t>(DiskManagerNativeErr::E_SET_EXT_BUNDLE_STATS_ERROR);
constexpr int32_t E_GET_ALL_EXT_BUNDLE_STATS_ERROR =
    static_cast<int32_t>(DiskManagerNativeErr::E_GET_ALL_EXT_BUNDLE_STATS_ERROR);
constexpr int32_t E_GET_INODE_ERROR = static_cast<int32_t>(DiskManagerNativeErr::E_GET_INODE_ERROR);
constexpr int32_t E_GET_SYSTEM_DATA_SIZE_ERROR =
    static_cast<int32_t>(DiskManagerNativeErr::E_GET_SYSTEM_DATA_SIZE_ERROR);
constexpr int32_t E_GET_BUNDLE_INODES_ERROR = static_cast<int32_t>(DiskManagerNativeErr::E_GET_BUNDLE_INODES_ERROR);
constexpr int32_t E_VOL_STATE = static_cast<int32_t>(DiskManagerNativeErr::E_VOL_STATE);
constexpr int32_t E_VOL_MOUNT_ERR = static_cast<int32_t>(DiskManagerNativeErr::E_VOL_MOUNT_ERR);
constexpr int32_t E_VOL_UMOUNT_ERR = static_cast<int32_t>(DiskManagerNativeErr::E_VOL_UMOUNT_ERR);
constexpr int32_t E_UMOUNT_BUSY = static_cast<int32_t>(DiskManagerNativeErr::E_UMOUNT_BUSY);
constexpr int32_t E_NO_CHILD = static_cast<int32_t>(DiskManagerNativeErr::E_NO_CHILD);

constexpr int32_t E_PERMISSION = static_cast<int32_t>(DiskManagerJsErr::E_PERMISSION);
constexpr int32_t E_PERMISSION_SYS = static_cast<int32_t>(DiskManagerJsErr::E_PERMISSION_SYS);
constexpr int32_t E_PARAMS = static_cast<int32_t>(DiskManagerJsErr::E_PARAMS);
constexpr int32_t E_DEVICENOTSUPPORT = static_cast<int32_t>(DiskManagerJsErr::E_DEVICENOTSUPPORT);
constexpr int32_t E_OSNOTSUPPORT = static_cast<int32_t>(DiskManagerJsErr::E_OSNOTSUPPORT);
constexpr int32_t E_IPCSS = static_cast<int32_t>(DiskManagerJsErr::E_IPCSS);
constexpr int32_t E_SUPPORTEDFS = static_cast<int32_t>(DiskManagerJsErr::E_SUPPORTEDFS);
constexpr int32_t E_MOUNT_ERR = static_cast<int32_t>(DiskManagerJsErr::E_MOUNT_ERR);
constexpr int32_t E_UNMOUNT = static_cast<int32_t>(DiskManagerJsErr::E_UNMOUNT);
constexpr int32_t E_VOLUMESTATE = static_cast<int32_t>(DiskManagerJsErr::E_VOLUMESTATE);
constexpr int32_t E_PREPARE = static_cast<int32_t>(DiskManagerJsErr::E_PREPARE);
constexpr int32_t E_DELETE = static_cast<int32_t>(DiskManagerJsErr::E_DELETE);
constexpr int32_t E_NOOBJECT = static_cast<int32_t>(DiskManagerJsErr::E_NOOBJECT);
constexpr int32_t E_OUTOFRANGE = static_cast<int32_t>(DiskManagerJsErr::E_OUTOFRANGE);
constexpr int32_t E_JS_PARAMS_INVALID = static_cast<int32_t>(DiskManagerJsErr::E_JS_PARAMS_INVALID);
constexpr int32_t E_JS_SET_EXT_BUNDLE_STATS_ERROR =
    static_cast<int32_t>(DiskManagerJsErr::E_JS_SET_EXT_BUNDLE_STATS_ERROR);
constexpr int32_t E_JS_GET_EXT_BUNDLE_STATS_ERROR =
    static_cast<int32_t>(DiskManagerJsErr::E_JS_GET_EXT_BUNDLE_STATS_ERROR);
constexpr int32_t E_JS_GET_ALL_EXT_BUNDLE_STATS_ERROR =
    static_cast<int32_t>(DiskManagerJsErr::E_JS_GET_ALL_EXT_BUNDLE_STATS_ERROR);
constexpr int32_t E_JS_RETRY_ERROR = static_cast<int32_t>(DiskManagerJsErr::E_JS_RETRY_ERROR);
constexpr int32_t E_JS_GET_INODE_ERROR = static_cast<int32_t>(DiskManagerJsErr::E_JS_GET_INODE_ERROR);
constexpr int32_t E_JS_GET_BUNDLE_INODES_ERROR = static_cast<int32_t>(DiskManagerJsErr::E_JS_GET_BUNDLE_INODES_ERROR);
constexpr int32_t E_JS_GET_SYSTEM_DATA_SIZE_ERROR =
    static_cast<int32_t>(DiskManagerJsErr::E_JS_GET_SYSTEM_DATA_SIZE_ERROR);

} // namespace OHOS

#endif // DISK_MANAGER_NAPI_ERRNO_H
