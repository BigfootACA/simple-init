/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include<string.h>
#include<Library/UefiRuntimeServicesTableLib.h>
#include"boot.h"
#include"confd.h"
#include"logger.h"
#include"defines.h"
#define TAG "reboot"

int run_boot_reboot(boot_config*boot){
	if(!boot)ERET(EINVAL);
	char*data=NULL;
	UINTN s=0;
	VOID*x=NULL;
	EFI_RESET_TYPE t=EfiResetCold;
	switch(boot->mode){
		case BOOT_REBOOT:
			data=confd_get_string_base(boot->key,"arg",NULL);
			if(data){
				t=EfiResetPlatformSpecific;
				x=data;
				s=strlen(data);
			}
		break;
		case BOOT_HALT:
		case BOOT_POWEROFF:t=EfiResetShutdown;break;
		default:ERET(EINVAL);
	}
	confd_save_file(NULL,NULL);
	gRT->ResetSystem(t,EFI_SUCCESS,s,x);
	tlog_warn("reset system failed");
	return -1;
}
