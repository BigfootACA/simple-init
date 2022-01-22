/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<string.h>
#include<stdlib.h>
#include<stddef.h>
#include<stdbool.h>
#include<sys/ioctl.h>
#include"list.h"
#include"confd.h"
#include"logger.h"
#include"system.h"
#include"pathnames.h"
#include"ttyd_internal.h"
#define TAG "ttyd"

list*tty_store=NULL;
int tty_dev_fd=-1,tty_epoll_fd=-1;
const char*tty_sock=DEFAULT_TTYD;
const char*tty_conf_ttys="ttyd.tty";
const char*tty_rt_ttys="runtime.ttyd.tty";
const char*tty_rt_tty_clt="runtime.ttyd.client";

void tty_conf_init(){
	if(confd_get_type(tty_conf_ttys)==TYPE_KEY)return;
	confd_set_boolean_base(tty_conf_ttys,"tty0.enabled",false);
	confd_set_boolean_base(tty_conf_ttys,"tty1.enabled",true);
	confd_set_boolean_base(tty_conf_ttys,"tty1.start_msg",true);
	confd_set_boolean_base(tty_conf_ttys,"tty2.enabled",true);
	confd_set_boolean_base(tty_conf_ttys,"tty2.start_msg",true);
	confd_set_boolean_base(tty_conf_ttys,"tty3.enabled",true);
	confd_set_boolean_base(tty_conf_ttys,"tty3.start_msg",true);
	confd_set_boolean_base(tty_conf_ttys,"tty4.enabled",true);
	confd_set_boolean_base(tty_conf_ttys,"tty4.start_msg",true);
	confd_set_boolean_base(tty_conf_ttys,"tty5.enabled",true);
	confd_set_boolean_base(tty_conf_ttys,"tty5.start_msg",true);
	confd_set_boolean_base(tty_conf_ttys,"tty6.enabled",true);
	confd_set_boolean_base(tty_conf_ttys,"tty6.start_msg",true);
	confd_set_string("ttyd.issue","Linux \\r with \\S \\V (\\l)\n\n");
	confd_set_string("ttyd.issue_file",_PATH_ETC"/issue");
	confd_set_string("ttyd.start_msg","[press any key to activate this console]\n");
}

bool tty_exists(const char*name){
	if(!tty_store||!name||strlen(name)<4||strncmp(name,"tty",3)!=0)return false;
	list*o=list_first(tty_store);
	if(o)do{
		LIST_DATA_DECLARE(data,o,struct tty_data*);
		if(data&&strcmp(name,data->name)==0)return true;
	}while((o=o->next));
	return false;
}

void tty_open(struct tty_data*data){
	if(!data||data->fd>=0)return;
	data->worker=(pid_t)confd_get_integer_base(tty_rt_tty_clt,data->name,0);
	if(data->worker>0){
		if(is_link(_PATH_PROC"/%d/root",data->worker))return;
		data->worker=0;
		confd_delete_base(tty_rt_tty_clt,data->name);
	}
	if((data->fd=openat(tty_dev_fd,data->name,O_RDWR|O_NONBLOCK))<0){
		if(errno==ENOENT)tlog_warn("tty %s not found",data->name);
		else telog_warn("open tty %s failed",data->name);
	}else{
		data->type=FD_TTY;
		data->ev.events=EPOLLIN;
		data->ev.data.ptr=data;
		fchown(data->fd,0,0);
		fchmod(data->fd,0600);
		tcflush(data->fd,TCIOFLUSH);
		struct termios tio;
		pid_t pid=setsid();
		if(pid<0)pid=getpid();
		pid_t s=tcgetsid(data->fd);
		if((s<0||s!=pid)){
			ioctl(data->fd,TIOCSCTTY,(long)1);
			tcsetpgrp(data->fd,pid);
		}
		if(tcgetattr(data->fd,&tio)>=0){
			alarm(5);
			tcdrain(data->fd);
			alarm(0);
			tcflush(data->fd,TCIOFLUSH);
			tio.c_cflag=0,tio.c_cflag|=CS8|HUPCL|CREAD;
			tio.c_iflag=0,tio.c_lflag=0,tio.c_oflag=OPOST|ONLCR;
			tio.c_cc[VMIN]=1,tio.c_cc[VTIME]=0,tio.c_line=0;
			tcsetattr(data->fd,TCSANOW,&tio);
		}
		epoll_ctl(tty_epoll_fd,EPOLL_CTL_ADD,data->fd,&data->ev);
		tlog_info("add watch tty %s",data->name);
		if(tty_confd_get_boolean(data,"start_msg",true)){
			char*start_msg=confd_get_string("ttyd.start_msg",NULL);
			if(start_msg){
				write(data->fd,start_msg,strlen(start_msg));
				free(start_msg);
			}
		}
	}
}

void tty_add(const char*base,const char*name){
	if(!base||!name)return;
	size_t s=strlen(name);
	if(s<4||s>63||strncmp(name,"tty",3)!=0){
		tlog_warn("invalid tty %s",name);
		return;
	}
	if(tty_exists(name))return;
	if(!confd_get_boolean_dict(base,name,"enabled",false)){
		tlog_debug("skip disabled %s",name);
		return;
	}
	struct tty_data*data=malloc(sizeof(struct tty_data));
	if(!data)return;
	memset(data,0,sizeof(struct tty_data));
	strncpy(data->name,name,63);
	data->fd=-1;
	tty_open(data);
	list_obj_add_new(&tty_store,data);
}

void tty_conf_add_all(){
	char**ttys;
	if((ttys=get_active_consoles())){
		for(size_t s=0;ttys[s];s++){
			if(confd_get_type_base(tty_rt_ttys,ttys[s])==TYPE_KEY)continue;
			if(confd_get_type_base(tty_conf_ttys,ttys[s])==TYPE_KEY)continue;
			tlog_notice("add console tty %s",ttys[s]);
			confd_set_boolean_dict(tty_rt_ttys,ttys[s],"enabled",true);
			confd_set_boolean_dict(tty_rt_ttys,ttys[s],"start_msg",true);
		}
		free(ttys);
	}
	if((ttys=confd_ls(tty_conf_ttys))){
		for(size_t s=0;ttys[s];s++)tty_add(tty_conf_ttys,ttys[s]);
		if(ttys[0])free(ttys[0]);
		free(ttys);
	}
	if((ttys=confd_ls(tty_rt_ttys))){
		for(size_t s=0;ttys[s];s++)tty_add(tty_rt_ttys,ttys[s]);
		if(ttys[0])free(ttys[0]);
		free(ttys);
	}
}

void tty_reopen_all(){
	if(!tty_store)return;
	list*o=list_first(tty_store);
	if(o)do{tty_open(LIST_DATA(o,struct tty_data*));}while((o=o->next));
}

bool tty_confd_get_boolean(struct tty_data*data,const char*key,bool def){
	if(confd_get_type_dict(tty_rt_ttys,data->name,key)==TYPE_BOOLEAN)
		return confd_get_boolean_dict(tty_rt_ttys,data->name,key,def);
	if(confd_get_type_dict(tty_conf_ttys,data->name,key)==TYPE_BOOLEAN)
		return confd_get_boolean_dict(tty_conf_ttys,data->name,key,def);
	return def;
}

char*tty_confd_get_string(struct tty_data*data,const char*key,char*def){
	if(confd_get_type_dict(tty_rt_ttys,data->name,key)==TYPE_STRING)
		return confd_get_string_dict(tty_rt_ttys,data->name,key,def);
	if(confd_get_type_dict(tty_conf_ttys,data->name,key)==TYPE_STRING)
		return confd_get_string_dict(tty_conf_ttys,data->name,key,def);
	return def;
}
