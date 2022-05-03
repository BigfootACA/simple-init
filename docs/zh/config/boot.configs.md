# boot.configs

启动项存储

类型: `KEY` (项)

支持的环境: `LINUX` | `UEFI`

## boot.configs.?

单个启动项

类型: `KEY` (项)

支持的环境: `LINUX` | `UEFI`

### boot.configs.?.mode

启动项的模式

取值: [include/boot.h@enum boot_mode](../../../include/boot.h)

| 值   | 名称               | 可用环境       | 解释                                       | 处理函数                                                  |
|------|--------------------|----------------|--------------------------------------------|-----------------------------------------------------------|
| 0x00 | `BOOT_NONE`        | `LINUX` `UEFI` | 选项将不起作用                             | -                                                         |
| 0x01 | `BOOT_SWITCHROOT`  | `LINUX`        | 切换到新的根目录并执行新的init以便继续启动 | [src/boot/root.c](../../../src/boot/root.c)               |
| 0x02 | `BOOT_CHARGER`     | `LINUX`        | 显示一个正在充电的界面                     | [src/boot/charger.c](../../../src/boot/charger.c)         |
| 0x03 | `BOOT_KEXEC`       | `LINUX`        | 运行新的内核（未实现）                     | -                                                         |
| 0x04 | `BOOT_REBOOT`      | `LINUX` `UEFI` | 重启                                       | [src/boot/reboot.c](../../../src/boot/reboot.c)           |
| 0x05 | `BOOT_POWEROFF`    | `LINUX` `UEFI` | 关机                                       | [src/boot/reboot.c](../../../src/boot/reboot.c)           |
| 0x06 | `BOOT_HALT`        | `LINUX`        | 关机                                       | [src/boot/reboot.c](../../../src/boot/reboot.c)           |
| 0x07 | `BOOT_SYSTEM`      | `LINUX`        | 继续启动系统，运行所有被启用的服务         | [src/boot/system.c](../../../src/boot/system.c)           |
| 0x08 | `BOOT_LINUX`       | `UEFI`         | 启动一个Linux内核                          | [src/boot/linux.c](../../../src/boot/linux.c)             |
| 0x09 | `BOOT_EFI`         | `UEFI`         | 启动一个.efi程序                           | [src/boot/efi.c](../../../src/boot/efi.c)                 |
| 0x0A | `BOOT_EXIT`        | `UEFI`         | 退出启动菜单以便UEFI继续启动               | [src/boot/exit.c](../../../src/boot/exit.c)               |
| 0x0B | `BOOT_SIMPLE_INIT` | `UEFI`         | 进入simple-init图形界面guiapp              | -                                                         |
| 0x0C | `BOOT_UEFI_OPTION` | `UEFI`         | 启动一个UEFI启动项                         | [src/boot/uefi_option.c](../../../src/boot/uefi_option.c) |
| 0xFF | `BOOT_FOLDER`      | `LINUX` `UEFI` | 该选项不会实际启动，只用于声明一个文件夹   | -                                                         |

类型: `INTEGER` (64位整型数字)

支持的环境: `LINUX` | `UEFI`

### boot.configs.?.desc

启动项的显示名称

该选项的值将用于启动菜单显示选项，找不到该选项则使用项名称代替

类型: `STRING` (字符串)

支持的环境: `LINUX` | `UEFI`

引用:
 - [src/boot/boot.c](../../../src/boot/boot.c)
 - [src/gui/interface/core/bootmenu.c](../../../src/gui/interface/core/bootmenu.c)

### boot.configs.?.show

是否在启动菜单显示选项

被隐藏的选项仍然可以通过其它途径启动（如[boot.current](boot.md)等）

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

支持的环境: `LINUX` | `UEFI`

引用:
 - [src/boot/boot.c](../../../src/boot/boot.c)
 - [src/gui/interface/core/bootmenu.c](../../../src/gui/interface/core/bootmenu.c)

### boot.configs.?.enabled

是否启用启动项

被禁用的选项将被隐藏，并拒绝启动

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

支持的环境: `LINUX` | `UEFI`

引用:
 - [src/boot/boot.c](../../../src/boot/boot.c)
 - [src/gui/interface/core/bootmenu.c](../../../src/gui/interface/core/bootmenu.c)

### boot.configs.?.icon

启动项的图标，用于显示于启动菜单

取值: [root/usr/share/pixmaps/simple-init](../../../root/usr/share/pixmaps/simple-init) 下的文件名，或绝对路径

类型: `STRING` (字符串)

支持的环境: `LINUX` | `UEFI`

引用:
 - [src/gui/interface/core/bootmenu.c](../../../src/gui/interface/core/bootmenu.c)

### boot.configs.?.splash

启动时的过场图片

取值: locate路径 [locates](locates.md)

类型: `STRING` (字符串)

支持的环境: `UEFI`

引用:
 - [src/boot/splash.c](../../../src/boot/splash.c)

### boot.configs.?.parent

启动项所处的文件夹

取值: [boot.configs](#boot.configs)下[mode](#boot.configs.?.mode)为`BOOT_FOLDER`的项

类型: `STRING` (字符串)

支持的环境: `LINUX` | `UEFI`

引用:
 - [src/boot/boot.c](../../../src/boot/boot.c)
 - [src/gui/interface/core/bootmenu.c](../../../src/gui/interface/core/bootmenu.c)

### boot.configs.?.extra

用于不同类型的启动项额外参数

类型: `KEY` (项)

支持的环境: `LINUX` | `UEFI`

详细选项: [boot.configs.extra](boot.configs.extra.md)
