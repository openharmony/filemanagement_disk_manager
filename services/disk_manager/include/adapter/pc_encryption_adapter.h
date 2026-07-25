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

#ifndef OHOS_FILEMANAGEMENT_DISK_MANAGER_PC_ENCRYPTION_ADAPTER_H
#define OHOS_FILEMANAGEMENT_DISK_MANAGER_PC_ENCRYPTION_ADAPTER_H

#include <cstdint>
#include <string>

#include "nocopyable.h"

namespace OHOS {
namespace DiskManager {

/**
 * 动态加载 libpc_encryption_ext_volume_user_api.z.so，查询数据盘加密状态。
 * 语义对齐 UsbFuseAdapter 的设计模式。
 */
class PcEncryptionAdapter : public NoCopyable {
public:
    static PcEncryptionAdapter &GetInstance();

    /**
     * 查询指定路径的加密状态。
     * @param volPath 卷路径，格式为 /mnt/data/voldata/data[diskId]
     * @param encStatus 输出的加密状态码
     * @return true 表示查询成功，false 表示查询失败（静默处理）
     */
    bool QueryEncryptionStatus(const std::string &volPath, int32_t &encStatus);

    /**
     * 通知数据盘卷已挂载完成（异步执行，不等待返回）。
     * @param diskId 磁盘ID
     * @param volumeId 卷ID
     * @param volPath 卷挂载路径
     */
    void NotifyVolumeMounted(const std::string &diskId,
                             const std::string &volumeId,
                             const std::string &volPath);

private:
    PcEncryptionAdapter();
    ~PcEncryptionAdapter();

    void Init();    // dlopen 加载 so
    void UnInit();  // dlclose 关闭 so

    void *handler_{nullptr};
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_FILEMANAGEMENT_DISK_MANAGER_PC_ENCRYPTION_ADAPTER_H