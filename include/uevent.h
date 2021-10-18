/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef UEVENT_H
#define UEVENT_H
#include"keyval.h"

// uevent action (from kernel)
enum uevent_action{
	ACTION_UNKNOWN,
	ACTION_ADD,
	ACTION_REMOVE,
	ACTION_CHANGE,
	ACTION_MOVE,
	ACTION_ONLINE,
	ACTION_OFFLINE,
	ACTION_BIND,
	ACTION_UNBIND
};
typedef enum uevent_action uevent_action;

// uevent data
struct uevent{
	uevent_action action;
	char*devpath;
	char*subsystem;
	int major,minor;
	char*devname;
	char*devtype;
	char*driver;
	char*modalias;
	long seqnum;
	keyval**environs;
};
typedef struct uevent uevent;

// src/devd/uevent.c: strings of uevent_action
extern const char *uevent_actions[];

// src/devd/uevent.c: convert environ to struct uevent
extern uevent*uevent_parse_x(char**envs,uevent*data);

// src/devd/uevent.c: convert environ string to struct uevent
extern uevent*uevent_parse(char*envs,uevent*data);

// src/devd/uevent.c: convert string to uevent_action
extern enum uevent_action uevent_chars2action(char* act);

// src/devd/uevent.c: convert uevent_action to string
extern const char*uevent_action2char(enum uevent_action act);

// src/devd/uevent.c: auto fill summary from environs
extern uevent*uevent_fill_summary(uevent*data);

#endif
