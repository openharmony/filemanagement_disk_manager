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

/*
 * 说明：storage_service 中 IsSystemApp / Convert2JsErrNum 位于 storage_manager_connect.cpp；本仓单独编译以便
 * innerkits（DiskManagerClient）与 NAPI 层解耦，避免静态库依赖 file_api / napi 头。
 */

#include "disk_manager_napi_utils.h"

#include <unordered_map>

#include "accesstoken_kit.h"
#include "disk_manager_napi_errno.h"
#include "ipc_skeleton.h"

namespace OHOS {
namespace DiskManager {

bool IsSystemApp()
{
    uint64_t fullTokenId = OHOS::IPCSkeleton::GetCallingFullTokenID();
    return Security::AccessToken::AccessTokenKit::IsSystemAppByFullTokenID(fullTokenId);
}

int32_t Convert2JsErrNum(int32_t errNum)
{
    static const std::unordered_map<int32_t, int32_t> errCodeTable {
        { E_PERMISSION_DENIED, E_PERMISSION },
        { E_WRITE_DESCRIPTOR_ERR, E_IPCSS },
        { E_NON_EXIST, E_NOOBJECT },
        { E_PREPARE_DIR, E_PREPARE },
        { E_DESTROY_DIR, E_DELETE },
        { E_VOL_MOUNT_ERR, E_MOUNT_ERR },
        { E_VOL_UMOUNT_ERR, E_UNMOUNT },
        { E_SET_POLICY, E_SET_POLICY },
        { E_USERID_RANGE, E_OUTOFRANGE },
        { E_VOL_STATE, E_VOLUMESTATE },
        { E_UMOUNT_BUSY, E_VOLUMESTATE },
        { E_NOT_SUPPORT, E_SUPPORTEDFS },
        { E_SYS_KERNEL_ERR, E_UNMOUNT },
        { E_NO_CHILD, E_NO_CHILD },
        { E_WRITE_PARCEL_ERR, E_IPCSS },
        { E_WRITE_REPLY_ERR, E_IPCSS },
        { E_SA_IS_NULLPTR, E_IPCSS },
        { E_REMOTE_IS_NULLPTR, E_IPCSS },
        { E_SERVICE_IS_NULLPTR, E_IPCSS },
        { E_BUNDLEMGR_ERROR, E_IPCSS },
        { E_MEDIALIBRARY_ERROR, E_IPCSS },
        { E_DEATH_RECIPIENT_IS_NULLPTR, E_IPCSS },
        { E_PARAMS_INVALID, E_JS_PARAMS_INVALID },
        { E_SYS_APP_PERMISSION_DENIED, E_PERMISSION_SYS },
        { E_SET_EXT_BUNDLE_STATS_ERROR, E_JS_SET_EXT_BUNDLE_STATS_ERROR },
        { E_GET_EXT_BUNDLE_STATS_ERROR, E_JS_GET_EXT_BUNDLE_STATS_ERROR },
        { E_GET_ALL_EXT_BUNDLE_STATS_ERROR, E_JS_GET_ALL_EXT_BUNDLE_STATS_ERROR },
        { E_GET_SYSTEM_DATA_SIZE_ERROR, E_JS_GET_SYSTEM_DATA_SIZE_ERROR },
        { E_GET_INODE_ERROR, E_JS_GET_INODE_ERROR },
        { E_GET_BUNDLE_INODES_ERROR, E_JS_GET_BUNDLE_INODES_ERROR },
    };

    auto it = errCodeTable.find(errNum);
    if (it != errCodeTable.end()) {
        return it->second;
    }
    return errNum;
}

} // namespace DiskManager
} // namespace OHOS
