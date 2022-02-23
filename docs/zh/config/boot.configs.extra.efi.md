引用:
 - [src/boot/efi.c](../../../src/boot/efi.c)

# boot.configs.?.extra.efi_fv_guid

使用GUID指定一个FV(固件卷)中的efi

注: 该选项可能只在simple-init处于固件卷中生效

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_EFI`

取值: GUID

类型: `STRING` (字符串)

支持的环境: `UEFI`

# boot.configs.?.extra.efi_file

efi文件路径

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_EFI`

取值: locate路径 [locates.md](locates.md)

类型: `STRING` (字符串)

支持的环境: `UEFI`

# boot.configs.?.extra.options

传输给efi程序的参数选项 (`LoadOptions`)

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_EFI`

取值: UTF-8字符串或UTF-16字符串

类型: `STRING` (字符串)

支持的环境: `UEFI`

# boot.configs.?.extra.options_wchar

指定[boot.configs.?.extra.options](#boot.configs.?.extra.options)的数据类型

为`true`时，将[boot.configs.?.extra.options](#boot.configs.?.extra.options)的值视为 `UTF-8` `char?` (`CHAR8?`)

为`false`时，将[boot.configs.?.extra.options](#boot.configs.?.extra.options)的值视为 `UTF-16` `wchar_t?` (`CHAR16?`)

条件: [boot.configs.?.mode](boot.configs.md)为`BOOT_EFI`

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

默认: `false`

支持的环境: `UEFI`
