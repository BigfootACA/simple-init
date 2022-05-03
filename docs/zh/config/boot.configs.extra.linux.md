引用:
- [src/boot/linux.c](../../../src/boot/linux.c)
- [src/liux.c](../../../src/boot/linux.c)

# boot.configs.?.extra.abootimg

指定一个安卓启动镜像，从中加载内核、设备树、initrd、命令行

目标可以是文件或者是块设备

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: locate路径 [locates](locates.md)

类型: `STRING` (字符串)

支持的环境: `UEFI`

# boot.configs.?.extra.kernel

指定要加载的内核

目标可以是文件或者是块设备

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: locate路径 [locates](locates.md)

类型: `STRING` (字符串)

支持的环境: `UEFI`

# boot.configs.?.extra.dtb

指定要加载的dtb(设备树)

目标可以是文件或者是块设备

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: locate路径 [locates](locates.md)

类型: `STRING` (字符串)

支持的环境: `UEFI`

# boot.configs.?.extra.dtbo

指定要加载的dtbo

目标可以是文件或者是块设备

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: locate路径 [locates](locates.md)

类型: `STRING` (字符串) 或 `KEY` (项)

支持的环境: `UEFI`

# boot.configs.?.extra.initrd

指定要加载的初始化内存盘(initcpio/initrd/initramfs/ramdisk)

目标可以是文件或者是块设备

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: locate路径 [locates](locates.md)

类型: `STRING` (字符串) 或 `KEY` (项)

支持的环境: `UEFI`

# boot.configs.?.extra.cmdline

指定启动时的内核命令行

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

类型: `STRING` (字符串)

支持的环境: `UEFI`

# boot.configs.?.extra.splash

向设备树添加`/reserved-memory/splash_region`

指定一个bootloader预初始化的显示帧缓冲区(framebuffer)

类型: `KEY` (项)

支持的环境: `UEFI`

## boot.configs.?.extra.splash.base

指定显示缓冲区域起始

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: 物理内存地址

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

## boot.configs.?.extra.splash.start

指定显示缓冲区域起始

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: 物理内存地址

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

## boot.configs.?.extra.splash.size

指定显示缓冲区域大小

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: 物理内存大小

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

## boot.configs.?.extra.splash.end

指定显示缓冲区域结束

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: 物理内存地址

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

# boot.configs.?.extra.memory

指定内存区域

如果未指定，默认从`KernelFdtDxe`或者`MemoryMap`获取内存大小

类型: `KEY` (项)

支持的环境: `UEFI`

## boot.configs.?.extra.memory.?

指定内存区域

如果未指定，默认从`KernelFdtDxe`或者`MemoryMap`获取内存大小

类型: `KEY` (项)

支持的环境: `UEFI`

### boot.configs.?.extra.memory.?.base

指定内存区域起始

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: 物理内存地址

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

### boot.configs.?.extra.memory.?.start

指定内存区域起始

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: 物理内存地址

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

### boot.configs.?.extra.memory.?.size

指定内存区域大小

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: 物理内存大小

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

### boot.configs.?.extra.memory.?.end

指定内存区域结束

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: 物理内存地址

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

# boot.configs.?.extra.use_efi

是否使用UEFI和efistub启动Linux内核

如果值为`false`，则尝试以无UEFI的模式启动Linux内核，Linux内核将不会识别到UEFI环境。

该选项用于在UEFI上启动一些不支持efistub的Linux内核

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

默认: `true`

支持的环境: `UEFI`

# boot.configs.?.extra.skip_dtb

跳过加载dtb (设备树 Device Tree Blob)

将不会加载dtb，如果UEFI已配置了fdt表，则会使用UEFI的fdt表

对于某些平台，如果没有加载dtb并且未使用acpi，则会拒绝启动

注：指定了该选项将不会更新fdt，而且在某些平台可能不会添加initrd的位置

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

默认: `false`

支持的环境: `UEFI`

# boot.configs.?.extra.skip_dtbo

跳过加载dtbo (Device Tree Blob Overlay)

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

默认: `false`

支持的环境: `UEFI`

# boot.configs.?.extra.skip_initrd

跳过加载initrd (initramfs/ramdisk)

对于部分A/B分区的设备，启动镜像中的initrd为recovery，不使用initrd进入安卓系统，使用initrd则进入recovery

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

默认: `false`

支持的环境: `UEFI`

# boot.configs.?.extra.skip_efi_memory_map

不使用UEFI的内存表更新fdt设置内存大小(Memory Map)

此选项对于使用UEFI模式启动无效 (efistub)

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

默认: `false`

支持的环境: `UEFI`

# boot.configs.?.extra.skip_kernel_fdt_memory

跳过从`KernelFdtDxe`获取的内存大小

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

默认: `false`

支持的环境: `UEFI`

# boot.configs.?.extra.skip_kernel_fdt_cmdline

跳过从`KernelFdtDxe`获取的内核命令行

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

默认: `false`

支持的环境: `UEFI`

# boot.configs.?.extra.load_custom_address

指定是否将内核、initrd、设备树加载到指定的物理内存位置

如果AllocatePages分配的内存无法正常启动内核，则可用自行指定内存位置用于启动

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

默认: `false`

支持的环境: `UEFI`

# boot.configs.?.extra.match_kernel_fdt_model

匹配`KernelFdtDxe`中设备树的`model`

用于在使用多个dtb/dtbo时自动选择

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

默认: `false`

支持的环境: `UEFI`

# boot.configs.?.extra.ignore_dtbo_error

忽略处理dtbo时发生的错误

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

默认: `false`

支持的环境: `UEFI`

# boot.configs.?.extra.add_simplefb

指定是否往dtb(设备树)添加Simple FrameBuffer设备

目前可能不稳定，如出现问题则建议关闭

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

默认: `false`

支持的环境: `UEFI`

# boot.configs.?.extra.update_splash

指定是否更新dtb(设备树)中显示帧缓冲区的内存范围

目前可能不稳定，如出现问题则建议关闭

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

默认: `true`

支持的环境: `UEFI`

# boot.configs.?.extra.dtb_model

当有多个dtb(设备树)时，选择要使用的目标dtb(设备树)的model

未指定将会根据其它信息自动选择

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

类型: `STRING` (字符串) 或 `KEY` (项)

支持的环境: `UEFI`

# boot.configs.?.extra.dtbo_model

当有多个dtbo(设备树叠加层)时，选择要使用的目标dtbo(设备树叠加层)的model

未指定将会根据其它信息自动选择

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

类型: `STRING` (字符串) 或 `KEY` (项)

支持的环境: `UEFI`

# boot.configs.?.extra.dtb_compatible

当有多个dtb(设备树)时，选择要使用的目标dtb(设备树)的compatible

未指定将会根据其它信息自动选择

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

类型: `STRING` (字符串) 或 `KEY` (项)

支持的环境: `UEFI`

# boot.configs.?.extra.dtbo_compatible

当有多个dtbo(设备树叠加层)时，选择要使用的目标dtbo(设备树叠加层)的compatible

未指定将会根据其它信息自动选择

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

类型: `STRING` (字符串) 或 `KEY` (项)

支持的环境: `UEFI`

# boot.configs.?.extra.dtb_id

当有多个dtb(设备树)时，选择要使用的目标dtb(设备树)的ID

未指定将会根据其它信息自动选择

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

# boot.configs.?.extra.dtbo_id

当有多个dtbo(设备树叠加层)时，选择要使用的目标dtbo(设备树叠加层)的ID

未指定将会根据其它信息自动选择

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

# boot.configs.?.extra.soc_id

指定高通平台的ChipInfo SocID，用于自动选择dtb和dtbo

未指定将会自动从`ChipInfoDxe`获取

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

# boot.configs.?.extra.soc_rev

指定高通平台的ChipInfo SocRev，用于自动选择dtb和dtbo

未指定将会自动从`ChipInfoDxe`获取

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

# boot.configs.?.extra.foundry_id

指定高通平台的ChipInfo FoundryID，用于自动选择dtb和dtbo

未指定将会自动从`ChipInfoDxe`获取

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

# boot.configs.?.extra.variant_major

指定高通平台的PlatformInfo VariantMajor，用于自动选择dtb和dtbo

未指定将会自动从`PlatformInfoDxe`获取

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

# boot.configs.?.extra.variant_minor

指定高通平台的PlatformInfo VariantMinor，用于自动选择dtb和dtbo

未指定将会自动从`PlatformInfoDxe`获取

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

# boot.configs.?.extra.variant_id

指定高通平台的PlatformInfo VariantID，用于自动选择dtb和dtbo

未指定将会自动从`PlatformInfoDxe`获取

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

# boot.configs.?.extra.subtype_id

指定高通平台的PlatformInfo SubtypeID，用于自动选择dtb和dtbo

未指定将会自动从`PlatformInfoDxe`获取

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

# boot.configs.?.extra.subtype_ddr

指定高通平台的PlatformInfo SubtypeDDR，用于自动选择dtb和dtbo

未指定将会自动从`PlatformInfoDxe`获取

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

# boot.configs.?.extra.screen_width

指定屏幕宽度(width)，用于添加Simple FrameBuffer设备

未指定将会自动从`GraphicalOutputProtocol`获取

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

# boot.configs.?.extra.screen_height

指定屏幕高度(height)，用于添加Simple FrameBuffer设备

未指定将会自动从`GraphicalOutputProtocol`获取

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

# boot.configs.?.extra.screen_stride

指定屏幕行长度(stride)，用于添加Simple FrameBuffer设备

未指定将会自动从`GraphicalOutputProtocol`获取

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

# boot.configs.?.extra.address

指定加载内核、initrd、设备树的物理内存区域

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

类型: `KEY` (项)

支持的环境: `UEFI`

## boot.configs.?.extra.address.load

指定默认的加载内核、initrd、设备树的物理内存区域

当[boot.configs.?.extra.address.kernel](#boot.configs.?.extra.address.kernel)、[boot.configs.?.extra.address.initrd](#boot.configs.?.extra.address.initrd)或[boot.configs.?.extra.address.dtb](#boot.configs.?.extra.address.dtb)未指定时，默认将使用该段地址

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

类型: `KEY` (项)

支持的环境: `UEFI`

### boot.configs.?.extra.address.load.base

指定加载内核、initrd、设备树的物理内存区域起始位置

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

取值: 物理内存地址

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

### boot.configs.?.extra.address.load.start

指定加载内核、initrd、设备树的物理内存区域起始位置

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

取值: 物理内存地址

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

### boot.configs.?.extra.address.load.size

指定加载内核、initrd、设备树的物理内存区域大小

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

取值: 物理内存大小

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

### boot.configs.?.extra.address.load.end

指定加载内核、initrd、设备树的物理内存区域结束位置

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

取值: 物理内存地址

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

## boot.configs.?.extra.address.kernel

指定加载内核的物理内存区域

未指定时，默认将使用[boot.configs.?.extra.address.load](#boot.configs.?.extra.address.load)的剩余可用空间

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

类型: `KEY` (项)

支持的环境: `UEFI`

### boot.configs.?.extra.address.kernel.base

指定加载内核的物理内存区域起始位置

未指定时，默认将使用[boot.configs.?.extra.address.load.base](#boot.configs.?.extra.address.load.base)的剩余可用空间

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

取值: 物理内存地址

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

### boot.configs.?.extra.address.kernel.start

指定加载内核的物理内存区域起始位置

未指定时，默认将使用[boot.configs.?.extra.address.load.start](#boot.configs.?.extra.address.load.start)的剩余可用空间

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

取值: 物理内存地址

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

### boot.configs.?.extra.address.kernel.size

指定加载内核的物理内存区域大小

未指定时，默认将使用[boot.configs.?.extra.address.load.size](#boot.configs.?.extra.address.load.size)的剩余可用空间

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

取值: 物理内存大小

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

### boot.configs.?.extra.address.kernel.end

指定加载内核的物理内存区域结束位置

未指定时，默认将使用[boot.configs.?.extra.address.load.end](#boot.configs.?.extra.address.load.end)的剩余可用空间

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

取值: 物理内存地址

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

## boot.configs.?.extra.address.initrd

指定加载initrd的物理内存位置

未指定时，默认将使用[boot.configs.?.extra.address.load](#boot.configs.?.extra.address.load)的剩余可用空间

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

类型: `KEY` (项)

支持的环境: `UEFI`

### boot.configs.?.extra.address.initrd.base

指定加载initrd的物理内存区域起始位置

未指定时，默认将使用[boot.configs.?.extra.address.load.base](#boot.configs.?.extra.address.load.base)的剩余可用空间

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

取值: 物理内存地址

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

### boot.configs.?.extra.address.initrd.start

指定加载initrd的物理内存区域起始位置

未指定时，默认将使用[boot.configs.?.extra.address.load.start](#boot.configs.?.extra.address.load.start)的剩余可用空间

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

取值: 物理内存地址

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

### boot.configs.?.extra.address.initrd.size

指定加载initrd的物理内存区域大小

未指定时，默认将使用[boot.configs.?.extra.address.load.size](#boot.configs.?.extra.address.load.size)的剩余可用空间

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

取值: 物理内存大小

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

### boot.configs.?.extra.address.initrd.end

指定加载initrd的物理内存区域结束位置

未指定时，默认将使用[boot.configs.?.extra.address.load.end](#boot.configs.?.extra.address.load.end)的剩余可用空间

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

取值: 物理内存地址

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

## boot.configs.?.extra.address.dtb

指定加载设备树的物理内存位置

未指定时，默认将使用[boot.configs.?.extra.address.load](#boot.configs.?.extra.address.load)的剩余可用空间

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

类型: `KEY` (项)

支持的环境: `UEFI`

### boot.configs.?.extra.address.dtb.base

指定加载设备树的物理内存区域起始位置

未指定时，默认将使用[boot.configs.?.extra.address.load.base](#boot.configs.?.extra.address.load.base)的剩余可用空间

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

取值: 物理内存地址

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

### boot.configs.?.extra.address.dtb.start

指定加载设备树的物理内存区域起始位置

未指定时，默认将使用[boot.configs.?.extra.address.load.start](#boot.configs.?.extra.address.load.start)的剩余可用空间

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

取值: 物理内存地址

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

### boot.configs.?.extra.address.dtb.size

指定加载设备树的物理内存区域大小

未指定时，默认将使用[boot.configs.?.extra.address.load.size](#boot.configs.?.extra.address.load.size)的剩余可用空间

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

取值: 物理内存大小

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

### boot.configs.?.extra.address.dtb.end

指定加载设备树的物理内存区域结束位置

未指定时，默认将使用[boot.configs.?.extra.address.load.end](#boot.configs.?.extra.address.load.end)的剩余可用空间

条件:
 - [boot.configs.?.mode](boot.configs.md)为`BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address)为`true`

取值: 物理内存地址

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`
