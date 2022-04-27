/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include<libgen.h>
#include<stdbool.h>
#include"list.h"
#include"lock.h"
#include"array.h"
#include"confd.h"
#include"service.h"
#include"defines.h"
#define TAG "service"

static bool svc_conf_parse_exec_cmd(const char*key,struct svc_exec*exec){
	char**array=NULL,**t;
	if(!key||!exec)return false;
	if(exec->exec.cmd.path)free(exec->exec.cmd.path);
	if(exec->exec.cmd.args)array_free(exec->exec.cmd.args);
	if(exec->exec.cmd.environ)array_free(exec->exec.cmd.environ);
	exec->exec.cmd.path=NULL,exec->exec.cmd.args=NULL,exec->exec.cmd.environ=NULL;
	if(confd_get_type_base(key,"path")==TYPE_STRING){
		exec->exec.cmd.path=confd_get_string_base(key,"path",NULL);
		if((t=confd_ls_base(key,"args"))){
			size_t size=sizeof(char*);
			for(size_t i=0;t[i];i++)size+=sizeof(char*);
			if(!(array=malloc(size)))EDONE(tlog_warn("alloc cmd args failed"));
			memset(array,0,size);
			for(size_t i=0;t[i];i++)
				array[i]=confd_get_string_dict(key,"args",t[i],NULL);
			if(t[0])free(t[0]);
			free(t);
		}
		if(!array)EDONE(tlog_warn("invalid command arguments"));
		exec->exec.cmd.args=array;
	}else if(confd_get_type_base(key,"command")==TYPE_STRING){
		char*cmd=confd_get_string_base(key,"command",NULL);
		if(!(array=args2array(cmd,0))||!array[0])EDONE();
		if((exec->exec.cmd.path=strdup(array[0])))
			exec->exec.cmd.args=array_dup(array);
		free_args_array(array);
		if(!exec->exec.cmd.args)EDONE();
	}else EDONE(tlog_warn("no command specified"));
	if((t=confd_ls_base(key,"environ"))){
		size_t size=sizeof(char*);
		for(size_t i=0;t[i];i++)size+=sizeof(char*);
		memset(array,0,size);
		if(!(array=malloc(size)))EDONE(tlog_warn("alloc environ failed"));
		for(size_t i=0;t[i];i++){
			char*val=confd_get_string_dict(key,"environ",t[i],NULL);
			if(!val)continue;
			size_t len=strlen(t[i])+strlen(val)+4;
			if(!(array[i]=malloc(len))){
				if(t[0])free(t[0]);
				free(t);
				free(val);
				EDONE(tlog_warn("alloc environ item failed"));
			}
			memset(array[i],0,len);
			snprintf(array[i],len-1,"%s=%s",t[i],val);
			free(val);
		}
		if(t[0])free(t[0]);
		free(t);
		exec->exec.cmd.environ=array;
	}
	return true;
	done:
	if(array)array_free(array);
	return false;
}

static struct svc_exec*svc_conf_parse_exec(
	const char*base,
	const char*path,
	struct svc_exec**e,
	struct service*svc,
	char*name
){
	struct svc_exec*exec=NULL;
	char*t,key[4096],rn[256];
	if(!base||!path||!svc||!name)EPRET(EINVAL);
	memset(rn,0,sizeof(rn));
	memset(key,0,sizeof(key));
	if(!svc)strncpy(rn,name,sizeof(rn)-1);
	else snprintf(rn,sizeof(rn)-1,"%s %s",svc->name,name);
	snprintf(key,sizeof(key)-1,"%s.%s",base,path);
	switch(confd_get_type(key)){
		case TYPE_KEY:break;
		case TYPE_STRING:
		case TYPE_INTEGER:
		case TYPE_BOOLEAN:EDONE(tlog_warn(
			"invalid service exec config"
		));
		default:return NULL;
	}
	if(!(exec=svc_new_exec(rn)))EPRET(ENOMEM);
	exec->prop.svc=svc;
	exec->prop.uid=confd_get_integer_base(key,"uid",exec->prop.uid);
	exec->prop.gid=confd_get_integer_base(key,"gid",exec->prop.gid);
	exec->prop.timeout=confd_get_integer_base(key,"timeout",exec->prop.timeout);
	if((t=confd_get_string_base(
		key,"type",
		(char*)svc_exec_type_short_string(exec->prop.type)
	))){
		bool b=short_string_svc_exec_type(t,&exec->prop.type);
		free(t);
		if(!b)EDONE(tlog_warn("unsupported exec type"));
	}
	switch(exec->prop.type){
		case TYPE_COMMAND:
			if(!svc_conf_parse_exec_cmd(key,exec))goto done;
		break;
		default:EDONE(tlog_warn("unimplement exec type in config"));
	}
	if(e){
		if(*e)svc_free_exec(*e);
		*e=exec;
	}
	return exec;
	done:
	if(!exec)svc_free_exec(exec);
	return NULL;
}

struct service*svc_conf_parse_service(
	const char*base,
	const char*name,
	struct service*s
){
	char*t,key[4096],**cs;
	struct service*svc=s,*x;
	enum svc_work work=WORK_UNKNOWN;
	if(!base||!name)EPRET(EINVAL);
	memset(key,0,sizeof(key));
	snprintf(key,sizeof(key)-1,"%s.%s",base,name);
	if((t=confd_get_string_base(key,"work",NULL))){
		bool b=short_string_svc_work(t,&work);
		free(t);
		if(!b)EDONE(tlog_warn(
			"unsupported service work mode"
		));
	}
	if(!svc&&!(s=svc=svc_lookup_by_name((char*)name))){
		if(work==WORK_UNKNOWN)EDONE(tlog_warn(
			"invalid service work mode"
		));
		if(!(svc=svc_new_service((char*)name,work)))
			EDONE(tlog_warn("alloc new service failed"));
		MUTEX_LOCK(svc->lock);
	}else{
		MUTEX_LOCK(svc->lock);
		if(work!=WORK_UNKNOWN)svc->mode=work;
		work=svc->mode;
	}
	if((t=confd_get_string_base(key,"desc",NULL))){
		if(svc->description)free(svc->description);
		svc->description=t;
	}
	if((t=confd_get_string_base(key,"pid_file",NULL))){
		if(svc->pid_file)free(svc->pid_file);
		svc->pid_file=t;
	}
	svc->terminal_output_signal=confd_get_boolean_base(key,"termout_signal",svc->terminal_output_signal);
	svc->stop_on_shutdown=confd_get_boolean_base(key,"stop_on_shutdown",svc->stop_on_shutdown);
	svc->ignore_failed=confd_get_boolean_base(key,"ignore_failed",svc->ignore_failed);
	svc->restart_delay=confd_get_boolean_base(key,"restart_delay",svc->restart_delay);
	svc->auto_restart=confd_get_boolean_base(key,"auto_restart",svc->auto_restart);
	svc->stdio_syslog=confd_get_boolean_base(key,"stdio_syslog",svc->stdio_syslog);
	svc->restart_max=confd_get_boolean_base(key,"restart_max",svc->restart_max);
	if(work!=WORK_FAKE){
		svc_conf_parse_exec(key,"restart",&svc->restart,svc,"restart");
		svc_conf_parse_exec(key,"reload",&svc->reload,svc,"reload");
		svc_conf_parse_exec(key,"start",&svc->start,svc,"start");
		svc_conf_parse_exec(key,"stop",&svc->stop,svc,"stop");
		if(!svc->start)EDONE(tlog_warn("no start execute set"))
		if(!svc->stop)svc_set_stop_function(svc,svc_default_stop);
		if(!svc->reload)svc_set_reload_function(svc,svc_default_reload);
		if(!svc->restart)svc_set_restart_function(svc,svc_default_restart);
	}
	time(&svc->last_update);
	MUTEX_UNLOCK(svc->lock);
	if((cs=confd_ls_base(key,"depends_on"))){
		for(size_t i=0;cs[i];i++){
			if(!(t=confd_get_string_dict(key,"depends_on",cs[i],NULL)))continue;
			if(!(x=svc_lookup_by_name(t)))tlog_warn("service %s not found",t);
			else if(svc_add_depend(svc,x)!=0)tlog_warn("add depend %s failed",t);
			free(t);
		}
		if(cs[0])free(cs[0]);
		free(cs);
	}
	if((cs=confd_ls_base(key,"depends_of"))){
		for(size_t i=0;cs[i];i++){
			if(!(t=confd_get_string_dict(key,"depends_of",cs[i],NULL)))continue;
			if(!(x=svc_lookup_by_name(t)))tlog_warn("service %s not found",t);
			else if(svc_add_depend(x,svc)!=0)tlog_warn("add depend %s failed",t);
			free(t);
		}
		if(cs[0])free(cs[0]);
		free(cs);
	}
	if(!s)svc_add_service(svc);
	return svc;
	done:
	MUTEX_UNLOCK(svc->lock);
	if(!s&&svc)svc_free_service(svc);
	return NULL;
}

void svc_conf_parse_services(const char*base){
	char**ss=confd_ls(base);
	if(ss){
		for(size_t i=0;ss[i];i++)
			svc_conf_parse_service(base,ss[i],NULL);
		if(ss[0])free(ss[0]);
		free(ss);
	}
}
