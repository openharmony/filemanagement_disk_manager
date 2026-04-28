# Disk Management

## Disk Management Overview

Disk Management is the disk management component of the OpenHarmony File Management subsystem. It relies on open-source third-party disk utilities shipped with OpenHarmony (such as gptfdisk and f2fs-tools) to discover disks and volumes, handle mounting and unmounting, partitioning, filesystem checking and repair, formatting, and related events.

## System Architecture

**Figure 1** OpenHarmony disk management architecture

![OpenHarmony disk management architecture](figures/disk-management-architecture.png)

### Disk Management Architecture Description

As shown in the diagram, the software stack consists of the application layer, framework layer, system service layer, and kernel layer.

- **Application layer**
  - **File Management**: Issues volume management requests for file and volume scenarios
  - **System Settings**: Issues disk and partition management requests for device maintenance
- **Framework layer**
  - **Core File Kit**: Presents a unified API upward and forwards requests to system services
- **System service layer**
  - **Disk Management Service**: Core orchestration module responsible for state management, policy control, and presenting capabilities outward
  - **Disk query**: Aggregates disk and partition information and status
  - **Disk partitioning**: Creates, resizes, and deletes partitions
  - **Disk check**: Performs filesystem checks
  - **Mount and unmount**: Performs volume mount and unmount operations and state transitions
  - **Formatting**: Performs filesystem formatting
  - **Check and repair**: Performs filesystem repair
  - [**Storage Management Service**](https://gitcode.com/openharmony/filemanagement_storage_service): Executes privileged operations, interacts with lower-level facilities, and returns results upstream
- **Kernel layer**
  - **Disk utilities**: Provides low-level execution for partitioning, formatting, checks, and repair
  - **Kernel filesystems**: Supplies disk information queries and reports state changes

Call flow is as follows: File Management and System Settings reach Disk Management Service through Core File Kit. Disk Management Service coordinates with Storage Management Service over Inter-Process Communication (IPC). The kernel layer performs the actual disk operations and reports status back.

## Directory Structure

```text
.
├── interfaces/                  # Public API layer (IDL, innerkits, JS)
│   ├── innerkits/
│   └── kits/
├── services/disk_manager/       # SystemAbility (SA) implementation (provider, business logic, daemon adapters)
├── sa_profile/                  # SystemAbility and process configuration
├── common/                      # Shared error codes and base definitions
├── etc/                         # Business configuration
├── figures/                     # Diagrams for this component
├── utils/                       # Logging and common utilities
└── test/                        # Unit tests and fuzz tests
```

## Disk Management Capabilities

- Disk and partition event handling
- Volume information queries
- Volume mount and unmount, formatting, check, and repair

## Build and Integration

Disk Management is delivered as a system service component in the OpenHarmony File Management (`filemanagement`) subsystem and is built into the system image.

- **Source placement**: Check out this repository to the agreed location in the OpenHarmony source tree (see `bundle.json` in this repository for the path definition; the sample path is `foundation/filemanagement/disk_manager`). After aligning with the upstream directory layout, participate in full system builds.
- **Product integration**: In a product or board solution project, pull in the `disk_manager` build target through component dependencies (`deps`/component lists). Exact dependency entries follow the component integration rules of your distribution or product baseline.

## Disk Management Developer Guide

### Application-Side Disk Management Development Flow

The following applies to application developers building on a system image that already includes this component and who need volume and disk management capabilities.

1. Import APIs through Core File Kit in ArkTS (example: `import { volumeManager } from '@kit.CoreFileKit';`). To invoke capabilities from C++, select innerkits according to project conventions. If you use Taihe or similar stacks, follow your project template.
2. `@ohos.file.volumeManager` is a system API and can only be used by system applications. Declare the application type in configuration files such as `module.json5`, and request `ohos.permission` entries that match each API (for example `STORAGE_MANAGER` for volume queries, `MOUNT_UNMOUNT_MANAGER` for mount and unmount, `MOUNT_FORMAT_MANAGER` for formatting and partitioning). See the official documentation for the authoritative mapping between permissions and APIs.
3. Call `getAllVolumes`, `getVolumeById`, `getVolumeByUuid`, `mount`, `unmount`, `format`, `partition`, `setVolumeDescription`, and related APIs using a volume ID (example: `vol-{major}-{minor}`) or a UUID. Meet each API's prerequisites for filesystem type and volume state (for example some operations only apply when the volume is unmounted), and consult documented error codes.
4. Refer to @ohos.file.volumeManager (system APIs) and the Core File Kit overview in **API Reference** below for descriptions, parameters, error codes, and samples.

## API Reference

- [Core File Kit overview](https://github.com/openharmony/docs/blob/master/en/application-dev/reference/apis-core-file-kit/Readme-EN.md)
- [Volume and disk management (system APIs)](https://github.com/openharmony/docs/blob/master/en/application-dev/reference/apis-core-file-kit/js-apis-file-volumemanager-sys.md)

These documents define disk-management-related APIs and describe parameters and error codes. On devices that expose the required disk capabilities, system applications can use them to discover devices and perform volume mount and unmount, partitioning, check, repair, formatting, and related operations.

## Related Repositories

- [filemanagement_disk_manager](https://gitcode.com/openharmony-sig/filemanagement_disk_manager)
- [filemanagement_storage_service](https://gitcode.com/openharmony/filemanagement_storage_service)
- [applications_settings_data](https://gitcode.com/openharmony/applications_settings_data)
- [third_party_gptfdisk](https://gitcode.com/openharmony/third_party_gptfdisk)
- [third_party_e2fsprogs](https://gitcode.com/openharmony/third_party_e2fsprogs)
- [third_party_f2fs-tools](https://gitcode.com/openharmony/third_party_f2fs-tools)
- [third_party_ntfs-3g](https://gitcode.com/openharmony/third_party_ntfs-3g)
- [third_party_exfatprogs](https://gitcode.com/openharmony/third_party_exfatprogs)
