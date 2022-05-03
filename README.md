## A simple init for linux (maybe it can also run under UEFI)

### Description

A linux system in a single binary (Stuffed with strange things)

Some references of this project may not meet the license, if you find any, please help me to correct it.

### WARNING

This project has not been completed and may not work properly.

### Depends

| Library     | Project     | Description                |
|-------------|-------------|----------------------------|
| blkid       | util-linux  | used to find block devices |
| mount       | util-linux  | mount anything             |
| fdisk       | util-linux  | for partition manager      |
| kmod        | kmod        | load kernel modules        |
| readline    | readline    | builtin initshell support  |
| gtk3        | gtk         | GUI on GTK-3 for debugging |
| sdl2        | sdl         | GUI on SDL2 for debugging  |
| libdrm      | libdrm      | GUI on DRM                 |
| lvgl        | lvgl        | GUI support                |
| wqy         | wenquanyi   | WQY-MicroHei Font for GUI  |
| fontawesone | fontawesone | Font Awesome Font for GUI  |
| freetype2   | freetype    | GUI load TrueType fonts    |
| lodepng     | lodepng     | GUI load png pictures      |
| nanosvg     | nanosvg     | GUI load svg pictures      |
| libjpeg     | libjpeg     | GUI load jpeg pictures     |
| vncserver   | libvnc      | GUI over VNC Server        |
| hivex       | libguestfs  | Windows Registry/BCD Edit  |
| json-c      | json-c      | JSON Format Support        |
| mxml        | mxml        | XML Format Support         |
| lua         | lua         | Lua Scripting Engine       |
| stb         | stb         | Single File Libraries      |
| libtsm      | libtsm      | Terminal Emulator Support  |
| regexp      | regexp      | Regular Expression Support |
| zlib        | zlib        | Deflate compression        |
| libufdt     | aosp        | For dtb overlay merge      |
| libfdt      | dtc         | For dtb process            |

### Code used

| Project            | Description                                     | URL                                                          |
|--------------------|-------------------------------------------------|--------------------------------------------------------------|
| busybox            | some builtin commands                           | https://www.busybox.net                                      |
| coreutils          | some builtin commands                           | https://www.gnu.org/software/coreutils/                      |
| util-linux         | some builtin commands and block lookup / mount  | https://github.com/karelzak/util-linux                       |
| lv_drivers         | fbdev, drm, gtk driver for LVGL GUI             | https://github.com/lvgl/lv_drivers                           |
| lv_sim_eclipse_sdl | sdl2 driver for LVGL GUI                        | https://github.com/lvgl/lv_sim_eclipse_sdl                   |
| lv_demos           | gui benchmark                                   | https://github.com/lvgl/lv_demos                             |
| lv_lib_freetype    | freetype2 true type fonts load                  | https://github.com/lvgl/lv_lib_freetype                      |
| lv_lib_png         | png image load                                  | https://github.com/lvgl/lv_lib_png                           |
| edk2-libc          | builtin libc implement                          | https://github.com/tianocore/edk2-libc                       |
| musl               | builtin libc implement                          | https://musl.libc.org/                                       |
| kmod               | some builtin commands and modules load / unload | https://git.kernel.org/pub/scm/utils/kernel/kmod/kmod.git    |
| papirus            | GUI icons                                       | https://github.com/PapirusDevelopmentTeam/papirus-icon-theme |

### Documents

Build guide and examples: [Docs](docs/index.md)
