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

#ifndef OHOS_FILEMANAGEMENT_DISK_MANAGER_DFX_H
#define OHOS_FILEMANAGEMENT_DISK_MANAGER_DFX_H

#include <string>

#include "disk_manager_dfx_types.h"
#include "disk_manager_errno.h"
#include "disk_manager_radar.h"

namespace OHOS {
namespace DiskManager {

// Behavior DFX scope for both IPC and business-layer entry (e.g. DiskManager::Mount).
class IpcDfxScope {
public:
    IpcDfxScope(const std::string &funcName, int32_t stage, VolumeOpType opType, const VolumeReportInfo &info)
        : funcName_(funcName), stage_(stage), opType_(opType), info_(info)
    {
        (void)opType_;
        DiskManagerRadar::GetInstance().ReportBehavior(funcName_, stage_, "START", info_, E_OK);
    }

    IpcDfxScope(const IpcDfxScope &) = delete;
    IpcDfxScope &operator=(const IpcDfxScope &) = delete;
    IpcDfxScope(IpcDfxScope &&) = delete;
    IpcDfxScope &operator=(IpcDfxScope &&) = delete;

    void MergeFrom(const VolumeReportInfo &other)
    {
        info_.MergeFrom(other);
    }

    int32_t Finish(int32_t ret)
    {
        if (finished_) {
            return ret;
        }
        finished_ = true;
        const std::string status = (ret == E_OK) ? "SUCCESS" : "FAIL";
        DiskManagerRadar::GetInstance().ReportBehavior(funcName_, stage_, status, info_, ret);
        return ret;
    }

    ~IpcDfxScope()
    {
        if (!finished_) {
            Finish(DISK_MGR_ERR);
        }
    }

private:
    std::string funcName_;
    int32_t stage_ = 0;
    VolumeOpType opType_ = VolumeOpType::OTHER;
    VolumeReportInfo info_;
    bool finished_ = false;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_FILEMANAGEMENT_DISK_MANAGER_DFX_H
