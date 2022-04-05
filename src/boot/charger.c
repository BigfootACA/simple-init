/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include"boot.h"
#include"hardware.h"
#define TAG "charger"

extern int charger_main();
int run_boot_charger(boot_config*boot __attribute__((unused))){
	#ifdef ENABLE_GUI
	int bats[64]={0};
	if(pwr_scan_device(bats,63,true)<=0){
		tlog_info("wait for battery device");
		bool found=false;
		for(int i=0;i<20;i++){
			sleep(3);
			if(pwr_scan_device(bats,63,true)<=0)continue;
			found=true;
			break;
		}
		if(!found)return trlog_warn(-1,"no battery found, continuing boot");
	}
	pwr_close_device(bats);
	switch(fork()){
		case -1:return terlog_warn(-1,"fork failed");
		case 0:break;
		default:return 0;
	}
	_exit(charger_main());
	#endif
	return -1;
}
