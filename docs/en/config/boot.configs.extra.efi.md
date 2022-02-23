Reference:
 - [src/boot/efi.c](../../../src/boot/efi.c)

# boot.configs.?.extra.efi_fv_guid

Specify an efi file in an FV (Firmware Volume) with a GUID.

Notice: This option may only take effect if simple-init is in the firmware volume

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_EFI`

Values: GUID

Type: `STRING`

Supported environments: `UEFI`

# boot.configs.?.extra.efi_file

efi file path

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_EFI`

Values: locate path [locates.md](locates.md)

Type: `STRING`

Supported environments: `UEFI`

# boot.configs.?.extra.options

Options passed to the efi program (`LoadOptions`)

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_EFI`

Values: UTF-8 string or UTF-16 string

Type: `STRING`

Supported environments: `UEFI`

# boot.configs.?.extra.options_wchar

Set [boot.configs.?.extra.options](#boot.configs.?.extra.options) data type

If `true`, the value of [boot.configs.?.extra.options](#boot.configs.?.extra.options) is treated as `UTF-8` `char?` (`CHAR8?`)

if `false`, the value of [boot.configs.?.extra.options](#boot.configs.?.extra.options) is treated as `UTF-16` `wchar_t?` (`CHAR16?`)

Condition: [boot.configs.?.mode](boot.configs.md) is `BOOT_EFI`

Values: `true` or `false`

Type: `BOOLEAN`

Default: `false`

Supported environments: `UEFI`
