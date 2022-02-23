/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"boot.h"
#include"logger.h"
#include"linux_boot.h"
#define TAG "linux"

int run_boot_linux(boot_config*boot){
	linux_boot*lb=NULL;
	linux_config*cfg=NULL;
	if(boot->mode!=BOOT_LINUX)return -1;
	if(!(cfg=linux_config_new_from_confd(boot->key)))
		EDONE(tlog_error("generate config failed"));
	strncpy(cfg->tag,boot->ident,sizeof(cfg->tag)-1);
	if(!(lb=linux_boot_new(cfg)))
		EDONE(tlog_error("create linux boot failed"));
	if(linux_boot_prepare(lb)!=0)
		EDONE(tlog_error("prepare linux boot failed"));
	if(linux_boot_execute(lb)!=0)
		EDONE(tlog_error("execute linux boot failed"));

	tlog_warn("linux boot should never return");
	done:
	if(cfg)linux_config_free(cfg);
	if(lb)linux_boot_free(lb);
	return -1;
}
