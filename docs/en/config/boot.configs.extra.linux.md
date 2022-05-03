Reference:
- [src/boot/linux.c](../../../src/boot/linux.c)
- [src/liux.c](../../../src/boot/linux.c)

# boot.configs.?.extra.abootimg

Specify an android boot image from which to load the kernel, dtb, initrd, cmdline

The target can be a file or a block device

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: locate path [locates](locates.md)

Type: `STRING`

Supported environments: `UEFI`

# boot.configs.?.extra.kernel

Specify the kernel to load

The target can be a file or a block device

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: locate path [locates](locates.md)

Type: `STRING`

Supported environments: `UEFI`

# boot.configs.?.extra.dtb

Specify the dtb (device tree blob) to load

The target can be a file or a block device

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: locate path [locates](locates.md)

Type: `STRING`

Supported environments: `UEFI`

# boot.configs.?.extra.dtbo

Specify the dtbo to load

The target can be a file or a block device

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: locate path [locates](locates.md)

Type: `STRING` or `KEY`

Supported environments: `UEFI`

# boot.configs.?.extra.initrd

Specify the initramfs (initcpio/initrd/ramdisk) to load

The target can be a file or a block device

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: locate path [locates](locates.md)

Type: `STRING` or `KEY`

Supported environments: `UEFI`

# boot.configs.?.extra.cmdline

Specifies the kernel command line

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Type: `STRING`

Supported environments: `UEFI`

# boot.configs.?.extra.splash

Add `/reserved-memory/splash_region` to device tree

Specify a bootloader pre-initialized display framebuffer

Type: `KEY`

Supported environments: `UEFI`

## boot.configs.?.extra.splash.base

Specifies the display buffer area start

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: physical memory address

Type: `INTEGER`

Supported environments: `UEFI`

## boot.configs.?.extra.splash.start

Specifies the display buffer area start

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: physical memory address

Type: `INTEGER`

Supported environments: `UEFI`

## boot.configs.?.extra.splash.size

Specifies the display buffer area size

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: physical memory size

Type: `INTEGER`

Supported environments: `UEFI`

## boot.configs.?.extra.splash.end

Specifies the display buffer area end

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: physical memory address

Type: `INTEGER`

Supported environments: `UEFI`

# boot.configs.?.extra.memory

Specify memory area

If not specified, the default memory size is obtained from `KernelFdtDxe` or `MemoryMap`

Type: `KEY`

Supported environments: `UEFI`

## boot.configs.?.extra.memory.?

Specify memory area

If not specified, the default memory size is obtained from `KernelFdtDxe` or `MemoryMap`

Type: `KEY`

Supported environments: `UEFI`

### boot.configs.?.extra.memory.?.base

Specify memory area start

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: physical memory address

Type: `INTEGER`

Supported environments: `UEFI`

### boot.configs.?.extra.memory.?.start

Specify memory area start

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: physical memory address

Type: `INTEGER`

Supported environments: `UEFI`

### boot.configs.?.extra.memory.?.size

Specify memory area size

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: physical memory size

Type: `INTEGER`

Supported environments: `UEFI`

### boot.configs.?.extra.memory.?.end

Specify memory area end

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: physical memory address

Type: `INTEGER`

Supported environments: `UEFI`

# boot.configs.?.extra.use_efi

Whether to use UEFI and efistub to boot the Linux kernel

If the value is `false`, try to boot the Linux kernel in non-UEFI mode, the Linux kernel will not recognize the UEFI environment.

This option is used to boot some Linux kernel that do not support efistub and UEFI

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: `true` or `false`

Type: `BOOLEAN`

Default: `true`

Supported environments: `UEFI`

# boot.configs.?.extra.skip_dtb

Skip loading dtb (Device Tree Blob)

Will not load dtb, if UEFI has configured fdt table, UEFI's fdt table will be used

For some platforms refuses to start if dtb is not loaded and acpi is not used

Note: Specifying this option will not update the fdt and may not add the location of the initrd on some platforms

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: `true` or `false`

Type: `BOOLEAN`

Default: `false`

Supported environments: `UEFI`

# boot.configs.?.extra.skip_dtbo

Skip loading dtbo (Device Tree Blob Overlay)

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: `true` or `false`

Type: `BOOLEAN`

Default: `false`

Supported environments: `UEFI`

# boot.configs.?.extra.skip_initrd

Skip loading initrd (initramfs/ramdisk)

For some devices with A/B partitions, the initrd in the boot image is recovery, enter the Android without initrd, and enter recovery with initrd

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: `true` or `false`

Type: `BOOLEAN`

Default: `false`

Supported environments: `UEFI`

# boot.configs.?.extra.skip_efi_memory_map

Update fdt settings memory size without using UEFI's memory table (Memory Map)

This option has no effect on booting with UEFI mode (efistub)

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: `true` or `false`

Type: `BOOLEAN`

Default: `false`

Supported environments: `UEFI`

# boot.configs.?.extra.skip_kernel_fdt_memory

Skip memory size obtained from `KernelFdtDxe`

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: `true` or `false`

Type: `BOOLEAN`

Default: `false`

Supported environments: `UEFI`

# boot.configs.?.extra.skip_kernel_fdt_cmdline

Skip the kernel command line obtained from `KernelFdtDxe`

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: `true` or `false`

Type: `BOOLEAN`

Default: `false`

Supported environments: `UEFI`

# boot.configs.?.extra.load_custom_address

Specifies whether to load the kernel, initrd, device tree into the specified physical memory location

If the memory allocated by AllocatePages cannot start the kernel normally, you can specify the memory location for startup by yourself

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: `true` or `false`

Type: `BOOLEAN`

Default: `false`

Supported environments: `UEFI`

# boot.configs.?.extra.match_kernel_fdt_model

Matches the `model` of device tree in `KernelFdtDxe`

For automatic selection when using multiple dtb/dtbo

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: `true` or `false`

Type: `BOOLEAN`

Default: `false`

Supported environments: `UEFI`

# boot.configs.?.extra.ignore_dtbo_error

Ignore errors while processing dtbo

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: `true` or `false`

Type: `BOOLEAN`

Default: `false`

Supported environments: `UEFI`

# boot.configs.?.extra.add_simplefb

Specifies whether to add Simple FrameBuffer device to dtb (Device Tree)

It may be unstable at present, if there is a problem, it is recommended to close

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: `true` or `false`

Type: `BOOLEAN`

Default: `false`

Supported environments: `UEFI`

# boot.configs.?.extra.update_splash

Specifies whether to update the memory region of the display frame buffer in the dtb (Device Tree)

It may be unstable at present, if there is a problem, it is recommended to close

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Values: `true` or `false`

Type: `BOOLEAN`

Default: `true`

Supported environments: `UEFI`

# boot.configs.?.extra.dtb_model

When there are multiple dtb (Device Tree), select the model of the target dtb (Device Tree) to use

Unspecified will be automatically selected based on other information

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Type: `STRING` or `KEY`

Supported environments: `UEFI`

# boot.configs.?.extra.dtbo_model

When there are multiple dtbo (Device Tree Overlay), select the model of the target dtbo (Device Tree Overlay) to use

Unspecified will be automatically selected based on other information

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Type: `STRING` or `KEY`

Supported environments: `UEFI`

# boot.configs.?.extra.dtb_compatible

When there are multiple dtb (Device Tree), select the compatible of the target dtb (Device Tree) to use

Unspecified will be automatically selected based on other information

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Type: `STRING` or `KEY`

Supported environments: `UEFI`

# boot.configs.?.extra.dtbo_compatible

When there are multiple dtbo (Device Tree Overlay), select the compatible of the target dtbo (Device Tree Overlay) to use

Unspecified will be automatically selected based on other information

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Type: `STRING` or `KEY`

Supported environments: `UEFI`

# boot.configs.?.extra.dtb_id

When there are multiple dtb (Device Tree), select the ID of the target dtb (Device Tree) to use

Unspecified will be automatically selected based on other information

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Type: `INTEGER`

Supported environments: `UEFI`

# boot.configs.?.extra.dtbo_id

When there are multiple dtbo (Device Tree Overlay), select the ID of the target dtbo (Device Tree Overlay) to use

Unspecified will be automatically selected based on other information

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Type: `INTEGER`

Supported environments: `UEFI`

# boot.configs.?.extra.soc_id

Specify the ChipInfo SocID of Qualcomm platform for automatic selection of dtb and dtbo

Unspecified will be automatically obtained from `ChipInfoDxe`

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Type: `INTEGER`

Supported environments: `UEFI`

# boot.configs.?.extra.soc_rev

Specify the ChipInfo SocRev of Qualcomm platform for automatic selection of dtb and dtbo

Unspecified will be automatically obtained from `ChipInfoDxe`

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Type: `INTEGER`

Supported environments: `UEFI`

# boot.configs.?.extra.foundry_id

Specify the ChipInfo FoundryID of Qualcomm platform for automatic selection of dtb and dtbo

Unspecified will be automatically obtained from `ChipInfoDxe`

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Type: `INTEGER`

Supported environments: `UEFI`

# boot.configs.?.extra.variant_major

Specify the PlatformInfo VariantMajor of Qualcomm platform for automatic selection of dtb and dtbo

Unspecified will be automatically obtained from `PlatformInfoDxe`

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Type: `INTEGER`

Supported environments: `UEFI`

# boot.configs.?.extra.variant_minor

Specify the PlatformInfo VariantMinor of Qualcomm platform for automatic selection of dtb and dtbo

Unspecified will be automatically obtained from `PlatformInfoDxe`

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Type: `INTEGER`

Supported environments: `UEFI`

# boot.configs.?.extra.variant_id

Specify the PlatformInfo VariantID of Qualcomm platform for automatic selection of dtb and dtbo

Unspecified will be automatically obtained from `PlatformInfoDxe`

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Type: `INTEGER`

Supported environments: `UEFI`

# boot.configs.?.extra.subtype_id

Specify the PlatformInfo SubtypeID of Qualcomm platform for automatic selection of dtb and dtbo

Unspecified will be automatically obtained from `PlatformInfoDxe`

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Type: `INTEGER`

Supported environments: `UEFI`

# boot.configs.?.extra.subtype_ddr

Specify the PlatformInfo SubtypeDDR of Qualcomm platform for automatic selection of dtb and dtbo

Unspecified will be automatically obtained from `PlatformInfoDxe`

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Type: `INTEGER`

Supported environments: `UEFI`

# boot.configs.?.extra.screen_width

Specify the screen width for adding Simple FrameBuffer devices

Unspecified will automatically get from `GraphicalOutputProtocol`

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Type: `INTEGER`

Supported environments: `UEFI`

# boot.configs.?.extra.screen_height

Specify the screen height for adding Simple FrameBuffer devices

Unspecified will automatically get from `GraphicalOutputProtocol`

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Type: `INTEGER`

Supported environments: `UEFI`

# boot.configs.?.extra.screen_stride

Specify the screen stride for adding Simple FrameBuffer devices

Unspecified will automatically get from `GraphicalOutputProtocol`

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`

Type: `INTEGER`

Supported environments: `UEFI`

# boot.configs.?.extra.address

Specifies physical memory area to load the kernel, initrd, device tree

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Type: `KEY`

Supported environments: `UEFI`

## boot.configs.?.extra.address.load

Specifies default physical memory area where the kernel, initrd, and device tree are loaded

When [boot.configs.?.extra.address.kernel](#boot.configs.?.extra.address.kernel),[boot.configs.?.extra.address.initrd](#boot.configs.?.extra.address.initrd), [boot.configs.?.extra.address.dtb](#boot.configs.?.extra.address.dtb) are not specified, this address will be used by default

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Type: `KEY`

Supported environments: `UEFI`

### boot.configs.?.extra.address.load.base

Specify physical memory start location to load kernel, initrd, device tree

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Values: physical memory address

Type: `INTEGER`

Supported environments: `UEFI`

### boot.configs.?.extra.address.load.start

Specify physical memory start location to load kernel, initrd, device tree

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Values: physical memory address

Type: `INTEGER`

Supported environments: `UEFI`

### boot.configs.?.extra.address.load.size

Specify physical memory size location to load kernel, initrd, device tree

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Values: physical memory size

Type: `INTEGER`

Supported environments: `UEFI`

### boot.configs.?.extra.address.load.end

Specify physical memory end location to load kernel, initrd, device tree

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Values: physical memory address

Type: `INTEGER`

Supported environments: `UEFI`

## boot.configs.?.extra.address.kernel

Specifies physical memory area to load kernel

When not specified, the remaining free space of [boot.configs.?.extra.address.load](#boot.configs.?.extra.address.load) will be used by default

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Type: `KEY`

Supported environments: `UEFI`

### boot.configs.?.extra.address.kernel.base

Specifies physical memory start location to load kernel

When not specified, the remaining free space of [boot.configs.?.extra.address.load.base](#boot.configs.?.extra.address.load.base) will be used by default

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Values: physical memory address

Type: `INTEGER`

Supported environments: `UEFI`

### boot.configs.?.extra.address.kernel.start

Specifies physical memory start location to load kernel

When not specified, the remaining free space of [boot.configs.?.extra.address.load.start](#boot.configs.?.extra.address.load.start) will be used by default

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Values: physical memory address

Type: `INTEGER`

Supported environments: `UEFI`

### boot.configs.?.extra.address.kernel.size

Specifies physical memory size to load kernel

When not specified, the remaining free space of [boot.configs.?.extra.address.load.size](#boot.configs.?.extra.address.load.size) will be used by default

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Values: physical memory size

Type: `INTEGER`

Supported environments: `UEFI`

### boot.configs.?.extra.address.kernel.end

Specifies physical memory end location to load kernel

When not specified, the remaining free space of [boot.configs.?.extra.address.load.end](#boot.configs.?.extra.address.load.end) will be used by default

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Values: physical memory address

Type: `INTEGER`

Supported environments: `UEFI`

## boot.configs.?.extra.address.initrd

Specifies physical memory area to load initrd

When not specified, the remaining free space of [boot.configs.?.extra.address.load](#boot.configs.?.extra.address.load) will be used by default

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Type: `KEY`

Supported environments: `UEFI`

### boot.configs.?.extra.address.initrd.base

Specifies physical memory start location to load initrd

When not specified, the remaining free space of [boot.configs.?.extra.address.load.base](#boot.configs.?.extra.address.load.base) will be used by default

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Values: physical memory address

Type: `INTEGER`

Supported environments: `UEFI`

### boot.configs.?.extra.address.initrd.start

Specifies physical memory start location to load initrd

When not specified, the remaining free space of [boot.configs.?.extra.address.load.start](#boot.configs.?.extra.address.load.start) will be used by default

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Values: physical memory address

Type: `INTEGER`

Supported environments: `UEFI`

### boot.configs.?.extra.address.initrd.size

Specifies physical memory size to load initrd

When not specified, the remaining free space of [boot.configs.?.extra.address.load.size](#boot.configs.?.extra.address.load.size) will be used by default

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Values: physical memory size

Type: `INTEGER`

Supported environments: `UEFI`

### boot.configs.?.extra.address.initrd.end

Specifies physical memory end location to load initrd

When not specified, the remaining free space of [boot.configs.?.extra.address.load.end](#boot.configs.?.extra.address.load.end) will be used by default

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Values: physical memory address

Type: `INTEGER`

Supported environments: `UEFI`

## boot.configs.?.extra.address.dtb

Specifies physical memory area to load dtb

When not specified, the remaining free space of [boot.configs.?.extra.address.load](#boot.configs.?.extra.address.load) will be used by default

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Type: `KEY`

Supported environments: `UEFI`

### boot.configs.?.extra.address.dtb.base

Specifies physical memory start location to load dtb

When not specified, the remaining free space of [boot.configs.?.extra.address.load.base](#boot.configs.?.extra.address.load.base) will be used by default

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Values: physical memory address

Type: `INTEGER`

Supported environments: `UEFI`

### boot.configs.?.extra.address.dtb.start

Specifies physical memory start location to load dtb

When not specified, the remaining free space of [boot.configs.?.extra.address.load.start](#boot.configs.?.extra.address.load.start) will be used by default

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Values: physical memory address

Type: `INTEGER`

Supported environments: `UEFI`

### boot.configs.?.extra.address.dtb.size

Specifies physical memory size to load dtb

When not specified, the remaining free space of [boot.configs.?.extra.address.load.size](#boot.configs.?.extra.address.load.size) will be used by default

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Values: physical memory size

Type: `INTEGER`

Supported environments: `UEFI`

### boot.configs.?.extra.address.dtb.end

Specifies physical memory end location to load dtb

When not specified, the remaining free space of [boot.configs.?.extra.address.load.end](#boot.configs.?.extra.address.load.end) will be used by default

Condition:
 - [boot.configs.?.mode](boot.configs.md) is `BOOT_LINUX`
 - [boot.configs.?.extra.load_custom_address](#boot.configs.?.extra.load_custom_address) is `true`

Values: physical memory address

Type: `INTEGER`

Supported environments: `UEFI`
