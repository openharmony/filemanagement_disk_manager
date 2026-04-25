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

#include "usb_fuse_adapter.h"

#include <dlfcn.h>

#include "disk_manager_errno.h"
#include "disk_manager_hilog.h"
#include "parameters.h"

namespace OHOS {
namespace DiskManager {

namespace {
using FuncMount = int32_t (*)(int, const std::string &, const std::string &);
using FuncUMount = int32_t (*)(const std::string &);
using FuncUsbFuseByType = int32_t (*)(const std::string &, bool &);

constexpr const char *FUSE_PARAM_SERVICE_ENTERPRISE_ENABLE = "const.enterprise.external_storage_device.manage.enable";
} // namespace

UsbFuseAdapter &UsbFuseAdapter::GetInstance()
{
    static UsbFuseAdapter instance;
    return instance;
}

UsbFuseAdapter::UsbFuseAdapter()
{
    Init();
    LOGI("UsbFuseAdapter created");
}

UsbFuseAdapter::~UsbFuseAdapter()
{
    UnInit();
    LOGI("UsbFuseAdapter destroyed");
}

void UsbFuseAdapter::Init()
{
    handler_ = dlopen("/system/lib64/libspace_ability_fuse_ext.z.so", RTLD_LAZY);
    if (handler_ == nullptr) {
        LOGW("Usb fuse policy library not loaded: %{public}s", dlerror());
    }
}

void UsbFuseAdapter::UnInit()
{
    if (handler_ != nullptr) {
        dlclose(handler_);
        handler_ = nullptr;
    }
}

int32_t UsbFuseAdapter::NotifyUsbFuseMount(int fuseFd, const std::string &volumeId, const std::string &fsUuid)
{
    LOGI("NotifyUsbFuseMount enter fuseFd=%{public}d volumeId=%{public}s fsUuidLen=%{public}zu", fuseFd,
         volumeId.c_str(), fsUuid.size());
    if (handler_ == nullptr) {
        LOGE("NotifyUsbFuseMount: handler is nullptr");
        return DiskManagerErrNo::E_DAEMON_IPC_FAILED;
    }
    FuncMount funcMount = reinterpret_cast<FuncMount>(dlsym(handler_, "NotifyExternalVolumeFuseMount"));
    if (funcMount == nullptr) {
        LOGE("NotifyUsbFuseMount: dlsym NotifyExternalVolumeFuseMount failed");
        return DiskManagerErrNo::E_DAEMON_IPC_FAILED;
    }
    if (funcMount(fuseFd, volumeId, fsUuid) != DiskManagerErrNo::E_OK) {
        LOGE("NotifyUsbFuseMount: policy returned failure");
        return DiskManagerErrNo::E_DAEMON_IPC_FAILED;
    }
    return DiskManagerErrNo::E_OK;
}

int32_t UsbFuseAdapter::NotifyUsbFuseUmount(const std::string &volumeId)
{
    LOGI("NotifyUsbFuseUmount enter volumeId=%{public}s", volumeId.c_str());
    if (handler_ == nullptr) {
        LOGE("NotifyUsbFuseUmount: handler is nullptr");
        return DiskManagerErrNo::E_DAEMON_IPC_FAILED;
    }
    FuncUMount funcUMount = reinterpret_cast<FuncUMount>(dlsym(handler_, "NotifyExternalVolumeFuseUmount"));
    if (funcUMount == nullptr) {
        LOGE("NotifyUsbFuseUmount: dlsym NotifyExternalVolumeFuseUmount failed");
        return DiskManagerErrNo::E_DAEMON_IPC_FAILED;
    }
    if (funcUMount(volumeId) != DiskManagerErrNo::E_OK) {
        LOGE("NotifyUsbFuseUmount: policy returned failure");
        return DiskManagerErrNo::E_DAEMON_IPC_FAILED;
    }
    return DiskManagerErrNo::E_OK;
}

bool UsbFuseAdapter::IsUsbFuseByType(const std::string &fsType)
{
    LOGI("IsUsbFuseByType enter fsType=%{public}s", fsType.c_str());
    bool enabled = true;
    if (handler_ == nullptr) {
        LOGE("IsUsbFuseByType: handler is nullptr, default enabled=%{public}d", static_cast<int>(enabled));
        return enabled;
    }
    FuncUsbFuseByType funcUsbFuseByType = reinterpret_cast<FuncUsbFuseByType>(dlsym(handler_, "IsUsbFuseByType"));
    if (funcUsbFuseByType == nullptr) {
        LOGE("IsUsbFuseByType: dlsym failed, default enabled=%{public}d", static_cast<int>(enabled));
        return enabled;
    }
    if (funcUsbFuseByType(fsType, enabled) != DiskManagerErrNo::E_OK) {
        LOGE("IsUsbFuseByType: policy returned failure, default enabled=%{public}d", static_cast<int>(enabled));
        return enabled;
    }
    return enabled;
}

bool UsbFuseAdapter::IsUsbFuseEnabledForFsType(const std::string &fsType)
{
    LOGI("IsUsbFuseEnabledForFsType enter fsType=%{public}s", fsType.c_str());
    const bool enabledByCcm = OHOS::system::GetBoolParameter(FUSE_PARAM_SERVICE_ENTERPRISE_ENABLE, false);
    if (!enabledByCcm || fsType.empty()) {
        LOGI("IsUsbFuseEnabledForFsType enabledByCcm=%{public}d enabledByType=%{public}d fsType=%{public}s",
             static_cast<int32_t>(enabledByCcm), 0, fsType.c_str());
        return false;
    }

    const bool enabledByType = IsUsbFuseByType(fsType);
    LOGI("IsUsbFuseEnabledForFsType enabledByCcm=%{public}d enabledByType=%{public}d fsType=%{public}s",
         static_cast<int32_t>(enabledByCcm), static_cast<int32_t>(enabledByType), fsType.c_str());
    return enabledByType;
}

} // namespace DiskManager
} // namespace OHOS
