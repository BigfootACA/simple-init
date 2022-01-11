/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _BOOT_H
#define _BOOT_H
#include"keyval.h"
#include"logger.h"
struct boot_config;
enum boot_mode;
typedef struct boot_config boot_config;
typedef enum boot_mode boot_mode;
typedef int boot_main(boot_config*);

// boot mode
enum boot_mode{
	BOOT_NONE=0,
	BOOT_SWITCHROOT,
	BOOT_CHARGER,
	BOOT_KEXEC,
	BOOT_REBOOT,
	BOOT_POWEROFF,
	BOOT_HALT,
	BOOT_SYSTEM,
};

// boot config
struct boot_config{
	boot_mode mode;
	char ident[32];
	char icon[32];
	char desc[256];
	char base[256];
	char key[256];
	bool save;
	bool replace;
	bool show;
	bool enabled;
};

extern boot_main*boot_main_func[];

// src/boot/boot.c: create boot config
extern char*boot_create_config(struct boot_config*cfg,keyval**data);

// src/boot/bootdef.c: create initial boot configs
extern void boot_init_configs(void);

// src/boot/bootdef.c: get boot config by name
extern boot_config*boot_get_config(const char*name);

// src/boot/boot.c: execute boot config
extern int boot(boot_config*boot);

// src/boot/boot.c: execute boot config by name
extern int boot_name(const char*name);

// src/boot/boot.c: convert boot_mode to string
extern char*bootmode2string(enum boot_mode mode);

// src/boot/boot.c: dump boot config to logger
extern void dump_boot_config(char*tag,enum log_level level,boot_config*boot);

// src/boot/boot.c: register default boot
extern int register_default_boot(void);

#endif
