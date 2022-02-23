Reference:
 - [src/boot/root.c](../../../src/boot/root.c)

# boot.configs.?.extra.rw

Mount the device as read-write or read-only

When the value is `0` will mount as read-only

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_SWITCHROOT`

Type: `INTEGER`

Supported environments: `LINUX`

# boot.configs.?.extra.fstype

Mount the device in the specified filesystem format

This option acts on [boot.configs.?.extra.path](#boot.configs.?.extra.path)

Refer to `/proc/filesystems`

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_SWITCHROOT`

Type: `STRING`

Supported environments: `LINUX`

# boot.configs.?.extra.flags

Specify mount options for the filesystem

This option acts on [boot.configs.?.extra.path](#boot.configs.?.extra.path)

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_SWITCHROOT`

Type: `STRING`

Supported environments: `LINUX`

# boot.configs.?.extra.init

Specifies the init to be execute after switch to the new root directory

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_SWITCHROOT`

Type: `STRING`

Supported environments: `LINUX`

Default:
 - `"/sbin/init"`
 - `"/bin/init"`
 - `"/usr/sbin/init"`
 - `"/usr/bin/init"`
 - `"/usr/local/sbin/init"`
 - `"/usr/local/bin/init"`
 - `"/lib/systemd/systemd"`
 - `"/usr/lib/systemd/systemd"`
 - `"/usr/local/lib/systemd/systemd"`
 - `"/init"`
 - `"/linuxrc"`
 - `"/bin/bash"`
 - `"/usr/bin/bash"`
 - `"/usr/local/bin/bash`
 - `"/bin/sh"`
 - `"/usr/bin/sh"`
 - `"/usr/local/bin/sh"`

# boot.configs.?.extra.wait

Specifies the wait time for the filesystem

If filesystem is not found, wait the specified time

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_SWITCHROOT`

Values: Seconds

Type: `INTEGER`

Supported environments: `LINUX`

Default: `5`

# boot.configs.?.extra.path

Specify the filesystem to mount

Notice: This value is not always the final root directory, if `loop` or `overlayfs` is used it will be replaced with the configured directory accordingly

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_SWITCHROOT`

Values:
 - `/dev/sdXXXX`                  ( `/dev/sda` / `/dev/sdb1` / `/dev/sde6` )
 - `/dev/vdXXXX`                  ( `/dev/vda` / `/dev/vdb1` / `/dev/vde6` )
 - `/dev/nvmeXnXXXX`              ( `/dev/nvme0n1` / `/dev/nvme1n1p1` )
 - `UUID=xxxx-xxxx-xxxx-xxxx`     ( `UUID=3693746e-4b4c-4bf0-934f-addfea74b563` )
 - `LABEL=xxxxx`                  ( `LABEL=ESP` / `LABEL=Linux` )
 - `PARTUUID=xxxx-xxxx-xxxx-xxxx` ( `PARTUUID=6948c4cb-b1a1-4ed6-9ad3-4ca6e4038e1b` )
 - `PARTLABEL=xxxxx`              ( `PARTLABEL=BOOT` / `PARTLABEL=esp` )

Type: `STRING`

Supported environments: `LINUX`

# boot.configs.?.extra.loop

Specifies the filesystem image path to mount as `loop`

Skip if not set or if the value is `"none"`

The path is relative to the filesystem mounted by [boot.configs.?.extra.path](#boot.configs.?.extra.path)

Notice: This value is not always the final root directory, if `overlayfs` is used it will be replaced with the configured directory accordingly

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_SWITCHROOT`

Type: `STRING`

Supported environments: `LINUX`

# boot.configs.?.extra.loop_sector

Specifies the sector size of the filesystem image to be mounted as `loop`

If not set, it will follow the default, common values are `512`, `4096`, etc.

This option acts on [boot.configs.?.extra.loop](#boot.configs.?.extra.loop)

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_SWITCHROOT`

Type: `INTEGER`

Supported environments: `LINUX`

# boot.configs.?.extra.loop_partno

Specifies the partition number of the filesystem image to be mounted as `loop`

If the target image has a partition table, you can use this parameter to specify the target partition to be mounted

This option acts on [boot.configs.?.extra.loop](#boot.configs.?.extra.loop)

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_SWITCHROOT`

Type: `INTEGER`

Supported environments: `LINUX`

# boot.configs.?.extra.loop_offset

Specifies the offset of the filesystem image to be mounted as `loop`

If the filesystem to be mounted is not at the beginning of the target image, use the offset of the specified filesystem's location in the image

This option acts on [boot.configs.?.extra.loop](#boot.configs.?.extra.loop)

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_SWITCHROOT`

Type: `INTEGER`

Supported environments: `LINUX`

# boot.configs.?.extra.loop_fstype

Specifies the filesystem format of the `loop` image to mount

This option acts on [boot.configs.?.extra.loop](#boot.configs.?.extra.loop)

Refer to `/proc/filesystems`

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_SWITCHROOT`

Type: `STRING`

Supported environments: `LINUX`

# boot.configs.?.extra.loop_flags

Filesystem options specifying the `loop` image to mount

This option acts on [boot.configs.?.extra.loop](#boot.configs.?.extra.loop)

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_SWITCHROOT`

Type: `STRING`

Supported environments: `LINUX`

# boot.configs.?.extra.data

Specifies the filesystem to mount for the `overlayfs` backend storage

Skip if not set or if the value is `"none"`

A value of `"tmpfs"` will use `tmpfs`, the data will be kept in memory only

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_SWITCHROOT`

Values:
 - `/dev/sdXXXX`                  ( `/dev/sda` / `/dev/sdb1` / `/dev/sde6` )
 - `/dev/vdXXXX`                  ( `/dev/vda` / `/dev/vdb1` / `/dev/vde6` )
 - `/dev/nvmeXnXXXX`              ( `/dev/nvme0n1` / `/dev/nvme1n1p1` )
 - `UUID=xxxx-xxxx-xxxx-xxxx`     ( `UUID=3693746e-4b4c-4bf0-934f-addfea74b563` )
 - `LABEL=xxxxx`                  ( `LABEL=ESP` / `LABEL=Linux` )
 - `PARTUUID=xxxx-xxxx-xxxx-xxxx` ( `PARTUUID=6948c4cb-b1a1-4ed6-9ad3-4ca6e4038e1b` )
 - `PARTLABEL=xxxxx`              ( `PARTLABEL=BOOT` / `PARTLABEL=esp` )

Type: `STRING`

Supported environments: `LINUX`

# boot.configs.?.extra.data_prefix

Specifies the path prefix to use for the `overlayfs` backend storage

The path is relative to the filesystem mounted by [boot.configs.?.extra.data](#boot.configs.?.extra.data)

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_SWITCHROOT`

Type: `STRING`

Supported environments: `LINUX`

# boot.configs.?.extra.data_size

Specify the `tmpfs` size to use for the `overlayfs` backend storage path

This option acts on [boot.configs.?.extra.data](#boot.configs.?.extra.data)

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_SWITCHROOT`
 - [boot.configs.?.extra.data](#boot.configs.?.extra.data) is `"tmpfs"`

Values:
 - `XX%`   ( `10%` / `55%` / `100%` )
 - `XXXXX` ( `1048576` / `536870912` )
 - `XXk`   ( `512k` / `900k` )
 - `XXm`   ( `128m` / `500m` / `900m` )
 - `XXg`   ( `1g` / `4g` / `16g` )

Type: `STRING`

Supported environments: `LINUX`

# boot.configs.?.extra.data_fstype

Specifies the filesystem format to use for `overlayfs` backend storage mounts

This option acts on [boot.configs.?.extra.data](#boot.configs.?.extra.data)

Refer to `/proc/filesystems`

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_SWITCHROOT`

Type: `STRING`

Supported environments: `LINUX`

# boot.configs.?.extra.data_flags

Specify filesystem options to be used for `overlayfs` backend storage mounts

This option acts on [boot.configs.?.extra.data](#boot.configs.?.extra.data)

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_SWITCHROOT`

Type: `STRING`

Supported environments: `LINUX`

# boot.configs.?.extra.overlay_name

Name to mount `overlayfs`

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_SWITCHROOT`

Type: `STRING`

Supported environments: `LINUX`
