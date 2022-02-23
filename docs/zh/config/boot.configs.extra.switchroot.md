引用:
 - [src/boot/root.c](../../../src/boot/root.c)

# boot.configs.?.extra.rw

将设备挂载为可读写或只读

当值为`0`时将挂载为只读

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_SWITCHROOT`

类型: `INTEGER` (64位整型数字)

支持的环境: `LINUX`

# boot.configs.?.extra.fstype

指定文件系统格式挂载设备

该选项作用于[boot.configs.?.extra.path](#boot.configs.?.extra.path)

可参考`/proc/filesystems`

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_SWITCHROOT`

类型: `STRING` (字符串)

支持的环境: `LINUX`

# boot.configs.?.extra.flags

指定文件系统的挂载选项

该选项作用于[boot.configs.?.extra.path](#boot.configs.?.extra.path)

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_SWITCHROOT`

类型: `STRING` (字符串)

支持的环境: `LINUX`

# boot.configs.?.extra.init

指定切换到新根目录后执行的init

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_SWITCHROOT`

类型: `STRING` (字符串)

支持的环境: `LINUX`

默认:
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

指定文件系统的等待时间

如果找不到要挂载的文件系统，则等待指定的时间

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_SWITCHROOT`

取值: 秒

类型: `INTEGER` (64位整型数字)

支持的环境: `LINUX`

默认: `5`

# boot.configs.?.extra.path

指定要挂载的文件系统

注: 该值不总是最终的根目录，如果使用了`loop`或者`overlayfs`将会相应的替换为配置后的目录

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_SWITCHROOT`

取值:
 - `/dev/sdXXXX`                  ( `/dev/sda` / `/dev/sdb1` / `/dev/sde6` )
 - `/dev/vdXXXX`                  ( `/dev/vda` / `/dev/vdb1` / `/dev/vde6` )
 - `/dev/nvmeXnXXXX`              ( `/dev/nvme0n1` / `/dev/nvme1n1p1` )
 - `UUID=xxxx-xxxx-xxxx-xxxx`     ( `UUID=3693746e-4b4c-4bf0-934f-addfea74b563` )
 - `LABEL=xxxxx`                  ( `LABEL=ESP` / `LABEL=Linux` )
 - `PARTUUID=xxxx-xxxx-xxxx-xxxx` ( `PARTUUID=6948c4cb-b1a1-4ed6-9ad3-4ca6e4038e1b` )
 - `PARTLABEL=xxxxx`              ( `PARTLABEL=BOOT` / `PARTLABEL=esp` )

类型: `STRING` (字符串)

支持的环境: `LINUX`

# boot.configs.?.extra.loop

指定要挂载为`loop`的文件系统镜像路径

未设置或者值为`"none"`时将会跳过

该路径相对于[boot.configs.?.extra.path](#boot.configs.?.extra.path)挂载的文件系统

注: 该值不总是最终的根目录，如果使用了`overlayfs`将会相应的替换为配置后的目录

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_SWITCHROOT`

类型: `STRING` (字符串)

支持的环境: `LINUX`

# boot.configs.?.extra.loop_sector

指定要挂载为`loop`的文件系统镜像的扇区大小

未设置将跟随默认，常见值为`512`、`4096`等

该选项作用于[boot.configs.?.extra.loop](#boot.configs.?.extra.loop)

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_SWITCHROOT`

类型: `INTEGER` (64位整型数字)

支持的环境: `LINUX`

# boot.configs.?.extra.loop_partno

指定要挂载为`loop`的文件系统镜像的分区序号

如果目标镜像存在分区表，则可以用此参数指定要挂载的目标分区

该选项作用于[boot.configs.?.extra.loop](#boot.configs.?.extra.loop)

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_SWITCHROOT`

类型: `INTEGER` (64位整型数字)

支持的环境: `LINUX`

# boot.configs.?.extra.loop_offset

指定要挂载为`loop`的文件系统镜像的偏移量

如果要挂载的文件系统不在目标镜像的最开始，则使用指定文件系统在镜像中所在位置偏移量

该选项作用于[boot.configs.?.extra.loop](#boot.configs.?.extra.loop)

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_SWITCHROOT`

类型: `INTEGER` (64位整型数字)

支持的环境: `LINUX`

# boot.configs.?.extra.loop_fstype

指定要挂载的`loop`镜像的文件系统格式

该选项作用于[boot.configs.?.extra.loop](#boot.configs.?.extra.loop)

可参考`/proc/filesystems`

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_SWITCHROOT`

类型: `STRING` (字符串)

支持的环境: `LINUX`

# boot.configs.?.extra.loop_flags

指定要挂载的`loop`镜像的文件系统选项

该选项作用于[boot.configs.?.extra.loop](#boot.configs.?.extra.loop)

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_SWITCHROOT`

类型: `STRING` (字符串)

支持的环境: `LINUX`

# boot.configs.?.extra.data

指定要挂载用于`overlayfs`后端存储的文件系统

未设置或者值为`"none"`时将会跳过

值为`"tmpfs"`时将使用`tmpfs`，数据将仅保存于内存中

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_SWITCHROOT`

取值:
 - `/dev/sdXXXX`                  ( `/dev/sda` / `/dev/sdb1` / `/dev/sde6` )
 - `/dev/vdXXXX`                  ( `/dev/vda` / `/dev/vdb1` / `/dev/vde6` )
 - `/dev/nvmeXnXXXX`              ( `/dev/nvme0n1` / `/dev/nvme1n1p1` )
 - `UUID=xxxx-xxxx-xxxx-xxxx`     ( `UUID=3693746e-4b4c-4bf0-934f-addfea74b563` )
 - `LABEL=xxxxx`                  ( `LABEL=ESP` / `LABEL=Linux` )
 - `PARTUUID=xxxx-xxxx-xxxx-xxxx` ( `PARTUUID=6948c4cb-b1a1-4ed6-9ad3-4ca6e4038e1b` )
 - `PARTLABEL=xxxxx`              ( `PARTLABEL=BOOT` / `PARTLABEL=esp` )

类型: `STRING` (字符串)

支持的环境: `LINUX`

# boot.configs.?.extra.data_prefix

指定要用于`overlayfs`后端存储的路径前缀

该路径相对于[boot.configs.?.extra.data](#boot.configs.?.extra.data)挂载的文件系统

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_SWITCHROOT`

类型: `STRING` (字符串)

支持的环境: `LINUX`

# boot.configs.?.extra.data_size

指定要用于`overlayfs`后端存储路径的`tmpfs`大小

该选项作用于[boot.configs.?.extra.data](#boot.configs.?.extra.data)

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_SWITCHROOT`
 - [boot.configs.?.extra.data](#boot.configs.?.extra.data)为`"tmpfs"`

取值:
 - `XX%`   ( `10%` / `55%` / `100%` )
 - `XXXXX` ( `1048576` / `536870912` )
 - `XXk`   ( `512k` / `900k` )
 - `XXm`   ( `128m` / `500m` / `900m` )
 - `XXg`   ( `1g` / `4g` / `16g` )

类型: `STRING` (字符串)

支持的环境: `LINUX`

# boot.configs.?.extra.data_fstype

指定要用于`overlayfs`后端存储挂载时的文件系统格式

该选项将用于挂载[boot.configs.?.extra.data](#boot.configs.?.extra.data)

可参考`/proc/filesystems`

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_SWITCHROOT`

类型: `STRING` (字符串)

支持的环境: `LINUX`

# boot.configs.?.extra.data_flags

指定要用于`overlayfs`后端存储挂载时的文件系统选项

该选项将用于挂载[boot.configs.?.extra.data](#boot.configs.?.extra.data)

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_SWITCHROOT`

类型: `STRING` (字符串)

支持的环境: `LINUX`

# boot.configs.?.extra.overlay_name

挂载`overlayfs`的名称

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_SWITCHROOT`

类型: `STRING` (字符串)

支持的环境: `LINUX`
