# logger

Special settings for logger service

Type: `KEY`

Supported environments: `LINUX` | `UEFI`

## logger.file_output

Write log to specified file

Values: locate path [locates](locates.md)

Type: `STRING`

Supported environments: `UEFI`

Reference:
 - [src/loggerd/client.c](../../../src/loggerd/client.c)

## logger.use_console

Whether to print logs to the terminal (`ConOut`) by default

Values: `true` or `false`

Type: `BOOLEAN`

Supported environments: `UEFI`

Default: Value of `PcdLoggerdUseConsole`

Reference:
 - [src/loggerd/client.c](../../../src/loggerd/client.c)

## logger.min_level

Min log level

Values: loggerd log level

| Logger Level  | Value  |
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

Type: `INTEGER`

Supported environments: `UEFI`

Default: Value of `PcdLoggerdMinLevel`

Reference:
 - [src/loggerd/client.c](../../../src/loggerd/client.c)

## logger.old_file

What to do with old log files

Values:

| Action   | Value |
|----------|-------|
| Truncate | 0     |
| Append   | 1     |
| Rename   | 2     |

Type: `INTEGER`

Supported environments: `UEFI`

Default: 0 (Truncate)

Reference:
 - [src/loggerd/client.c](../../../src/loggerd/client.c)
