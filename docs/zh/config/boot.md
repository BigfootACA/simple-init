# boot

用于储存simple-init启动选项，或启动菜单项

类型: `KEY` (项)

支持的环境: `LINUX` | `UEFI`

## boot.default

保存启动菜单的默认选项，该项在运行时不会被更改

取值: [boot.configs](boot.configs.md) 下的任何子项名称

类型: `STRING` (字符串)

支持的环境: `LINUX` | `UEFI`

默认:
 - 在`Linux`环境: `"system"`
 - 在`UEFI`环境: `"continue"`

引用:
 - [src/boot/boot.c](../../../src/boot/boot.c)
 - [src/boot/bootdef.c](../../../src/boot/bootdef.c)
 - [src/gui/interface/core/bootmenu.c](../../../src/gui/interface/core/bootmenu.c)

## boot.second

保存启动菜单的第二默认选项，该项在运行时不会被更改。

当[boot.default](#boot.default)执行失败时会自动尝试该选项

取值: [boot.configs](boot.configs.md) 下的任何子项名称

类型: `STRING` (字符串)

支持的环境: `LINUX`

默认: `"system"`

引用:
 - [src/boot/boot.c](../../../src/boot/boot.c)
 - [src/boot/bootdef.c](../../../src/boot/bootdef.c)

## boot.current

启动菜单的即将启动的目标启动项，该项在运行时会被启动菜单更改。该项也可以用于保存当前启动项以供下一次启动使用

当找不到该选项时将会使用`boot.default`代替

取值: [boot.configs](boot.configs.md) 下的任何子项名称

类型: `STRING` (字符串)

支持的环境: `LINUX` | `UEFI`

默认:
 - 在`Linux`环境，内核命令行指定了`root=`、`rootblk=`等，则会设置为 `"switchroot"`
 - 在`Linux`环境，内核命令行指定了`androidboot.mode=charger`等，则会设置为 `"charger"`

引用:
 - [src/boot/boot.c](../../../src/boot/boot.c)
 - [src/cmdline/root.c](../../../src/cmdline/root.c)
 - [src/cmdline/androidboot.c](../../../src/cmdline/androidboot.c)
 - [src/gui/interface/core/bootmenu.c](../../../src/gui/interface/core/bootmenu.c)

## boot.timeout

启动菜单自动启动的倒计时

小于或等于0时会直接启动，不进行计时

取值: 秒

类型: `INTEGER` (64位整型数字)

支持的环境: `LINUX` | `UEFI`

默认: `10`

引用:
 - [src/gui/interface/core/bootmenu.c](../../../src/gui/interface/core/bootmenu.c)

## boot.timeout_text

启动菜单自动启动的倒计时的显示文本

`%d`将会替换为当前倒计时剩余时间（只能指定一次）

类型: `STRING` (字符串)

支持的环境: `LINUX` | `UEFI`

默认: 被翻译的 `"Auto boot in %d seconds"`

引用:
 - [src/gui/interface/core/bootmenu.c](../../../src/gui/interface/core/bootmenu.c)

## boot.title

启动菜单的标题

类型: `STRING` (字符串)

支持的环境: `LINUX` | `UEFI`

默认: 被翻译的 `"Boot Menu"`

引用:
 - [src/gui/interface/core/bootmenu.c](../../../src/gui/interface/core/bootmenu.c)

## boot.uefi_bootmenu

指定是否会从UEFI启动管理器获取启动项并加入启动菜单。

启用后枚举出的启动项不会保存。

枚举出的启动项以`uefi_bootXXXX`命名

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

支持的环境: `UEFI`

默认: `true`

引用:
 - [src/boot/bootdef.c](../../../src/boot/bootdef.c)

## boot.uefi_probe

指定是否自动搜索所有的分区以寻找可能安装的操作系统并加入启动菜单。

启用后找到的启动项不会保存。

找到出的启动项以`prober-X`命名

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

支持的环境: `UEFI`

默认: `true`

引用:
 - [src/boot/prober.c](../../../src/boot/prober.c)

## boot.console_log

启动时是否在终端显示日志

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

支持的环境: `UEFI`

默认: `true`

引用:
 - [src/boot/boot.c](../../../src/boot/boot.c)
 - [src/gui/interface/apps/boot_linux.c](../../../src/gui/interface/apps/boot_linux.c)

## boot.configs

启动项存储

类型: `KEY` (项)

支持的环境: `LINUX` | `UEFI`

详细选项: [boot.configs](boot.configs.md)
