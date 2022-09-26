/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include<string.h>
#include"boot.h"
#include"confd.h"
#include"logger.h"
#include"defines.h"
#include"system.h"
#define TAG "reboot"

int run_boot_reboot(boot_config*boot){
	if(!boot)ERET(EINVAL);
	char*data=NULL;
	enum reboot_cmd cmd;
	switch(boot->mode){
		case BOOT_REBOOT:
			data=confd_get_string_base(boot->key,"arg",NULL);
			cmd=data?REBOOT_DATA:REBOOT_COLD;
		break;
		case BOOT_HALT:
		case BOOT_POWEROFF:cmd=REBOOT_POWEROFF;break;
		default:ERET(EINVAL);
	}
	confd_save_file(NULL);
	adv_reboot(cmd,data);
	tlog_warn("reset system failed");
	return -1;
}
