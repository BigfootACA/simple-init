# boot.configs

Boot items store

Type: `KEY`

Supported environments: `LINUX` | `UEFI`

## boot.configs.?

Single boot item

Type: `KEY`

Supported environments: `LINUX` | `UEFI`

### boot.configs.?.mode

Boot mode

Values: [include/boot.h@enum boot_mode](../../../include/boot.h)

| Val  | Name               | Environment    | Desription                                 | Handler function	                                      |
|------|--------------------|----------------|--------------------------------------------|-----------------------------------------------------------|
| 0x00 | `BOOT_NONE`        | `LINUX` `UEFI` | Boot item will not work                    | -                                                         |
| 0x01 | `BOOT_SWITCHROOT`  | `LINUX`        | Switch to the new root and run new init    | [src/boot/root.c](../../../src/boot/root.c)               |
| 0x02 | `BOOT_CHARGER`     | `LINUX`        | Shows a charging screen                    | [src/boot/charger.c](../../../src/boot/charger.c)         |
| 0x03 | `BOOT_KEXEC`       | `LINUX`        | Run a new kernel (not implemented)         | -                                                         |
| 0x04 | `BOOT_REBOOT`      | `LINUX` `UEFI` | Reboot                                     | [src/boot/reboot.c](../../../src/boot/reboot.c)           |
| 0x05 | `BOOT_POWEROFF`    | `LINUX` `UEFI` | Power-Off                                  | [src/boot/reboot.c](../../../src/boot/reboot.c)           |
| 0x06 | `BOOT_HALT`        | `LINUX`        | Halt                                       | [src/boot/reboot.c](../../../src/boot/reboot.c)           |
| 0x07 | `BOOT_SYSTEM`      | `LINUX`        | Continue to boot and run enabled services  | [src/boot/system.c](../../../src/boot/system.c)           |
| 0x08 | `BOOT_LINUX`       | `UEFI`         | Boot a Linux kernel                        | [src/boot/linux.c](../../../src/boot/linux.c)             |
| 0x09 | `BOOT_EFI`         | `UEFI`         | Run a efi program                          | [src/boot/efi.c](../../../src/boot/efi.c)                 |
| 0x0A | `BOOT_EXIT`        | `UEFI`         | Exit the boot menu to continue booting     | [src/boot/exit.c](../../../src/boot/exit.c)               |
| 0x0B | `BOOT_SIMPLE_INIT` | `UEFI`         | Enter the simple-init guiapp               | -                                                         |
| 0x0C | `BOOT_UEFI_OPTION` | `UEFI`         | Start a UEFI boot item                     | [src/boot/uefi_option.c](../../../src/boot/uefi_option.c) |
| 0xFF | `BOOT_FOLDER`      | `LINUX` `UEFI` | Declare a folder                           | -                                                         |

Type: `INTEGER`

Supported environments: `LINUX` | `UEFI`

### boot.configs.?.desc

The display name of the boot item

The value of this option will be used for the boot menu display option, the item name will be used instead if the option is not found

Type: `STRING`

Supported environments: `LINUX` | `UEFI`

Reference:
 - [src/boot/boot.c](../../../src/boot/boot.c)
 - [src/gui/interface/core/bootmenu.c](../../../src/gui/interface/core/bootmenu.c)

### boot.configs.?.show

Whether to show options in the boot menu

Hidden options can still be started by other means (such as [boot.current](boot.md), etc.)

Values: `true` or `false`

Type: `BOOLEAN`

Supported environments: `LINUX` | `UEFI`

Reference:
 - [src/boot/boot.c](../../../src/boot/boot.c)
 - [src/gui/interface/core/bootmenu.c](../../../src/gui/interface/core/bootmenu.c)

### boot.configs.?.enabled

Whether to enable boot item

Disabled options will be hidden and refuse to start

Values: `true` or `false`

Type: `BOOLEAN`

Supported environments: `LINUX` | `UEFI`

Reference:
 - [src/boot/boot.c](../../../src/boot/boot.c)
 - [src/gui/interface/core/bootmenu.c](../../../src/gui/interface/core/bootmenu.c)

### boot.configs.?.icon

The icon of the boot item, used to display in the boot menu

Values: filename under [root/usr/share/pixmaps/simple-init](../../../root/usr/share/pixmaps/simple-init), or an absolute path

Type: `STRING`

Supported environments: `LINUX` | `UEFI`

Reference:
 - [src/gui/interface/core/bootmenu.c](../../../src/gui/interface/core/bootmenu.c)

### boot.configs.?.splash

Splash picture when boot

Values: locate path [locates](locates.md)

Type: `STRING`

Supported environments: `UEFI`

Reference:
 - [src/boot/splash.c](../../../src/boot/splash.c)

### boot.configs.?.parent

The folder where the boot item is located

Values: [mode](#boot.configs.?.mode) under [boot.configs](#boot.configs) is the item of `BOOT_FOLDER`

Type: `STRING`

Supported environments: `LINUX` | `UEFI`

Reference:
 - [src/boot/boot.c](../../../src/boot/boot.c)
 - [src/gui/interface/core/bootmenu.c](../../../src/gui/interface/core/bootmenu.c)

### boot.configs.?.extra

Additional parameters for different modes of boot items

Type: `KEY`

Supported environments: `LINUX` | `UEFI`

Detailed options: [boot.configs.extra](boot.configs.extra.md)
