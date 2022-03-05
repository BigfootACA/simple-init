# simple-init confd配置文档

## runtime

该键保留于程序运行时，不允许保存或从本地文件加载

## boot

用于储存simple-init启动选项，或启动菜单项

支持的环境: `LINUX` | `UEFI`

详细选项: [boot](boot.md)

## logger

用于储存Logger日志服务的特殊设置

支持的环境: `LINUX` | `UEFI`

详细选项: [logger](logger.md)

## uefi

用于储存UEFI下的特殊设置

支持的环境: `UEFI`

详细选项: [uefi](uefi.md)
