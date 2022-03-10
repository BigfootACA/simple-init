# locates

Used to locate a block device or file

Usage:
 - /xxx/xxx

   The path is relative to the filesystem where simple-init is located

 - @aaaa:/xxxx/xxxx

   Will use the file in the filesystem pointed to by the tag named aaaa

 - #bbbb

   Will use the block device pointed to by the tag named bbbb

Type: `KEY`

Supported environments: `UEFI`

Reference:
 - [src/locate/locate.c](../../../src/locate/locate.c)

## locates.?

A new locate

Key name will be used as tag

Type: `KEY`

Supported environments: `UEFI`

Reference:
 - [src/locate/locate.c](../../../src/locate/locate.c)

### locates.?.by_esp

Must be an EFI System Partition (ESP)

Values: `true` or `false`

Type: `BOOLEAN`

Supported environments: `UEFI`

Default: `false`

Reference:
 - [src/locate/disk.c](../../../src/locate/disk.c)

### locates.?.by_disk_label

Matches the specified partition table (disk label) format

Values: `"mbr"` or `"gpt"`

Type: `STRING`

Supported environments: `UEFI`

Reference:
 - [src/locate/disk.c](../../../src/locate/disk.c)

### locates.?.by_device_path

Matches UEFI Device Path(`EFI_DEVICE_PATH_PROTOCOL`)

Type: `STRING`

Supported environments: `UEFI`

Reference:
 - [src/locate/disk.c](../../../src/locate/disk.c)

### locates.?.by_fs_label

Target file system volume label matching (LABEL)

Note: Must be a file system format supported by UEFI

Type: `STRING`

Supported environments: `UEFI`

Reference:
 - [src/locate/fs.c](../../../src/locate/fs.c)

### locates.?.by_file

The specified file exists in the target file system

Note: Must be a file system format supported by UEFI

Type: `STRING`

Supported environments: `UEFI`

Reference:
 - [src/locate/fs.c](../../../src/locate/fs.c)

### locates.?.by_gpt_name

The target's partition's GPT partition name matches (PARTLABEL)

Note: This condition implies that `locates.?.by_disk_label` is equal to `gpt`

Type: `STRING`

Supported environments: `UEFI`

Reference:
 - [src/locate/gpt.c](../../../src/locate/gpt.c)

### locates.?.by_gpt_guid

The GPT partition GUID of the target's partition matches (PARTUUID)

Note: This condition implies that `locates.?.by_disk_label` is equal to `gpt`

Values: GUID

Type: `STRING`

Supported environments: `UEFI`

Reference:
 - [src/locate/gpt.c](../../../src/locate/gpt.c)

### locates.?.by_gpt_type

The GPT partition type GUID of the target's partition matches

Note: This condition implies that `locates.?.by_disk_label` is equal to `gpt`

Values: GUID

Type: `STRING`

Supported environments: `UEFI`

Reference:
 - [src/locate/gpt.c](../../../src/locate/gpt.c)

### locates.?.by_mbr_active

Must be an MBR boot partition (activated)

Note: This condition implies that `locates.?.by_disk_label` is equal to `mbr`

Values: `true` or `false`

Type: `BOOLEAN`

Supported environments: `UEFI`

Default: `false`

Reference:
 - [src/locate/mbr.c](../../../src/locate/mbr.c)

### locates.?.by_mbr_type

The format of the target partition's MBR matches

Note: This condition implies that `locates.?.by_disk_label` is equal to `mbr`

Values: `1` - `255`

Type: `INTEGER`

Supported environments: `UEFI`

Default: `false`

Reference:
 - [src/locate/mbr.c](../../../src/locate/mbr.c)
