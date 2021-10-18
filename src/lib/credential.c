/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<pwd.h>
#include<grp.h>
#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include"pathnames.h"
#include"system.h"

char*get_username(uid_t uid,char*buff,size_t size){
	memset(buff,0,size);
	struct passwd*pw=getpwuid(uid);
	if(pw)strncpy(buff,pw->pw_name,size-1);
	else if(uid==0)strncpy(buff,"root",size-1);
	else snprintf(buff,size-1,"%d",uid);
	return buff;
}

char*get_groupname(gid_t gid,char*buff,size_t size){
	memset(buff,0,size);
	struct group*gr=getgrgid(gid);
	if(gr)strncpy(buff,gr->gr_name,size-1);
	else if(gid==0)strncpy(buff,"root",size-1);
	else snprintf(buff,size-1,"%d",gid);
	return buff;
}

char*get_commname(pid_t pid,char*buff,size_t size,bool with_pid){
	if(pid<=0)return NULL;
	if(read_file(
		buff,size,false,
		_PATH_PROC"/%d/comm",pid
	)<=0)snprintf(buff,size-1,"%d",pid);
	else if(with_pid){
		char p[16]={0};
		snprintf(p,15,"[%d]",pid);
		strncat(buff,p,size-1);
	}
	return buff;
}

char*ucred2string(struct ucred*c,char*buff,size_t size,bool with_pid){
	if(!c)return NULL;
	char user[256],group[256],process[256];
	memset(buff,0,size);
	snprintf(
		buff,size-1,
		"%s (%s:%s)",
		get_commname(c->pid,process,256,with_pid),
		get_username(c->uid,user,256),
		get_groupname(c->gid,group,256)
	);
	return buff;
}
