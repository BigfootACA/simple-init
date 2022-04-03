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
#ifdef ENABLE_UEFI
#include<Library/BaseLib.h>
#include<Library/PrintLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/UefiBootManagerLib.h>
#endif

#undef reboot
#undef root
#undef system
#undef charger
#undef exit
#undef linux
#undef simple_init

#define DECLARE_BOOT_COMM(name) extern int run_boot_##name(boot_config*boot)
#define CFG_COMM(name) run_boot_##name
#ifdef ENABLE_UEFI
#define BOOT_DEFAULT "continue"
#define BOOT_SECOND "simple-init"
#define DECLARE_BOOT_LINUX(name)
#define DECLARE_BOOT_UEFI(name) DECLARE_BOOT_COMM(name)
#define CFG_LINUX(name) NULL
#define CFG_UEFI(name) CFG_COMM(name)
#else
#define BOOT_DEFAULT "system"
#define BOOT_SECOND "system"
#define DECLARE_BOOT_LINUX(name) DECLARE_BOOT_COMM(name)
#define DECLARE_BOOT_UEFI(name)
#define CFG_LINUX(name) CFG_COMM(name)
#define CFG_UEFI(name) NULL
#endif

DECLARE_BOOT_COMM(lua);
DECLARE_BOOT_COMM(reboot);
DECLARE_BOOT_LINUX(root);
DECLARE_BOOT_LINUX(system);
DECLARE_BOOT_LINUX(charger);
DECLARE_BOOT_UEFI(exit);
DECLARE_BOOT_UEFI(efi);
DECLARE_BOOT_UEFI(linux);
DECLARE_BOOT_UEFI(uefi_option);

boot_main*boot_main_func[]={
	[BOOT_NONE]           = NULL,
	[BOOT_SWITCHROOT]     = CFG_LINUX(root),
	[BOOT_CHARGER]        = CFG_LINUX(charger),
	[BOOT_KEXEC]          = CFG_LINUX(reboot),
	[BOOT_REBOOT]         = CFG_COMM(reboot),
	[BOOT_POWEROFF]       = CFG_COMM(reboot),
	[BOOT_HALT]           = CFG_LINUX(reboot),
	[BOOT_SYSTEM]         = CFG_LINUX(system),
	[BOOT_EXIT]           = CFG_UEFI(exit),
	[BOOT_LINUX]          = CFG_UEFI(linux),
	[BOOT_EFI]            = CFG_UEFI(efi),
	[BOOT_SIMPLE_INIT]    = NULL,
	[BOOT_UEFI_OPTION]    = CFG_UEFI(uefi_option),
	[BOOT_LUA]            = CFG_COMM(lua),
	[BOOT_FOLDER]         = NULL,
};

#define EXTRA_ARG(val)((keyval*[]){&KV("arg",(val)),NULL})

#ifdef ENABLE_UEFI
static void load_uefi_boot(){
	UINTN cnt=0,i=0;
	EFI_BOOT_MANAGER_LOAD_OPTION*bo;
	struct boot_config opt;
	struct boot_config menu={
		.mode=BOOT_FOLDER,
		.ident="uefi-bootmenu",
		.icon="uefi.svg",
		.desc="UEFI Boot Menu",
		.save=false,.replace=false,
		.show=true,.enabled=true
	};
	struct boot_config def={
		.mode=BOOT_UEFI_OPTION,
		.parent="uefi-bootmenu",
		.icon="uefi.svg",
		.save=false,.replace=false,
		.show=true,.enabled=true
	};
	if(!confd_get_boolean("boot.uefi_bootmenu",true))return;
	boot_create_config(&menu,NULL);
	EfiBootManagerRefreshAllBootOption();
	if(!(bo=EfiBootManagerGetLoadOptions(&cnt,LoadOptionTypeBoot)))return;
	if(cnt<=0)return;
	else for(i=0;i<cnt;i++){
		CopyMem(&opt,&def,sizeof(struct boot_config));
		UnicodeStrToAsciiStrS(bo[i].Description,opt.desc,sizeof(opt.desc));
		AsciiSPrint(
			opt.ident,
			sizeof(opt.ident)-1,
			"uefi-boot%04d",
			bo[i].OptionNumber
		);
		boot_create_config(&opt,NULL);
		confd_set_integer_base(
			opt.key,
			"option",
			bo[i].OptionNumber
		);
	}
}
#endif

void boot_init_configs(void){
	#ifdef ENABLE_UEFI
	boot_scan_efi();
	boot_create_config(&(struct boot_config){
		.mode=BOOT_EXIT,
		.ident="continue",.icon="exit.svg",
		.desc="Continue Boot",
		.save=false,.replace=false,
		.show=true,.enabled=true
	},NULL);
	boot_create_config(&(struct boot_config){
		.mode=BOOT_SIMPLE_INIT,
		.ident="simple-init",.icon="launcher.svg",
		.desc="Enter Simple Init",
		.save=false,.replace=false,
		.show=true,.enabled=true
	},NULL);
	load_uefi_boot();
	#else
	boot_create_config(&(struct boot_config){
		.mode=BOOT_SWITCHROOT,
		.ident="switchroot",.icon="linux.svg",
		.desc="Default SwitchRoot",
		.save=false,.replace=false,
		.show=true,.enabled=false
	},NULL);
	boot_create_config(&(struct boot_config){
		.mode=BOOT_SYSTEM,
		.ident="system",.icon="launcher.svg",
		.desc="Enter Simple Init",
		.save=false,.replace=false,
		.show=true,.enabled=true
	},NULL);
	boot_create_config(&(struct boot_config){
		.mode=BOOT_CHARGER,
		.ident="charger",.icon="battery.svg",
		.desc="Charger Screen",
		.save=false,.replace=false,
		.show=false,.enabled=false
	},NULL);
	#endif
	boot_create_config(&(struct boot_config){
		.mode=BOOT_FOLDER,
		.ident="reboots",.icon="reboot.svg",
		.desc="Reboot Menu",
		.save=false,.replace=false,
		.show=true,.enabled=true
	},NULL);
	boot_create_config(&(struct boot_config){
		.mode=BOOT_POWEROFF,
		.ident="poweroff",.icon="poweroff.svg",
		.desc="Power off system",
		.parent="reboots",
		.save=false,.replace=false,
		.show=true,.enabled=true
	},NULL);
	boot_create_config(&(struct boot_config){
		.mode=BOOT_REBOOT,
		.ident="reboot",.icon="reboot.svg",
		.desc="Reboot system",
		.parent="reboots",
		.save=false,.replace=false,
		.show=true,.enabled=true
	},NULL);
	boot_create_config(&(struct boot_config){
		.mode=BOOT_REBOOT,
		.ident="edl",.icon="download.svg",
		.desc="Reboot into EDL",
		.parent="reboots",
		.save=false,.replace=false,
		.show=true,.enabled=true
	},EXTRA_ARG("edl"));
	boot_create_config(&(struct boot_config){
		.mode=BOOT_REBOOT,
		.ident="recovery",.icon="twrp.png",
		.desc="Reboot into recovery",
		.parent="reboots",
		.save=false,.replace=false,
		.show=true,.enabled=true
	},EXTRA_ARG("recovery"));
	boot_create_config(&(struct boot_config){
		.mode=BOOT_REBOOT,
		.ident="bootloader",.icon="fastboot.svg",
		.desc="Reboot into bootloader",
		.parent="reboots",
		.save=false,.replace=false,
		.show=true,.enabled=true
	},EXTRA_ARG("bootloader"));

	if(confd_get_type("boot.default")!=TYPE_STRING)
		confd_set_string("boot.default",BOOT_DEFAULT);
	if(confd_get_type("boot.second")!=TYPE_STRING)
		confd_set_string("boot.second",BOOT_SECOND);
}
