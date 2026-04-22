# Disk Management

## Introduction

Disk Management is the disk-management component of the OpenHarmony file management subsystem. It builds on open-source third-party disk utilities shipped with OpenHarmony (such as gptdisk and f2fs-tools) to discover disks and volumes, handle mount and unmount, partitioning, filesystem check and repair, formatting, and the related events.

## System architecture

**Figure 1** OpenHarmony disk management architecture

![OpenHarmony disk management architecture](figures/disk-management-architecture.png)

### Architecture description

As shown in the diagram, the stack consists of the **application layer**, **framework layer**, **system service layer**, and **kernel layer**:

- **Application layer**
  - **File Management**: Sends volume-management requests for file and volume scenarios.
  - **System Settings**: Sends disk and partition management requests for device maintenance.
- **Framework layer**
  - **Core File Kit**: Exposes a unified API upward and forwards requests to system services.
- **System service layer**
  - **Disk Management Service**: Core orchestration—state management, policy, and the outward-facing capability surface.
  - **Disk query**: Aggregates disk and partition information and status.
  - **Disk partitioning**: Creates, resizes, and deletes partitions.
  - **Disk check**: Runs filesystem check operations.
  - **Mount / unmount**: Performs volume mount, unmount, and state transitions.
  - **Formatting**: Runs filesystem formatting.
  - **Check and repair**: Runs filesystem repair operations.
  - [**Storage Management Service**](https://gitcode.com/openharmony/filemanagement_storage_service): Runs privileged operations, talks to lower-level facilities, and returns results upstream.
- **Kernel layer**
  - **Disk utilities**: Low-level execution for partitioning, formatting, check, and repair.
  - **Kernel filesystems**: Supplies disk information queries and reports state changes.

**Call flow:** **File Management** and **System Settings** reach **Disk Management Service** through **Core File Kit**; it then works with **Storage Management Service** over IPC, and the kernel layer performs the actual disk work and reports status back.

## Directory structure

```text
.
├── interfaces/                  # Public API layer (IDL, innerkits, JS)
│   ├── innerkits/
│   └── kits/
├── services/disk_manager/       # SystemAbility implementation (provider, business logic, daemon adapters)
├── sa_profile/                  # SystemAbility and process configuration
├── common/                      # Shared error codes and base definitions
├── etc/                         # Business configuration
├── figures/                     # Diagrams for this component
├── utils/                       # Logging and common utilities
└── test/                        # Unit tests and fuzz tests
```

## Key capabilities

- Disk and partition event handling
- Volume information query
- Volume mount and unmount, formatting, check, and repair

## Build and integration

Disk Management is delivered as a **system service component** in the OpenHarmony **filemanagement** subsystem and is compiled into the system image:

- **Source placement**: Place this repository in the path defined in `bundle.json` inside the tree—namely `foundation/filemanagement/disk_manager`—aligned with the upstream layout so it takes part in full system builds.
- **Product integration**: In a product or board solution, depend on the **disk_manager** component through component dependencies (`deps` / component lists). Exact dependency entries follow the integration rules of your distribution or product baseline.

## Usage

### Developer workflow (application development)

For **application developers** who use volume and disk management capabilities in **system applications** (on a system image that already includes this component):

1. **Use the APIs**: In ArkTS, import via **Core File Kit**, for example `import { volumeManager } from '@kit.CoreFileKit';`. For native C++, use **innerkits** where your project allows; if you use **Taihe** or other stacks, follow your project template.
2. **System APIs and identity**: `@ohos.file.volumeManager` is a **system API** and can only be called from **system applications**. Declare the application type and request matching **ohos.permission** entries in `module.json5` (for example `STORAGE_MANAGER` for volume query, `MOUNT_UNMOUNT_MANAGER` for mount/unmount, `MOUNT_FORMAT_MANAGER` for format/partition—the official permission-to-API matrix is authoritative).
3. **Inputs and behavior**: Use a volume **ID** (for example `vol-{major}-{minor}`) or **UUID** with `getAllVolumes`, `getVolumeById`, `getVolumeByUuid`, `mount`, `unmount`, `format`, `partition`, `setVolumeDescription`, and related APIs. Observe per-API prerequisites such as supported filesystem types and volume state (for example some operations require an **unmounted** volume), and the documented error codes.
4. **Documentation**: Interface descriptions, parameters, error codes, and samples are in **API reference** below—**@ohos.file.volumeManager (system API)** and the Core File Kit overview.

## API reference

[Core File Kit overview](https://github.com/openharmony/docs/blob/master/en/application-dev/reference/apis-core-file-kit/Readme-EN.md)

[Volume and disk management (system APIs)](https://github.com/openharmony/docs/blob/master/en/application-dev/reference/apis-core-file-kit/js-apis-file-volumemanager-sys.md)

These pages document the public interfaces. System applications can manage supported disk devices—discovery, mount and unmount, partitioning, check, repair, formatting, and more—and provide the authoritative definitions and usage patterns.

## Related repositories

- [filemanagement_disk_manager](https://gitcode.com/openharmony-sig/filemanagement_disk_manager)
- [filemanagement_storage_service](https://gitcode.com/openharmony/filemanagement_storage_service)
- [applications_settings_data](https://gitcode.com/openharmony/applications_settings_data)
- [third_party_gptfdisk](https://gitcode.com/openharmony/third_party_gptfdisk)
- [third_party_e2fsprogs](https://gitcode.com/openharmony/third_party_e2fsprogs)
- [third_party_f2fs-tools](https://gitcode.com/openharmony/third_party_f2fs-tools)
- [third_party_ntfs-3g](https://gitcode.com/openharmony/third_party_ntfs-3g)
- [third_party_exfatprogs](https://gitcode.com/openharmony/third_party_exfatprogs)
