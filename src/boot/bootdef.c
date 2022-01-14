/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"boot.h"
#include"confd.h"
#include"keyval.h"

extern int run_boot_root(boot_config*boot);
extern int run_boot_reboot(boot_config*boot);
extern int run_boot_system(boot_config*boot);
extern int run_boot_charger(boot_config*boot);

boot_main*boot_main_func[]={
	[BOOT_NONE]       = NULL,
	[BOOT_SWITCHROOT] = run_boot_root,
	[BOOT_CHARGER]    = run_boot_charger,
	[BOOT_KEXEC]      = run_boot_reboot,
	[BOOT_REBOOT]     = run_boot_reboot,
	[BOOT_POWEROFF]   = run_boot_reboot,
	[BOOT_HALT]       = run_boot_reboot,
	[BOOT_SYSTEM]     = run_boot_system,
};

#define EXTRA_DATA(val)((keyval*[]){&KV("data",(val)),NULL})
void boot_init_configs(void){
	boot_create_config(&(struct boot_config){
		.ident="switchroot",.icon="linux.svg",
		.mode=BOOT_SWITCHROOT,.desc="Default SwitchRoot",
		.save=false,.replace=false,
		.show=true,.enabled=false
	},NULL);
	boot_create_config(&(struct boot_config){
		.ident="system",.icon="launcher.svg",
		.mode=BOOT_SYSTEM,.desc="Enter Simple Init",
		.save=false,.replace=false,
		.show=true,.enabled=true
	},NULL);
	boot_create_config(&(struct boot_config){
		.ident="charger",.icon="battery.svg",
		.mode=BOOT_CHARGER,.desc="Charger Screen",
		.save=false,.replace=false,
		.show=false,.enabled=false
	},NULL);
	boot_create_config(&(struct boot_config){
		.ident="poweroff",.icon="poweroff.svg",
		.mode=BOOT_POWEROFF,.desc="Power off system",
		.save=false,.replace=false,
		.show=true,.enabled=true
	},NULL);
	boot_create_config(&(struct boot_config){
		.ident="reboot",.icon="reboot.svg",
		.mode=BOOT_REBOOT,.desc="Reboot system",
		.save=false,.replace=false,
		.show=true,.enabled=true
	},NULL);
	boot_create_config(&(struct boot_config){
		.ident="edl",.icon="download.svg",
		.mode=BOOT_REBOOT,.desc="Reboot into EDL",
		.save=false,.replace=false,
		.show=true,.enabled=true
	},EXTRA_DATA("edl"));
	boot_create_config(&(struct boot_config){
		.ident="recovery",.icon="twrp.png",
		.mode=BOOT_REBOOT,.desc="Reboot into recovery",
		.save=false,.replace=false,
		.show=true,.enabled=true
	},EXTRA_DATA("recovery"));
	boot_create_config(&(struct boot_config){
		.ident="bootloader",.icon="fastboot.svg",
		.mode=BOOT_REBOOT,.desc="Reboot into bootloader",
		.save=false,.replace=false,
		.show=true,.enabled=true
	},EXTRA_DATA("bootloader"));
	if(confd_get_type("boot.default")!=TYPE_STRING)confd_set_string("boot.default","system");
	if(confd_get_type("boot.second")!=TYPE_STRING)confd_set_string("boot.second","system");
}
