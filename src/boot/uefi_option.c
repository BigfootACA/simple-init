/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Library/UefiBootManagerLib.h>
#include"logger.h"
#include"boot.h"
#include"confd.h"
#define TAG "option"

int run_boot_uefi_option(boot_config*boot){
	UINTN cnt=0,i=0,opt;
	EFI_BOOT_MANAGER_LOAD_OPTION*bo;
	if(confd_get_type_base(boot->key,"option")!=TYPE_INTEGER)ERET(EINVAL);
	if(!(bo=EfiBootManagerGetLoadOptions(&cnt,LoadOptionTypeBoot))||cnt<=0)
		return ENUM(trlog_warn(ENOENT,"no any option found"));
	opt=(UINTN)confd_get_integer_base(boot->key,"option",0x10000);
	if(opt>0xFFFF)ERET(EINVAL);
	for(i=0;i<cnt;i++){
		if(bo[i].OptionNumber!=opt)continue;
		EfiBootManagerBoot(&bo[i]);
		return 0;
	}
	return ENUM(trlog_warn(ENOENT,"no match option (%04llu) found",(unsigned long long)opt));
}
