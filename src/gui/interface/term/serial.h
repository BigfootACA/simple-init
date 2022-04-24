/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _SERIAL_H
#define _SERIAL_H

#include"gui/activity.h"
#ifdef ENABLE_UEFI
#include<Protocol/SerialIo.h>
#include<Library/UefiBootServicesTableLib.h>
#endif

struct serial_port_cfg{
	#ifdef ENABLE_UEFI
	EFI_SERIAL_IO_PROTOCOL*proto;
	#else
	char port[256+sizeof(void*)];
	#endif
	unsigned int baudrate;
};

struct baudrate{
	unsigned int speed;
	unsigned int number;
	char name[24];
};

extern struct baudrate serial_baudrates[];
extern struct gui_register guireg_serial_port;
#endif
