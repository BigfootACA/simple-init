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

## logger.min_level

最小的日志级别

取值: loggerd日志级别

| 日志级别      | 值     |
|---------------|--------|
| LEVEL_VERBOSE | 0xAE00 |
| LEVEL_DEBUG   | 0xAE01 |
| LEVEL_INFO    | 0xAE02 |
| LEVEL_NOTICE  | 0xAE04 |
| LEVEL_WARNING | 0xAE08 |
| LEVEL_ERROR   | 0xAE10 |
| LEVEL_CRIT    | 0xAE20 |
| LEVEL_ALERT   | 0xAE40 |
| LEVEL_EMERG   | 0xAE80 |

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

默认: `PcdLoggerdMinLevel`的值

引用:
 - [src/loggerd/client.c](../../../src/loggerd/client.c)

## logger.old_file

对于旧的日志文件的处理方式

取值:

| 行为   | 值 |
|--------|----|
| 截断   | 0  |
| 追加   | 1  |
| 重命名 | 2  |

类型: `INTEGER` (64位整型数字)

支持的环境: `UEFI`

默认: 0 (截断)

引用:
 - [src/loggerd/client.c](../../../src/loggerd/client.c)
