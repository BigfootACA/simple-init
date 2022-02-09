## A simple init for linux (maybe it can also run under UEFI)

### Description

A linux system in a single binary (Stuffed with strange things)

Some references of this project may not meet the license, if you find any, please help me to correct it.

### WARNING

This project has not been completed and may not work properly.

### Depends

| Library     | Project     | Description                |
| ----------- | ----------- | -------------------------- |
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
| freetype2   | freetype    | GUI load turetype fonts    |
| lodepng     | lodepng     | GUI load png pictures      |
| nanosvg     | nanosvg     | GUI load svg pictures      |
| libjpeg     | libjpeg     | GUI load jpeg pictures     |
| vncserver   | libvnc      | GUI over VNC Server        |
| hivex       | libguestfs  | Windows Registry/BCD Edit  |

### Code used

| Project            | Description                                     | URL                                                          |
| ------------------ | ----------------------------------------------- | ------------------------------------------------------------ |
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

### Features

- [x] Core Functions
  - [x] Init Daemon
    - [x] Signal handler
    - [x] Switch root
    - [x] Control interface
    - [x] PowerOff, Reboot, Shutdown
    - [x] Command line parse
    - [ ] Kexec
  - [x] Service Manager
    - [x] Once, Daemon, Foreground Service
    - [x] Auto restart
    - [x] Control interface
    - [x] Start / Stop / Restart / Reload service
    - [x] Stop service
    - [ ] Service config file
    - [ ] Control Groups
    - [ ] Name Space
  - [x] Device Daemon
    - [x] devtmpfs auto create / remove devices
    - [x] Load all modules
    - [x] Hotplug load modules
    - [x] Netlink uevent kobject
    - [x] Kernel userspace hotplug notifier
    - [ ] Devd rules config file
    - [x] Load firmware
    - [ ] Device permissions
  - [x] Logger Daemon
    - [x] General logger functions
    - [x] Log save
    - [ ] Log raw storage
    - [x] Log buffer
    - [x] Syslog interface
    - [x] Kernel ring buffer interface
    - [x] Runtime open / close log files
    - [x] LogFS log files save
    - [x] Control interface
    - [ ] Client read log buffer
- [ ] Builtin Shell
  - [x] Execute builtin commands
  - [x] Execute external commands
  - [x] History
  - [x] Line Edit
  - [ ] Pipe
  - [ ] Job Control
  - [ ] Shell Script
  - [x] Shell Prompt
- [ ] Hardware Functions
  - [ ] Buttons event listener
  - [x] Turn on or off the LED
  - [x] Vibrator / Haptics
  - [x] USB Gadget
    - [x] USB Mass Storage
    - [x] USB Remote-NDIS Network
    - [ ] USB Emulate HID Device
    - [x] USB Android-Debug-Bridge Daemon
    - [ ] USB MTP
    - [ ] USB Camera
    - [ ] USB Serial Console
    - [ ] USB Over IP
  - [ ] USB Host
    - [ ] USB Guard Daemon
    - [ ] USB Android-Debug-Bridge
    - [ ] USB Fastboot
    - [ ] USB Over IP
  - [ ] Power Supply
    - [x] Battery
    - [ ] Power low
    - [ ] Charger
  - [ ] Sensors
  - [ ] GPS
- [ ] Network
  - [ ] Network Manage Daemon
    - [ ] Interface up down
    - [ ] IPv4
    - [ ] IPv6
    - [ ] Static address
    - [ ] DHCP
    - [ ] Bridge
    - [ ] Vlan
    - [ ] TUN / TAP
    - [ ] Firewall
      - [ ] ebtables
      - [ ] iptables
      - [ ] ipset
    - [ ] Wireless
      - [ ] WLAN
      - [ ] Modem
    - [ ] PPPoE
    - [ ] VPN
  - [ ] Telnet Server
  - [ ] Telnet Client
  - [ ] SSH Server
  - [ ] SFTP Server
  - [ ] FTP Server
  - [ ] NTP Server
  - [ ] TFTP Server
  - [ ] DHCP Server
  - [ ] HTTP/HTTPS Server
  - [ ] Syslog Server
- [ ] Qualcomm Userspace Tools
  - [ ] qrtr
  - [ ] pd-mapper
  - [ ] tqftpserv
  - [ ] rmtfs
  - [ ] pil-squasher
- [ ] Filesystem
  - [ ] EXT2/3/4
    - [ ] Format
    - [ ] Resize
    - [ ] Check
  - [ ] Btrfs
    - [ ] Format
    - [ ] Resize
    - [ ] Check
  - [ ] F2FS
    - [ ] Format
    - [ ] Resize
    - [ ] Check
  - [ ] FAT12/16/32
    - [ ] Format
    - [ ] Resize
    - [ ] Check
  - [ ] exFAT(FAT64)
    - [ ] Format
    - [ ] Resize
    - [ ] Check
  - [ ] NTFS
    - [ ] Format
    - [ ] Resize
    - [ ] Check
- [ ] GUI
  - [x] Driver
    - [x] DRM
    - [x] Framebuffer
    - [x] Touchscreen
    - [x] GTK
    - [x] SDL2
    - [x] Buttons
  - [ ] Application
    - [x] File Manager
    - [x] Partition Manager
      - [ ] Make disk label
      - [ ] Create partition
      - [ ] Delete partition
      - [ ] Rename partition
      - [ ] Resize partition
      - [ ] Filesystem
    - [ ] Multi-boot Manager
      - [ ] Kexec
      - [ ] Switch Root
      - [ ] Config Edit
    - [ ] Registry Editor
      - [ ] Open
      - [ ] List
      - [ ] Read
      - [ ] Change
      - [ ] Save
    - [ ] Image Backup Recovery
      - [ ] Windows Imaging Archives (WIM)
        - [ ] Extract
        - [ ] Capture device
        - [ ] Apply to device
        - [ ] Info Read
        - [ ] Info Edit
        - [ ] View
      - [ ] RAW Disk Image
        - [ ] Capture
        - [ ] Recovery
    - [ ] Enter TWRP
      - [ ] Manual select TWRP image
    - [ ] System Info
    - [x] Reboot Menu
    - [x] Loggerd Viewer
- [ ] Remote Control Interface
  - [ ] Over USB
  - [ ] Over Network
  - [ ] Shell
  - [ ] Mass Storage
  - [ ] FUSE
  - [ ] Write image
  - [ ] File Manage
  - [ ] GPIO
  - [ ] Partition Manage
  - [ ] Reboot
  - [ ] Service
  - [ ] Network
  - [ ] System info
