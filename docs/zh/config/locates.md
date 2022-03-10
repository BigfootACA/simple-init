# locates

用于定位一个块设备或者文件

使用方法:
 - /xxx/xxx

   该路径相对于simple-init所在的文件系统

 - @aaaa:/xxxx/xxxx

   将使用名为aaaa的tag指向的文件系统中的文件

 - #bbbb

   将使用名为bbbb的tag指向的块设备

类型: `KEY` (项)

支持的环境: `UEFI`

引用:
 - [src/locate/locate.c](../../../src/locate/locate.c)

## locates.?

一个新的locate

项名将作为tag

类型: `KEY` (项)

支持的环境: `UEFI`

引用:
 - [src/locate/locate.c](../../../src/locate/locate.c)

### locates.?.by_esp

必须是一个EFI系统分区(ESP)

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

支持的环境: `UEFI`

默认: `false`

引用:
 - [src/locate/disk.c](../../../src/locate/disk.c)

### locates.?.by_disk_label

匹配指定的分区表格式

取值: `"mbr"` 或 `"gpt"`

类型: `STRING` (字符串)

支持的环境: `UEFI`

引用:
 - [src/locate/disk.c](../../../src/locate/disk.c)

### locates.?.by_device_path

匹配UEFI的设备路径(`EFI_DEVICE_PATH_PROTOCOL`)

类型: `STRING` (字符串)

支持的环境: `UEFI`

引用:
 - [src/locate/disk.c](../../../src/locate/disk.c)

### locates.?.by_fs_label

目标文件系统卷标匹配(LABEL)

注: 必须是UEFI所支持的文件系统格式

类型: `STRING` (字符串)

支持的环境: `UEFI`

引用:
 - [src/locate/fs.c](../../../src/locate/fs.c)

### locates.?.by_file

目标文件系统存在指定文件

注: 必须是UEFI所支持的文件系统格式

类型: `STRING` (字符串)

支持的环境: `UEFI`

引用:
 - [src/locate/fs.c](../../../src/locate/fs.c)

### locates.?.by_gpt_name

目标的分区的GPT分区名称匹配(PARTLABEL)

注: 该条件隐含`locates.?.by_disk_label`等于`gpt`

类型: `STRING` (字符串)

支持的环境: `UEFI`

引用:
 - [src/locate/gpt.c](../../../src/locate/gpt.c)

### locates.?.by_gpt_guid

目标的分区的GPT分区GUID匹配(PARTUUID)

注: 该条件隐含`locates.?.by_disk_label`等于`gpt`

取值: GUID

类型: `STRING` (字符串)

支持的环境: `UEFI`

引用:
 - [src/locate/gpt.c](../../../src/locate/gpt.c)

### locates.?.by_gpt_type

目标的分区的GPT分区类型GUID匹配

注: 该条件隐含`locates.?.by_disk_label`等于`gpt`

取值: GUID

类型: `STRING` (字符串)

支持的环境: `UEFI`

引用:
 - [src/locate/gpt.c](../../../src/locate/gpt.c)

### locates.?.by_mbr_active

必须是一个MBR启动分区（被激活）

注: 该条件隐含`locates.?.by_disk_label`等于`mbr`

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

支持的环境: `UEFI`

默认: `false`

引用:
 - [src/locate/mbr.c](../../../src/locate/mbr.c)

### locates.?.by_mbr_type

目标分区的MBR的格式匹配

注: 该条件隐含`locates.?.by_disk_label`等于`mbr`

取值: `1` - `255`

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

默认: `false`

引用:
 - [src/locate/mbr.c](../../../src/locate/mbr.c)
