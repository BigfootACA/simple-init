#ifndef DEVFS_H
#define DEVFS_H
#include"uevent.h"

// src/devd/devtmpfs.c: create all device node when no CONFIG_DEVTMPFS
extern int init_devtmpfs(char*path);

// src/devd/devd.c: handle kernel uevent helper
extern int initdevd_main(int argc,char**argv);

// src/devd/dyndev.c: handle device node create or remove when no CONFIG_DEVTMPFS
extern int process_new_node(int devdfd,uevent*event);

// src/devd/firmware.c: handle firmware_class helper, search cmdline.h:firmware_list to load
extern int process_firmware_load(uevent*event);

#ifdef ENABLE_KMOD
// src/devd/modalias.c: search modalias in /sys/devices to load all modules
extern int load_modalias();
#else
static inline int load_modalias(){return 0};
#endif

#endif
