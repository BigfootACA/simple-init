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
#include"init_internal.h"
#define TAG "reboot"

int run_boot_reboot(boot_config*boot){
	if(!boot)ERET(EINVAL);
	char*data=NULL;
	struct init_msg msg,response;
	size_t ss=sizeof(msg.data.data);
	init_initialize_msg(&msg,ACTION_REBOOT);
	switch(boot->mode){
		case BOOT_REBOOT:
			data=confd_get_string_base(boot->key,"arg",NULL);
			msg.action=ACTION_REBOOT;
			if(data){
				if(strlen(data)>=ss)return terlog_error(2,"reboot argument too long");
				strncpy(msg.data.data,data,ss-1);
				tlog_alert("rebooting with argument '%s'",data);
				free(data);
			}else tlog_alert("rebooting");
		break;
		case BOOT_HALT:
			msg.action=ACTION_HALT;
			tlog_alert("halt system");
		break;
		case BOOT_POWEROFF:
			msg.action=ACTION_POWEROFF;
			tlog_alert("poweroff system");
		break;
		case BOOT_KEXEC:return trlog_error(ENUM(ENOSYS),"kexec does not implemented");
		default:ERET(EINVAL);
	}
	init_send(&msg,&response);
	if(errno!=0)telog_warn("send command failed");
	if(response.data.status.ret!=0)tlog_warn(
		"reboot failed: %s",
		strerror(response.data.status.ret)
	);
	return response.data.status.ret;
}