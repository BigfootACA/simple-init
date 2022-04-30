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
	BOOT_NONE        = 0x00,
	BOOT_SWITCHROOT  = 0x01,
	BOOT_CHARGER     = 0x02,
	BOOT_KEXEC       = 0x03,
	BOOT_REBOOT      = 0x04,
	BOOT_POWEROFF    = 0x05,
	BOOT_HALT        = 0x06,
	BOOT_SYSTEM      = 0x07,
	BOOT_LINUX       = 0x08,
	BOOT_EFI         = 0x09,
	BOOT_EXIT        = 0x0A,
	BOOT_SIMPLE_INIT = 0x0B,
	BOOT_UEFI_OPTION = 0x0C,
	BOOT_LUA         = 0x0D,
	BOOT_FOLDER      = 0xFF,
};

// cpu architecture type
enum cpu_type{
	CPU_NONE=0,
	CPU_ANY,
	CPU_IA32,
	CPU_IA64,
	CPU_X64,
	CPU_ARM,
	CPU_AARCH64,
	CPU_RISCV32,
	CPU_RISCV64,
};

// efi prober declare
struct efi_path{
	bool enable;
	enum cpu_type cpu;
	char*icon;
	char*title;
	char**dir;
	char**name;
	char*load_opt;
	bool unicode;
};

// boot config
struct boot_config{
	boot_mode mode;
	char ident[64];
	char parent[64];
	char icon[64];
	char desc[256];
	char base[256];
	char key[256];
	char splash[PATH_MAX];
	bool save;
	bool replace;
	bool show;
	bool enabled;
};

extern const enum cpu_type current_cpu;
extern struct efi_path boot_efi_paths[];
extern boot_main*boot_main_func[];

// src/boot/boot.c: create boot config
extern int boot_create_config(struct boot_config*cfg,keyval**data);

// src/boot/bootdef.c: create initial boot configs
extern void boot_init_configs(void);

// src/boot/bootdef.c: get boot config by name
extern boot_config*boot_get_config(const char*name);

// src/boot/boot.c: draw splash image in boot config
extern int boot_draw_splash(boot_config*cfg);

// src/boot/boot.c: execute boot config
extern int boot(boot_config*boot);

// src/boot/boot.c: execute boot config by name
extern int boot_name(const char*name);

// src/boot/boot.c: convert boot_mode to string
extern char*bootmode2string(enum boot_mode mode);

// src/boot/boot.c: convert boot_mode to short string
extern char*bootmode2shortstring(enum boot_mode mode);

// src/boot/boot.c: convert short string to boot_mode
extern bool shortstring2bootmode(const char*str,enum boot_mode*mode);

// src/boot/boot.c: dump boot config to logger
extern void dump_boot_config(char*tag,enum log_level level,boot_config*boot);

#ifdef ENABLE_UEFI

// src/boot/bootdef.c: probe efi os loaders
extern void boot_scan_efi();

// src/boot/drivers.c: load uefi dxe driver
extern bool boot_load_driver(EFI_DEVICE_PATH_PROTOCOL*p);

// src/boot/drivers.c: load uefi dxe drivers from confd
extern void boot_load_drivers();

// src/boot/boot.c: set efi var
extern EFI_STATUS boot_setvar(CHAR16*key,VOID*buf,UINTN size);

// src/boot/boot.c: set efi string var
extern EFI_STATUS boot_setvar_str(CHAR16*key,CHAR16*str,...);

// src/boot/boot.c: set efi integer var
extern EFI_STATUS boot_setvar_int(CHAR16 *name,UINTN num);
#else
// src/boot/boot.c: register default boot
extern int register_default_boot(void);
#endif

#ifdef ENABLE_LUA
#include"xlua.h"
extern int boot_config_to_lua(lua_State*st,boot_config*boot);
#endif

#endif
