# Linux target run native (for testing GUI Applications)

## 1. Build Main

```bash
bash scripts/build.sh -DENABLE_SDL2=ON
```
output: `build/simple-init` (ELF Executable)

`ENABLE_SDL2`: enable SDL2 guidrv for GUI Applications

## 2. Run GUI Application

```bash
build/simple-init guiapp
```
NOTICE: need X11 environment

