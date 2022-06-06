/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef DEVFS_H
#define DEVFS_H
#include"uevent.h"

#define DEFAULT_DEVD _PATH_RUN"/initdevd.sock"

extern int devfd;

// src/devd/devtmpfs.c: create all device node when no CONFIG_DEVTMPFS
extern int init_devtmpfs(char*path);

// src/devd/devd.c: handle kernel uevent helper
extern int initdevd_main(int argc,char**argv);

// src/devd/dyndev.c: handle device node create or remove when no CONFIG_DEVTMPFS
extern int process_new_node(int devdfd,uevent*event);

// src/devd/firmware.c: handle firmware_class helper, search cmdline.h:firmware_list to load
extern int process_firmware_load(uevent*event);

// src/devd/devd.c: process uevent data
extern int process_uevent(uevent*event);

// src/devd/devd.c: connect and set devd fd
extern int open_devd_socket(char*tag,char*path);

// src/devd/devd.c: close current devd fd
extern void close_devd_socket(void);

// src/devd/server.c: launch devd
extern int start_devd(char*tag,pid_t*p);

// src/devd/devd.c: call DEV_INIT to init devtmpfs
extern int devd_call_init(void);

// src/devd/devd.c: call DEV_MODALIAS to load modalias
extern int devd_call_modalias(void);

// src/devd/devd.c: call DEV_MODLOAD to load modules from list config
extern int devd_call_modload(void);

// src/devd/devd.c: call DEV_QUIT to terminate devd
extern int devd_call_quit(void);

// open default devd socket
#define open_default_devd_socket(tag) open_devd_socket(tag,DEFAULT_DEVD)

// src/devd/modules_load.c: load modules from list config
extern int mods_conf_parse_file(const char*name,const char*file);

// src/devd/modules_load.c: scan folder and call mods_conf_parse_file
extern int mods_conf_parse_folder(const char*dir);

// src/devd/modules_load.c: search modules-load.d and call mods_conf_parse_folder
extern int mods_conf_parse(void);

// src/devd/modalias.c: search modalias in /sys/devices to load all modules
extern int load_modalias(void);

#endif
