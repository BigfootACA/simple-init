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
	BOOT_HALT
};

// boot config
struct boot_config{
	boot_mode mode;
	char ident[32],desc[256];
	keyval*data[64];
	boot_main*main;
};

// src/boot/boot.c: execute boot config
extern int boot(boot_config*boot);

// src/boot/boot.c: convert boot_mode to string
extern char*bootmode2string(enum boot_mode mode);

// src/boot/boot.c: dump boot config to logger
extern void dump_boot_config(char*tag,enum log_level level,boot_config*boot);
#endif
