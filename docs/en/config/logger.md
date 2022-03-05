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
