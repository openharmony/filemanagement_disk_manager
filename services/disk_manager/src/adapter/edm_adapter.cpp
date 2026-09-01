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
#include "edm_adapter.h"

#include "parameters.h"

#include <cerrno>
#include <cstdlib>

#include "disk_manager.h"
#include "disk_manager_errno.h"
#include "disk_manager_hilog.h"
#include "disk_manager_utils.h"

namespace OHOS {
namespace DiskManager {

namespace {

bool ConvertStringToInt(const std::string &str, int32_t &value)
{
    if (str.empty()) {
        return false;
    }
    errno = 0;
    char *endptr = nullptr;
    long result = std::strtol(str.c_str(), &endptr, 16);
    if (endptr == str.c_str() || *endptr != '\0' || errno == ERANGE) {
        return false;
    }
    if (result < INT32_MIN || result > INT32_MAX) {
        return false;
    }
    value = static_cast<int32_t>(result);
    return true;
}

/**
 * MDM精细化管控：读取系统参数判断当前设备是否为2B企业设备。
 * @return true=企业设备, false=非企业设备
 */
bool IsEnterpriseDevice()
{
    return OHOS::system::GetParameter("persist.edm.device_type", "") == "enterprise";
}

/**
 * MDM精细化管控：读取persist.edm.enable_external_storage_mount_intercept判断外置存储挂载拦截是否使能。
 * @return true=拦截已开启, false=未开启
 */
bool __attribute__((unused)) IsInterceptEnabled()
{
    bool result = IsEnterpriseDevice() &&
        OHOS::system::GetParameter("persist.edm.enable_external_storage_mount_intercept", "") == "true";
    LOGI("IsInterceptEnabled: result=%{public}s", result ? "true" : "false");
    return result;
}

/**
 * MDM精细化管控：读取persist.edm.disable_sata_odd_burn判断内置光驱刻录是否被禁止。
 * @return true=禁止刻录, false=允许刻录
 */
bool IsSataOddBurnDisabled()
{
    return IsEnterpriseDevice() &&
        OHOS::system::GetParameter("persist.edm.disable_sata_odd_burn", "") == "true";
}

/**
 * MDM精细化管控：仅SD_FLAG/USB_FLAG/CD_FLAG类型受管控。
 * @param diskType 磁盘类型
 * @return true=受管控, false=不受管控
 */
bool __attribute__((unused)) IsEdmManagedDiskType(int32_t diskType)
{
    return diskType == DiskType::SD_FLAG || diskType == DiskType::USB_FLAG || diskType == DiskType::CD_FLAG;
}
} // namespace

EdmAdapter &EdmAdapter::GetInstance()
{
    static EdmAdapter instance;
    return instance;
}

EdmAdapter::EdmAdapter() {}

EdmAdapter::~EdmAdapter() {}

bool EdmAdapter::IsEdmEnableOddBurn(const std::string &diskId, int32_t callerUserId)
{
    LOGI("IsEdmEnableOddBurn enter, diskId=%{public}s callerUserId=%{public}d", diskId.c_str(), callerUserId);
    if (!IsEnterpriseDevice()) {
        LOGI("IsEdmEnableOddBurn not enterprise device, diskId=%{public}s", diskId.c_str());
        return true;
    }
    Disk disk;
    if (DiskManager::GetInstance().GetDiskById(diskId, disk) != E_OK) {
        LOGE("IsEdmEnableOddBurn disk not found, diskId=%{public}s", diskId.c_str());
        return true;
    }
    if (disk.GetDiskType() != DiskType::CD_FLAG) {
        LOGI("IsEdmEnableOddBurn not Odd, diskType=%{public}d diskId=%{public}s", disk.GetDiskType(), diskId.c_str());
        return true;
    }
    const std::string &sysPath = disk.GetSysPath();
    // 内置SATA光驱刻录禁用：sysPath含"/ata/"或".sata/"
    bool sataDisabled = IsSataOddBurnDisabled();
    bool hasAta = sysPath.find("/ata/") != std::string::npos;
    bool hasSata = sysPath.find(".sata/") != std::string::npos;
    bool isSataOdd = hasAta || hasSata;
    LOGI("IsEdmEnableOddBurn disk found, diskType=%{public}d sysPath=%{public}s "
         "sataDisabled=%{public}d isSataOdd=%{public}d",
         disk.GetDiskType(), sysPath.c_str(), sataDisabled, isSataOdd);
    if (isSataOdd) {
        if (sataDisabled) {
            LOGI("IsEdmEnableOddBurn built-in SATA ODD burn disabled, diskId=%{public}s", diskId.c_str());
            return false;
        }
        LOGI("IsEdmEnableOddBurn built-in SATA ODD burn not disabled, diskId=%{public}s", diskId.c_str());
        return true;
    }

    // 非SATA光驱即为外置光驱，查询EDM白名单；USB的vid/pid/sn仅在此时需要
    std::string vid = disk.GetVendorId();
    std::string pid = disk.GetProductId();
    std::string sn = disk.GetSerialNumber();
    LOGI("IsEdmEnableOddBurn external ODD, query EDM whitelist, diskId=%{public}s "
         "productId=%{public}s vendorId=%{public}s serialNumber=%{public}s extraInfo=%{public}s",
         diskId.c_str(), pid.c_str(), vid.c_str(), GetAnonyString(sn).c_str(), disk.GetExtraInfo().c_str());
    if (!IsExternalOddBurnAllowed(callerUserId, pid, vid, sn)) {
        LOGI("IsEdmEnableOddBurn EDM whitelist denied, diskId=%{public}s", diskId.c_str());
        return false;
    }
    LOGI("IsEdmEnableOddBurn EDM whitelist allowed, diskId=%{public}s", diskId.c_str());
    return true;
}
 
bool EdmAdapter::IsExternalOddBurnAllowed(int32_t userId,
                                          const std::string &pid,
                                          const std::string &vid,
                                          const std::string &sn)
{
    LOGI("IsExternalOddBurnAllowed enter, userId=%{public}d pid=%{public}s vid=%{public}s sn=%{public}s",
         userId, pid.c_str(), vid.c_str(), GetAnonyString(sn).c_str());

    int32_t vendorId = 0;
    int32_t productId = 0;
    if (!ConvertStringToInt(vid, vendorId)) {
        LOGW("IsExternalOddBurnAllowed convert vid to int failed, vid=%{public}s", vid.c_str());
    }
    if (!ConvertStringToInt(pid, productId)) {
        LOGW("IsExternalOddBurnAllowed convert pid to int failed, pid=%{public}s", pid.c_str());
    }
    LOGI("IsExternalOddBurnAllowed call EDM IsAllowedOddBurn, userId=%{public}d vendorId=%{public}d "
         "productId=%{public}d sn=%{public}s", userId, vendorId, productId, GetAnonyString(sn).c_str());
    return true; // 允许刻录
}

} // namespace DiskManager
} // namespace OHOS
