# Linux目标本地运行 (用于测试GUI程序)

## 1. 编译主程序

```bash
bash scripts/build.sh -DENABLE_SDL2=ON
```

输出: `build/init` (ELF可执行文件)

`ENABLE_SDL2`: enable SDL2 guidrv for GUI Applications

## 2. 运行GUI程序

```bash
INIT_MAIN=guiapp build/init
```

注意: 需要X11桌面环境
