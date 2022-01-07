/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<sys/socket.h>
#include"str.h"
#include"system.h"
#include"service.h"
#include"logger.h"
#include"defines.h"
#include"language.h"
#include"init_internal.h"
#define TAG "init"

bool init_check_msg(struct init_msg*msg){
	return msg&&
		msg->magic0==INITD_MAGIC0&&
		msg->magic1==INITD_MAGIC1;
}

void init_initialize_msg(struct init_msg*msg,enum init_action act){
	if(!msg)return;
	memset(msg,0,sizeof(struct init_msg));
	msg->magic0=INITD_MAGIC0;
	msg->magic1=INITD_MAGIC1;
	msg->action=act;
}

bool init_check_privilege(enum init_action act,struct ucred*cred){
	if(!cred||cred->pid<=0)return false;
	if(cred->uid!=0||cred->gid!=0){
		char s[BUFSIZ];
		tlog_debug(
			"unprivileged init control request %s from %s",
			action2string(act),
			ucred2string(cred,s,BUFSIZ,true)
		);
		return false;
	}
	return true;
}

static void process_switchroot(struct init_msg*msg,struct init_msg*res){
	size_t sr=sizeof(msg->data.newroot.root);
	size_t si=sizeof(msg->data.newroot.init);
	char*realinit=msg->data.newroot.init;
	if(msg->data.newroot.root[0]==0){
		res->data.status.ret=errno=EINVAL;
		return;
	}
	if(
		msg->data.newroot.root[sr-1]!=0||
		msg->data.newroot.init[si-1]!=0
	){
		tlog_warn("stack overflow detected on request");
		res->data.status.ret=errno=EFAULT;
		return;
	}
	if(is_folder(msg->data.newroot.root)){
		action=msg->action;
		status=INIT_SHUTDOWN;
	}else{
		if(errno==0)errno=ENOTDIR;
		res->data.status.ret=errno;
		return;
	}
	if(realinit[0]==0)realinit=NULL;
	if(!search_init(
		realinit,
		msg->data.newroot.root
	)){
		if(errno==0)errno=ENOEXEC;
		telog_error("check newroot init");
		return;
	}
	errno=0;
}

static void process_setenv(struct init_msg*msg,struct init_msg*res){
	size_t sk=sizeof(msg->data.env.key);
	size_t sv=sizeof(msg->data.env.value);
	if(msg->data.env.key[0]==0){
		res->data.status.ret=errno=EINVAL;
		return;
	}
	if(
		msg->data.env.key[sk-1]!=0||
		msg->data.env.value[sv-1]!=0
	){
		tlog_warn("stack overflow detected on request");
		res->data.status.ret=errno=EFAULT;
		return;
	}
	if(!check_identifier(msg->data.env.key)){
		res->action=ACTION_FAIL;
		res->data.status.ret=EINVAL;
		return;
	}
	if(setenv(
		msg->data.env.key,
		msg->data.env.value,
		1
	)==-1){
		res->action=ACTION_FAIL;
		res->data.status.ret=errno;
	}else tlog_info(
		"add new environment variable \"%s\" = \"%s\"",
		msg->data.env.key,msg->data.env.value
	);
}

static void process_language(struct init_msg*msg,struct init_msg*res){
	if(lang_set(msg->data.data)!=0)res->data.status.ret=errno;
	else tlog_info("set language to %s",msg->data.data);
}

int init_process_data(struct init_client*clt,struct init_msg*msg){
	char s[BUFSIZ];
	struct init_msg res;
	init_initialize_msg(&res,ACTION_OK);
	if(!init_check_msg(msg))ERET(EINVAL);
	memset(&actiondata,0,sizeof(actiondata));
	memcpy(&actiondata,&msg->data,sizeof(union action_data));
	tlog_debug(
		"receive %s request from %s",
		action2string(msg->action),
		ucred2string(&clt->cred,s,BUFSIZ,true)
	);
	if(init_check_privilege(msg->action,&clt->cred))switch(msg->action){
		case ACTION_POWEROFF:case ACTION_HALT:case ACTION_REBOOT:
			action=msg->action,status=INIT_SHUTDOWN;
		break;
		case ACTION_SWITCHROOT:process_switchroot(msg,&res);break;
		case ACTION_ADDENV:process_setenv(msg,&res);break;
		case ACTION_LANGUAGE:process_language(msg,&res);break;
		case ACTION_SVC_START:
			if(service_start_by_name(msg->data.data)<0){
				res.action=ACTION_FAIL;
				res.data.status.ret=errno;
			}
		break;
		case ACTION_SVC_STOP:{
			struct service*svc;
			if((svc=svc_lookup_by_name(msg->data.data))){
				svc->retry=-1;
				if(service_stop(svc)>=0)break;
			}
			res.action=ACTION_FAIL;
			res.data.status.ret=errno;
		}break;
		case ACTION_SVC_RESTART:
			if(service_restart_by_name(msg->data.data)<0){
				res.action=ACTION_FAIL;
				res.data.status.ret=errno;
			}
		break;
		case ACTION_SVC_RELOAD:
			if(service_reload_by_name(msg->data.data)<0){
				res.action=ACTION_FAIL;
				res.data.status.ret=errno;
			}
		break;
		case ACTION_SVC_STATUS:{
			struct service*svc;
			if((svc=svc_lookup_by_name(msg->data.data))){
				res.data.svc_status=svc->status;
				break;
			}
			res.action=ACTION_OK;
			res.data.status.ret=errno;
		}break;
		case ACTION_SVC_DUMP:svc_dump_services();break;
		case ACTION_NONE:case ACTION_OK:case ACTION_FAIL:break;
		default:res.action=ACTION_FAIL,res.data.status.ret=ENOSYS;
	}
	init_send_data(clt->fd,&res);
	return 0;
}

const char*action2string(enum init_action act){
	switch(act){
		case ACTION_POWEROFF:return "Power Off";
		case ACTION_HALT:return "Halt";
		case ACTION_REBOOT:return "Reboot";
		case ACTION_SWITCHROOT:return "Switch Root";
		case ACTION_ADDENV:return "Add Environ";
		case ACTION_LANGUAGE:return "Set Language";
		case ACTION_SVC_START:return "Start Service";
		case ACTION_SVC_STOP:return "Stop Service";
		case ACTION_SVC_RESTART:return "ReStart Service";
		case ACTION_SVC_RELOAD:return "ReLoad Service";
		case ACTION_SVC_DUMP:return "Dump All Service";
		case ACTION_SVC_STATUS:return "Get Service Status";
		default:return "Unknown";
	}
}

int init_send_data(int fd,struct init_msg*send){
	ssize_t s;
	if(fd<0)ERET(ENOTCONN);
	if(!init_check_msg(send))ERET(EINVAL);
	while(true){
		s=write(fd,send,sizeof(struct init_msg));
		if(s<0)switch(errno){
			case EINTR:continue;
			default:return -1;
		}else break;
	}
	if(s!=sizeof(struct init_msg))ERET(EIO);
	return 0;
}

int init_recv_data(int fd,struct init_msg*response){
	ssize_t s;
	if(fd<0)ERET(ENOTCONN);
	if(!response)ERET(EINVAL);
	while(true){
		memset(response,0,sizeof(struct init_msg));
		s=read(fd,response,sizeof(struct init_msg));
		if(s<0)switch(errno){
			case EINTR:continue;
			default:return -1;
		}else break;
	}
	if(s!=sizeof(struct init_msg))ERET(EIO);
	if(!init_check_msg(response))ERET(EPROTO);
	return 0;
}
