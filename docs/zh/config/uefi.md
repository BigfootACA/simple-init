# uefi

用于储存UEFI下的特殊设置

类型: `KEY` (项)

支持的环境: `UEFI`

## uefi.drivers

自动加载指定的UEFI DXE驱动

类型: `KEY` (项)

支持的环境: `UEFI`

引用:
 - [src/boot/drivers.c](../../../src/boot/drivers.c)

## uefi.drivers.?

自动加载指定的UEFI DXE驱动

取值: locate路径 [locates](locates.md)

类型: `STRING` (字符串)

支持的环境: `UEFI`

引用:
 - [src/boot/drivers.c](../../../src/boot/drivers.c)
