/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<stddef.h>
#include<string.h>
#include"str.h"
#include"uevent.h"
#include"array.h"

const char *uevent_actions[]={
	[ACTION_ADD]     = "add",
	[ACTION_REMOVE]  = "remove",
	[ACTION_CHANGE]  = "change",
	[ACTION_MOVE]    = "move",
	[ACTION_ONLINE]  = "online",
	[ACTION_OFFLINE] = "offline",
	[ACTION_BIND]    = "bind",
	[ACTION_UNBIND]  = "unbind",
	[ACTION_UNKNOWN] = "unknown"
};

enum uevent_action uevent_chars2action(char*act){
	if(act)for(size_t s=0;s<ARRLEN(uevent_actions);s++)
		if(strcmp(uevent_actions[s],act)==0)return s;
	return ACTION_UNKNOWN;
}

const char*uevent_action2char(enum uevent_action act){
	return uevent_actions[act<0||act>=ARRLEN(uevent_actions)?ACTION_UNKNOWN:act];
}

uevent*uevent_fill_summary(uevent*data){
	if(!data||!data->environs)return NULL;
	keyval**kvs=data->environs;
	data->action=uevent_chars2action(kvarr_get(kvs,"ACTION",NULL));
	data->devpath=kvarr_get(kvs,"DEVPATH",NULL);
	data->subsystem=kvarr_get(kvs,"SUBSYSTEM",NULL);
	data->major=parse_int(kvarr_get(kvs,"MAJOR",NULL),-1);
	data->minor=parse_int(kvarr_get(kvs,"MINOR",NULL),-1);
	data->devname=kvarr_get(kvs,"DEVNAME",NULL);
	data->devtype=kvarr_get(kvs,"DEVTYPE",NULL);
	data->driver=kvarr_get(kvs,"DRIVER",NULL);
	data->modalias=kvarr_get(kvs,"MODALIAS",NULL);
	data->seqnum=parse_long(kvarr_get(kvs,"SEQNUM",NULL),-1);
	return data;
}

uevent*uevent_parse_x(char**envs,uevent*data){
	if(!envs||!data)return NULL;
	if(!(data->environs=kvarr_new_parse_arr(envs,'=')))return NULL;
	return uevent_fill_summary(data);
}

uevent*uevent_parse(char*envs,uevent*data){
	if(!envs||!data)return NULL;
	if(!(data->environs=kvarr_new_parse(envs,'\n','=')))return NULL;
	return uevent_fill_summary(data);
}
