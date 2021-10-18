/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<fcntl.h>
#include<unistd.h>
#include"system.h"
#include"pathnames.h"
#include"hardware.h"

void vibrate(int time){
	if(time<0||time>0xFFFF)return;
	if(fd_write_int(
		AT_FDCWD,
		_PATH_SYS_CLASS"/timed_output/vibrator/enable",
		time,true
	)==0)return;
	int v=led_find("vibrator");
	if(v<0)return;
	fd_write_int(v,"duration",time,true);
	fd_write_int(v,"activate",1,true);
	close(v);
}
