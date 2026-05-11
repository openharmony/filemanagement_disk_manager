/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HMDFS_FILE_MANAGER
#include "napi_module_dfs_service.h"
#endif
#include "disk_manager_napi_utils.h"
#include "volumemanager_n_exporter.h"

namespace OHOS {
namespace DiskManager {
namespace ModuleVolumeManager {
/***********************************************
 * Module export and register
 ***********************************************/
napi_value VolumeManagerExport(napi_env env, napi_value exports)
{
    static napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("getAllVolumes", GetAllVolumes),
        DECLARE_NAPI_FUNCTION("mount", Mount),
        DECLARE_NAPI_FUNCTION("unmount", Unmount),
        DECLARE_NAPI_FUNCTION("getVolumeByUuid", GetVolumeByUuid),
        DECLARE_NAPI_FUNCTION("getVolumeById", GetVolumeById),
        DECLARE_NAPI_FUNCTION("setVolumeDescription", SetVolumeDescription),
        DECLARE_NAPI_FUNCTION("format", Format),
        DECLARE_NAPI_FUNCTION("partition", Partition),
        DECLARE_NAPI_FUNCTION("getAllDisks", GetAllDisks),
        DECLARE_NAPI_FUNCTION("getDiskById", GetDiskById),
        DECLARE_NAPI_FUNCTION("erase", Erase),
        DECLARE_NAPI_FUNCTION("eject", Eject),
        DECLARE_NAPI_FUNCTION("createIsoImage", CreateIsoImage),
        DECLARE_NAPI_FUNCTION("burn", Burn),
        DECLARE_NAPI_FUNCTION("getOpProcess", GetOpProcess),
        /* 与 storage_service 既有命名兼容（见 kits_impl/volumemanager_napi.cpp） */
        DECLARE_NAPI_FUNCTION("getOpticalDriveOpsProgress", GetOpProcess),
        DECLARE_NAPI_FUNCTION("verifyBurnData", VerifyBurnData),
#ifdef HMDFS_FILE_MANAGER
        DECLARE_NAPI_FUNCTION("isSameAccountDevice", DfsService::IsSameAccountDevice),
        DECLARE_NAPI_FUNCTION("getDfsSwitchStatus", DfsService::GetDfsSwitchStatus),
        DECLARE_NAPI_FUNCTION("updateDfsSwitchStatus", DfsService::UpdateDfsSwitchStatus),
        DECLARE_NAPI_FUNCTION("connectDfs", DfsService::ConnectDfs),
        DECLARE_NAPI_FUNCTION("disconnectDfs", DfsService::DisconnectDfs),
        DECLARE_NAPI_FUNCTION("getConnectedDeviceList", DfsService::GetConnectedDeviceList),
        DECLARE_NAPI_FUNCTION("on", DfsService::DeviceOnline),
        DECLARE_NAPI_FUNCTION("off", DfsService::DeviceOffline),
#endif
    };
    FILEMGMT_CALL(napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    napi_value verifyTypeEnum = nullptr;
    FILEMGMT_CALL(napi_create_object(env, &verifyTypeEnum));
    napi_value keyData = nullptr;
    napi_value fullData = nullptr;
    FILEMGMT_CALL(napi_create_int32(env, 0, &keyData));
    FILEMGMT_CALL(napi_create_int32(env, 1, &fullData));
    FILEMGMT_CALL(napi_set_named_property(env, verifyTypeEnum, "KEY_DATA", keyData));
    FILEMGMT_CALL(napi_set_named_property(env, verifyTypeEnum, "FULL_DATA", fullData));
    FILEMGMT_CALL(napi_set_named_property(env, exports, "VerifyType", verifyTypeEnum));
    return exports;
}

static napi_module _module = {.nm_version = 1,
                              .nm_flags = 0,
                              .nm_filename = nullptr,
                              .nm_register_func = VolumeManagerExport,
                              .nm_modname = "file.volumeManager",
                              .nm_priv = ((void *)0),
                              .reserved = {0}};

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&_module);
}
} // namespace ModuleVolumeManager
} // namespace DiskManager
} // namespace OHOS
