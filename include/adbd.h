/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _ADBD_H
#define _ADBD_H
#include"list.h"
#define LOCAL_CLIENT_PREFIX "linux-systemd-"
enum adb_proto{
	PROTO_NONE=0,
	PROTO_USB,
	PROTO_TCP
};
struct adb_data{
	char shell[512];
	char banner[64];
	char ffs[1024];
	bool auth_enabled;
	int port;
	int notifyfd;
	int local_port;
	enum adb_proto proto;
	list*prop;
};
extern int init_adb_data(struct adb_data*d);
extern int free_adb_data(struct adb_data*d);
extern int adbd_init(struct adb_data*data);
#ifdef _GADGET_H
extern int gadget_add_func_adbd(gadget*gadget,char*name,char*path);
#endif
#endif
