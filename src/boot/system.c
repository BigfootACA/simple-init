/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<string.h>
#include"boot.h"
#include"logger.h"
#include"defines.h"
#include"init_internal.h"
#define TAG "switchroot"

int run_boot_system(boot_config*boot __attribute__((unused))){
	errno=0;
	struct init_msg msg,response;
	init_initialize_msg(&msg,ACTION_SVC_START);
	strcpy(msg.data.data,"system");
	init_send(&msg,&response);
	if(errno!=0)telog_error("send service command failed");
	errno=response.data.status.ret;
	if(errno!=0)telog_error("start system failed");
	return response.data.status.ret;
}
