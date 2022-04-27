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
#include"service.h"
#include"defines.h"
#define _xfree(ptr) if(ptr)free(ptr)

int svc_exec_set_name(struct svc_exec*exec,char*name){
	if(!exec||!name)ERET(EINVAL);
	if(strlen(name)>64)ERET(ENAMETOOLONG);
	MUTEX_LOCK(exec->lock);
	if(exec->prop.name)free(exec->prop.name);
	exec->prop.name=strdup(name);
	MUTEX_UNLOCK(exec->lock);
	if(!exec->prop.name)ERET(ENOMEM);
	else return 0;
}

struct service*svc_new_service(char*name,enum svc_work mode){
	if(!name)EPRET(EINVAL);
	struct service*svc=malloc(sizeof(struct service));
	if(!svc)EPRET(ENOMEM);
	memset(svc,0,sizeof(struct service));
	if(svc_set_name(svc,name)<0)goto fail;
	if(svc_set_stop_function(svc,svc_default_stop)<0)goto fail;
	if(svc_set_reload_function(svc,svc_default_reload)<0)goto fail;
	if(svc_set_restart_function(svc,svc_default_restart)<0)goto fail;
	MUTEX_INIT(svc->lock);
	svc->mode=mode;
	svc->retry=0;
	svc->restart_max=5;
	svc->status=STATUS_STOPPED;
	svc->stdio_syslog=true;
	svc->stop_on_shutdown=true;
	return svc;
	fail:
	svc_free_service(svc);
	return NULL;
}

struct svc_exec*svc_new_exec(char*name){
	if(!name)EPRET(EINVAL);
	struct svc_exec*exec=malloc(sizeof(struct svc_exec));
	if(!exec)EPRET(ENOMEM);
	memset(exec,0,sizeof(struct svc_exec));
	if(svc_exec_set_name(exec,name)<0){
		free(exec);
		return NULL;
	}
	MUTEX_INIT(exec->lock);
	exec->prop.timeout=30;
	return exec;
}

static void _free_exec_cont(struct svc_exec*exec){
	switch(exec->prop.type){
		case TYPE_COMMAND:
			array_free(exec->exec.cmd.args);
			_xfree(exec->exec.cmd.path);
		break;
		case TYPE_LIBRARY:
			_xfree(exec->exec.lib.library);
			_xfree(exec->exec.lib.symbol);
		break;
		default:;
	}
	memset(&exec->exec,0,sizeof(exec->exec));
}

void svc_free_exec(struct svc_exec*exec){
	if(!exec)return;
	MUTEX_DESTROY(exec->lock);
	_xfree(exec->prop.name);
	_free_exec_cont(exec);
	free(exec);
}

void svc_free_service(struct service*svc){
	if(!svc)return;
	MUTEX_DESTROY(svc->lock);
	_xfree(svc->name);
	_xfree(svc->description);
	list_free_all(svc->depends_on,NULL);
	list_free_all(svc->depends_of,NULL);
	svc_free_exec(svc->start);
	svc_free_exec(svc->stop);
	svc_free_exec(svc->restart);
	svc_free_exec(svc->reload);
	free(svc);
}

int svc_exec_set_command(struct svc_exec*exec,char*path,char**args,char**envs){
	if(!exec||!path)ERET(EINVAL);
	if(args&&!args[0])ERET(EINVAL);
	char*_path=NULL,**_args=NULL,**_envs=NULL;
	if(
		!(_path=strdup(path))||
		!(_envs=array_dup(envs?envs:environ))||
		!(_args=array_dup(args?
			args:
			(char*[]){basename(_path),NULL}
		))
	){
		_xfree(_path);
		array_free(_args);
		array_free(_envs);
		ERET(ENOMEM);
	}
	MUTEX_LOCK(exec->lock);
	_free_exec_cont(exec);
	exec->prop.type=TYPE_COMMAND;
	exec->exec.cmd.path=_path;
	exec->exec.cmd.args=_args;
	exec->exec.cmd.environ=_envs;
	MUTEX_UNLOCK(exec->lock);
	return 0;
}

int svc_exec_set_library(struct svc_exec*exec,char*library,char*symbol){
	if(!exec||!library||!symbol)ERET(EINVAL);
	char*_library=NULL,*_symbol=NULL;
	if(
		!(_library=strdup(library))||
		!(_symbol=strdup(symbol))
	){
		_xfree(_library);
		_xfree(_symbol);
		ERET(ENOMEM);
	}
	MUTEX_LOCK(exec->lock);
	_free_exec_cont(exec);
	exec->prop.type=TYPE_LIBRARY;
	exec->exec.lib.library=_library;
	exec->exec.lib.symbol=_symbol;
	MUTEX_UNLOCK(exec->lock);
	return 0;
}

int svc_exec_set_function(struct svc_exec*exec,svc_main*main){
	if(!exec||!main)ERET(EINVAL);
	_free_exec_cont(exec);
	MUTEX_LOCK(exec->lock);
	exec->prop.type=TYPE_FUNCTION;
	exec->exec.func=main;
	MUTEX_UNLOCK(exec->lock);
	return 0;
}

struct service*svc_create_service(char*name,enum svc_work mode){
	struct service*svc=svc_new_service(name,mode);
	if(!svc)return NULL;
	if(svc_add_service(svc)!=0){
		svc_free_service(svc);
		return NULL;
	}
	return svc;
}

static int _free_service(void*d){
	if(d)svc_free_service((struct service*)d);
	return 0;
}

int svc_free_all_services(list*l){
	return list_free_all(l,_free_service);
}
