# 磁盘管理

## 简介

磁盘管理是 OpenHarmony 文件管理子系统中的磁盘管理部件，使用 OpenHarmony 中集成的开源三方磁盘操作工具（如 gptfdisk、f2fs-tools 等），负责磁盘与卷的识别、挂载/卸载、分区、检查、修复、格式化及相关事件处理。

## 系统架构

**图1** OpenHarmony磁盘管理架构图

![](figures/disk-management-architecture-zh.png)

### 架构说明

按图中模块划分，系统由应用层、框架层、系统服务层和内核层构成：

- **应用层**
  - `文件管理`：面向文件与卷场景发起卷管理请求。
  - `系统设置`：面向设备维护场景发起磁盘/分区管理请求。
- **框架层**
  - `Core File Kit`：对上提供统一接口，将请求转发到系统服务。
- **系统服务层**
  - `磁盘管理服务`：核心编排模块，负责状态管理、策略控制和能力对外暴露。
  - `磁盘查询`：聚合磁盘/分区信息与状态。
  - `磁盘分区`：执行分区创建、调整与删除。
  - `磁盘检查`：执行对文件系统的检查操作。
  - `挂载卸载`：执行卷挂载、卸载和状态切换。
  - `格式化`：执行文件系统格式化。
  - `检查修复`：执行文件系统的修复操作。
  -  [存储管理服务](https://gitcode.com/openharmony/filemanagement_storage_service)：承接高权限操作执行，负责与底层能力交互并回传结果。
- **内核层**
  - `磁盘操作工具`：提供分区、格式化、检查、修复等底层执行能力。
  - `内核文件系统`：提供磁盘信息查询和状态变化的上报能力。

调用关系上，`文件管理/系统设置` 通过 `Core File Kit` 进入 `磁盘管理服务`，再由其通过 IPC 协同 `存储管理服务`，最终调用内核层完成实际磁盘操作与状态回传。


## 目录结构

```text
.
├── interfaces/                  # 对外接口层（IDL、innerkits、JS）
│   ├── innerkits/
│   └── kits/
├── services/disk_manager/       # SA 服务实现（Provider、业务管理、daemon 适配）
├── sa_profile/                  # SystemAbility 配置与进程配置
├── common/                      # 通用错误码与基础定义
├── etc                          # 业务配置信息
├── figures                      # 服务介绍图
├── utils/                       # 日志与通用工具
└── test/                        # 单元测试与模糊测试
```

## 主要能力

- 磁盘与分区事件处理
- 卷信息查询
- 卷挂载/卸载、格式化、检查和修复

## 构建与集成

磁盘管理以 **系统服务部件** 形式纳入 OpenHarmony **文件管理（filemanagement）** 子系统，随系统镜像编译与部署：

- **源码落位**：将本仓置于 OpenHarmony 源码树约定路径（见仓内 `bundle.json`），具体为 `foundation/filemanagement/disk_manager`，与主干目录结构对齐后参与全量构建。
- **产品集成**：在产品或芯片方案工程中，通过部件依赖（`deps` / 部件列表）引入 **disk_manager** 目标；具体依赖写法以所在发行版 / 产品工程的部件集成规范为准。

## 使用指南

### 开发者使用流程（应用开发）

面向在系统应用中访问卷 / 磁盘管理能力的 **应用开发者**（需在已集成本部件的系统上开发）：

1. **接入 API**：在 ArkTS 中通过 **Core File Kit** 导入，例如 `import { volumeManager } from '@kit.CoreFileKit';`。若需 C++ 侧能力，可按项目规范选用 **innerkits**；另有 **Taihe** 等链路时以工程模板为准。
2. **系统接口与身份**：`@ohos.file.volumeManager` 为 **系统接口**，仅 **系统应用** 可调用；在 `module.json5` 等配置中声明应用类型，并申请与各接口匹配的 **ohos.permission**（如卷查询常用 `STORAGE_MANAGER`，挂载/卸载常用 `MOUNT_UNMOUNT_MANAGER`，格式化/分区常用 `MOUNT_FORMAT_MANAGER`；完整权限表与接口对应关系以官方文档为准）。
3. **调用数据与能力**：使用卷 **ID**（如 `vol-{主设备号}-{次设备号}`）或 **UUID** 调用 `getAllVolumes`、`getVolumeById`、`getVolumeByUuid`、`mount`、`unmount`、`format`、`partition`、`setVolumeDescription` 等；注意各接口对文件系统类型、卷状态（如部分操作仅 **卸载态** 可用）的前置条件及返回错误码。
4. **查阅文档**：接口说明、参数、错误码与示例见下文 **「API参考」** 中的 **@ohos.file.volumeManager（系统接口）** 与 Core File Kit 总览。

## API参考

[Core File Kit介绍](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-core-file-kit/Readme-CN.md)

[磁盘管理能力使用参考](https://gitcode.com/openharmony/docs/blob/ca6a74112dca41d78b4bb2ca2612aca7d2bce450/zh-cn/application-dev/reference/apis-core-file-kit/js-apis-file-volumemanager-sys.md)

提供了接口的说明文档，系统应用可以对已支持磁盘设备，进行识别、挂载/卸载、分区、检查、修复、格式化等。可以帮助开发者快速查找到指定接口的详细描述和调用方法。

## 相关仓

- [filemanagement_disk_manager](https://gitcode.com/openharmony-sig/filemanagement_disk_manager)
- [filemanagement_storage_service](https://gitcode.com/openharmony/filemanagement_storage_service)
- [applications_settings_data](https://gitcode.com/openharmony/applications_settings_data)
- [third_party_gptfdisk](https://gitcode.com/openharmony/third_party_gptfdisk)
- [third_party_e2fsprogs](https://gitcode.com/openharmony/third_party_e2fsprogs)
- [third_party_f2fs-tools](https://gitcode.com/openharmony/third_party_f2fs-tools)
- [third_party_ntfs-3g](https://gitcode.com/openharmony/third_party_ntfs-3g)
- [third_party_exfatprogs](https://gitcode.com/openharmony/third_party_exfatprogs)
