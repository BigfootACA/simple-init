/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"boot.h"
#include"array.h"
#include"confd.h"
#include"keyval.h"
#include"gui/splash.h"
#ifdef ENABLE_UEFI
#include<Library/BaseLib.h>
#include<Library/PrintLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
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

static struct initial_cfg{
	bool valid;
	struct boot_config cfg;
	keyval**args;
}initial_cfgs[]={
	#ifdef ENABLE_UEFI
	{
		.valid=true,
		.cfg={
			.mode=BOOT_EXIT,
			.ident="continue",
			.desc="Continue Boot",
			.save=false,.replace=false,
			.show=true,.enabled=true
		},.args=NULL
	},{
		.valid=true,
		.cfg={
			.mode=BOOT_SIMPLE_INIT,
			.ident="simple-init",
			.desc="Enter Simple Init",
			.save=false,.replace=false,
			.show=true,.enabled=true
		},.args=NULL
	},{
		.valid=true,
		.cfg={
			.mode=BOOT_FOLDER,
			.ident="uefi-bootmenu",
			.desc="UEFI Boot Menu",
			.save=false,.replace=false,
			.show=false,.enabled=true
		},.args=NULL
	},
	#else
	{
		.valid=true,
		.cfg={
			.mode=BOOT_SWITCHROOT,
			.ident="switchroot",
			.desc="Default SwitchRoot",
			.save=false,.replace=false,
			.show=true,.enabled=false
		},.args=NULL
	},{
		.valid=true,
		.cfg={
			.mode=BOOT_SYSTEM,
			.ident="system",
			.desc="Enter Simple Init",
			.save=false,.replace=false,
			.show=true,.enabled=true
		},.args=NULL
	},{
		.valid=true,
		.cfg={
			.mode=BOOT_CHARGER,
			.ident="charger",
			.desc="Charger Screen",
			.save=false,.replace=false,
			.show=false,.enabled=true
		},.args=NULL
	},
	#endif
	{
		.valid=true,
		.cfg={
			.mode=BOOT_FOLDER,
			.ident="reboots",
			.desc="Reboot Menu",
			.save=false,.replace=false,
			.show=true,.enabled=true
		},.args=NULL
	},{
		.valid=true,
		.cfg={
			.mode=BOOT_POWEROFF,
			.ident="poweroff",
			.desc="Power off system",
			.parent="reboots",
			.save=false,.replace=false,
			.show=true,.enabled=true
		},.args=NULL
	},{
		.valid=true,
		.cfg={
			.mode=BOOT_REBOOT,
			.ident="reboot",
			.desc="Reboot system",
			.parent="reboots",
			.save=false,.replace=false,
			.show=true,.enabled=true
		},.args=NULL
	},{
		.valid=true,
		.cfg={
			.mode=BOOT_REBOOT,
			.ident="edl",
			.desc="Reboot into EDL",
			.parent="reboots",
			.save=false,.replace=false,
			.show=true,.enabled=true
		},.args=EXTRA_ARG("edl")
	},{
		.valid=true,
		.cfg={
			.mode=BOOT_REBOOT,
			.ident="recovery",
			.desc="Reboot into recovery",
			.parent="reboots",
			.save=false,.replace=false,
			.show=true,.enabled=true
		},.args=EXTRA_ARG("recovery")
	},{
		.valid=true,
		.cfg={
			.mode=BOOT_REBOOT,
			.ident="bootloader",
			.desc="Reboot into bootloader",
			.parent="reboots",
			.save=false,.replace=false,
			.show=true,.enabled=true
		},.args=EXTRA_ARG("bootloader")
	},
	{.valid=false}
};

#ifdef ENABLE_UEFI
static void load_uefi_boot(){
	UINTN cnt=0,i=0;
	EFI_BOOT_MANAGER_LOAD_OPTION*bo=NULL;
	struct boot_config*opt=NULL,*def=NULL;
	if(!(opt=AllocateZeroPool(sizeof(struct boot_config))))goto done;
	if(!(def=AllocateZeroPool(sizeof(struct boot_config))))goto done;
	def->mode=BOOT_UEFI_OPTION;
	strcpy(def->parent,"uefi-bootmenu");
	strcpy(def->icon,"@bootitem-uefi-bootitem");
	def->save=false;
	def->replace=false;
	def->show=true;
	def->enabled=true;
	if(!confd_get_boolean("boot.uefi_bootmenu",true))goto done;
	EfiBootManagerRefreshAllBootOption();
	if(!(bo=EfiBootManagerGetLoadOptions(&cnt,LoadOptionTypeBoot)))goto done;
	if(cnt<=0)goto done;
	else for(i=0;i<cnt;i++){
		CopyMem(opt,def,sizeof(struct boot_config));
		UnicodeStrToAsciiStrS(bo[i].Description,opt->desc,sizeof(opt->desc));
		AsciiSPrint(
			opt->ident,
			sizeof(opt->ident)-1,
			"uefi-boot%04d",
			bo[i].OptionNumber
		);
		boot_create_config(opt,NULL);
		confd_set_integer_base(
			opt->key,
			"option",
			bo[i].OptionNumber
		);
	}
	confd_set_boolean("boot.configs.uefi-bootmenu.show",true);
	done:
	if(opt)FreePool(opt);
	if(def)FreePool(def);
	if(bo)EfiBootManagerFreeLoadOptions(bo,cnt);
}
#endif

void boot_init_configs(void){
	#ifdef ENABLE_UEFI
	gui_splash_set_text(true,_("Probing OS on all disks..."));
	boot_scan_efi();
	#endif
	struct initial_cfg*c;
	gui_splash_set_text(true,_("Loading builtin boot configs..."));
	for(size_t i=0;(c=&initial_cfgs[i])->valid;i++)
		boot_create_config(&c->cfg,c->args);
	#ifdef ENABLE_UEFI
	gui_splash_set_text(true,_("Loading UEFI boot options..."));
	load_uefi_boot();
	#endif
	if(confd_get_type("boot.default")!=TYPE_STRING)
		confd_set_string("boot.default",BOOT_DEFAULT);
	if(confd_get_type("boot.second")!=TYPE_STRING)
		confd_set_string("boot.second",BOOT_SECOND);
}
