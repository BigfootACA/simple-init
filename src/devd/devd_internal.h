/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef DEVD_INTERNAL_H
#define DEVD_INTERNAL_H
#include<stdbool.h>
#include"devd.h"

// devd packet magic
#define DEVD_MAGIC0 0xEF
#define DEVD_MAGIC1 0x77

// devd operation
enum devd_oper{
	DEV_OK       =0xAD00,
	DEV_FAIL     =0xAD01,
	DEV_QUIT     =0xAD02,
	DEV_ADD      =0xAD03,
	DEV_INIT     =0xAD04,
	DEV_MODALIAS =0xAD05,
	DEV_MODLOAD  =0xAD06,
};

// devd message packet
struct devd_msg{
	unsigned char magic0,magic1;
	enum devd_oper oper;
	size_t size;
};

// src/devd/internal.c: init a devd packaet
extern void devd_internal_init_msg(struct devd_msg*msg,enum devd_oper oper,size_t size);

// src/devd/internal.c: check devd packaet magic
extern bool devd_internal_check_magic(struct devd_msg*msg);

// src/devd/internal.c: read extra data
extern char*devd_read_data(int fd,struct devd_msg*msg);

// src/devd/internal.c: send a devd packaet
extern int devd_internal_send_msg(int fd,enum devd_oper oper,void*data,size_t size);

// src/devd/internal.c: read a devd packaet
extern int devd_internal_read_msg(int fd,struct devd_msg*buff);

// src/devd/internal.c: send a string devd packet
extern int devd_internal_send_msg_string(int fd,enum devd_oper oper,char*data);

// src/devd/netlink.c: kobject uevent netlink listen thread
extern int uevent_netlink_thread(void);

// src/devd/internal.c: send devd command
extern int devd_command_with_data(enum devd_oper oper,void*data,size_t size);

// src/devd/internal.c: send devd command without data
extern int devd_command(enum devd_oper oper);

#endif