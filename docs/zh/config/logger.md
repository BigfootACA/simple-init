# logger

用于储存Logger日志服务的特殊设置

类型: `KEY` (项)

支持的环境: `LINUX` | `UEFI`

## logger.file_output

将日志写入到指定文件

取值: locate路径 [locates](locates.md)

类型: `STRING` (字符串)

支持的环境: `UEFI`

引用:
 - [src/loggerd/client.c](../../../src/loggerd/client.c)

## logger.use_console

默认是否向终端(`ConOut`)打印日志

取值: `true` 或 `false`

类型: `BOOLEAN` (布尔值)

支持的环境: `UEFI`

默认: `PcdLoggerdUseConsole`的值

引用:
 - [src/loggerd/client.c](../../../src/loggerd/client.c)
