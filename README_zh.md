# 磁盘管理

## 磁盘管理概述

磁盘管理是OpenHarmony文件管理子系统中的磁盘管理部件。本部件基于OpenHarmony已集成的开源三方磁盘工具（例如gptfdisk、f2fs-tools等），负责磁盘与卷的识别、挂载与卸载、分区、检查、修复、格式化及相关事件处理。

## 系统架构

**图1** OpenHarmony磁盘管理架构

![](figures/disk-management-architecture-zh.png)

### 磁盘管理架构说明

如图所示，系统由下至上分为应用层、框架层、系统服务层与内核层。

- **应用层**
  - **文件管理**：面向文件与卷场景发起卷管理请求
  - **系统设置**：面向设备维护场景发起磁盘与分区管理请求
- **框架层**
  - **Core File Kit**：对上提供统一接口，并将请求转发至系统服务
- **系统服务层**
  - **磁盘管理服务**：核心编排模块，负责状态管理、策略控制及对外能力呈现
  - **磁盘查询**：聚合磁盘与分区信息及状态
  - **磁盘分区**：执行分区创建、调整与删除
  - **磁盘检查**：执行文件系统检查
  - **挂载卸载**：执行卷挂载、卸载及状态切换
  - **格式化**：执行文件系统格式化
  - **检查修复**：执行文件系统修复
  - [**存储管理服务**](https://gitcode.com/openharmony/filemanagement_storage_service)：承接高权限操作，与底层能力交互并回传结果
- **内核层**
  - **磁盘操作工具**：提供分区、格式化、检查、修复等底层执行能力
  - **内核文件系统**：提供磁盘信息查询及状态变化上报

调用关系如下：文件管理与系统设置通过Core File Kit进入磁盘管理服务。磁盘管理服务通过进程间通信（Inter-Process Communication，IPC）协同存储管理服务，由内核层完成实际磁盘操作并回传状态。

## 目录结构

```text
.
├── interfaces/                  # 对外接口层（IDL、innerkits、JS）
│   ├── innerkits/
│   └── kits/
├── services/disk_manager/       # SystemAbility（SA）服务实现（Provider、业务管理、daemon适配）
├── sa_profile/                  # SystemAbility配置与进程配置
├── common/                      # 通用错误码与基础定义
├── etc/                         # 业务配置信息
├── figures/                     # 服务介绍图
├── utils/                       # 日志与通用工具
└── test/                        # 单元测试与模糊测试
```

## 磁盘管理能力

- 磁盘与分区事件处理
- 卷信息查询
- 卷挂载与卸载、格式化、检查及修复

## 构建与集成

磁盘管理以系统服务部件形式纳入OpenHarmony文件管理（filemanagement）子系统，随系统镜像编译与部署。

- **源码落位**：将本仓置于OpenHarmony源码树中的约定路径（路径定义见仓内`bundle.json`，示例路径为`foundation/filemanagement/disk_manager`）。与主干目录结构对齐后参与全量构建。
- **产品集成**：在产品或芯片方案工程中，通过部件依赖（`deps`/部件列表）引入disk_manager构建目标。具体依赖写法以所属发行版或产品工程的部件集成规范为准。

## 磁盘管理开发指导

### 磁盘管理应用侧开发流程

下文面向在已集成本部件的系统镜像上开发、且需要使用卷与磁盘管理能力的应用开发者。

1. 在ArkTS中通过Core File Kit导入接口（示例：`import { volumeManager } from '@kit.CoreFileKit';`）。若需在C++侧调用能力，按工程规范选用innerkits。若使用Taihe等链路，以工程模板为准。
2. `@ohos.file.volumeManager`为系统接口，仅系统应用可调用。在`module.json5`等配置中声明应用类型，并申请与各接口匹配的ohos.permission（例如卷查询常用`STORAGE_MANAGER`，挂载与卸载常用`MOUNT_UNMOUNT_MANAGER`，格式化与分区常用`MOUNT_FORMAT_MANAGER`。权限与接口的对应关系以官方文档为准）。
3. 使用卷ID（示例：`vol-{主设备号}-{次设备号}`）或UUID，调用`getAllVolumes`、`getVolumeById`、`getVolumeByUuid`、`mount`、`unmount`、`format`、`partition`、`setVolumeDescription`等接口。遵守各接口对文件系统类型、卷状态等前置条件（例如部分操作仅在卸载态下可用），并参考文档给出的错误码。
4. 查阅下文API参考中的@ohos.file.volumeManager（系统接口）及Core File Kit总览，获取接口说明、参数、错误码与示例。

## API参考

- [Core File Kit介绍](https://gitcode.com/openharmony/docs/blob/master/zh-cn/application-dev/reference/apis-core-file-kit/Readme-CN.md)
- [磁盘管理能力使用参考（系统接口）](https://gitcode.com/openharmony/docs/blob/ca6a74112dca41d78b4bb2ca2612aca7d2bce450/zh-cn/application-dev/reference/apis-core-file-kit/js-apis-file-volumemanager-sys.md)

上述文档给出磁盘管理相关接口的定义、参数与错误码说明。系统应用在具备相应磁盘能力的设备上，可据此完成设备识别以及卷的挂载、卸载、分区、检查、修复与格式化等操作。

## 相关仓库

- [filemanagement_disk_manager](https://gitcode.com/openharmony-sig/filemanagement_disk_manager)
- [filemanagement_storage_service](https://gitcode.com/openharmony/filemanagement_storage_service)
- [applications_settings_data](https://gitcode.com/openharmony/applications_settings_data)
- [third_party_gptfdisk](https://gitcode.com/openharmony/third_party_gptfdisk)
- [third_party_e2fsprogs](https://gitcode.com/openharmony/third_party_e2fsprogs)
- [third_party_f2fs-tools](https://gitcode.com/openharmony/third_party_f2fs-tools)
- [third_party_ntfs-3g](https://gitcode.com/openharmony/third_party_ntfs-3g)
- [third_party_exfatprogs](https://gitcode.com/openharmony/third_party_exfatprogs)
