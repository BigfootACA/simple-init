# Linux target run native (for testing GUI Applications)

## 1. Build Main

```bash
bash scripts/build.sh -DENABLE_SDL2=ON
```
output: `build/init` (ELF Executable)

`ENABLE_SDL2`: enable SDL2 guidrv for GUI Applications

## 2. Run GUI Application

```bash
INIT_MAIN=guiapp build/init
```
NOTICE: need X11 environment

