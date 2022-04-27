/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"str.h"
#include"system.h"
#include"logger.h"
#include"service.h"
#include"defines.h"
#define TAG "service"

static int _svc_proc_status_dump(int ident,struct proc_status*status){
	int i;
	char prefix[BUFSIZ];
	for(i=0;i<ident&&i<BUFSIZ-1;i++)prefix[i]=' ';
	prefix[i+1]=0;
	tlog_debug("%sproc status %p:",prefix,status);
	tlog_debug("%s    running:      %s",  prefix,BOOL2STR(status->running));
	tlog_debug("%s    timeout:      %s",      prefix,BOOL2STR(status->timeout));
	if(status->start!=0)tlog_debug("%s    start time:   %ld",     prefix,status->start);
	if(status->active!=0)tlog_debug("%s    active time:  %ld",     prefix,status->active);
	if(status->finish!=0)tlog_debug("%s    finish time:  %ld",     prefix,status->finish);
	if(status->pid!=0)tlog_debug("%s    pid:          %d",      prefix,status->pid);
	if(!status->running){
		tlog_debug("%s    exit code:    %d",      prefix,status->exit_code);
		if(status->exit_signal>0)tlog_debug("%s    exit signal:  %d %s",  prefix,status->exit_signal,signame(status->exit_signal));
	}
	return 0;
}

static int _svc_exec_dump(int ident,struct svc_exec*exec){
	if(!exec)ERET(EINVAL);
	int i;
	char prefix[BUFSIZ];
	for(i=0;i<ident&&i<BUFSIZ-1;i++)prefix[i]=' ';
	prefix[i+1]=0;
	MUTEX_LOCK(exec->lock);
	tlog_debug("%sservice execute %p:",prefix,exec);
	tlog_debug("%s    prop:",prefix);
	tlog_debug("%s        name:         %s",     prefix,exec->prop.name);
	tlog_debug("%s        service:      %p (%s)",prefix,exec->prop.svc,exec->prop.svc?exec->prop.svc->name:"null");
	tlog_debug("%s        type:         %s",     prefix,svc_exec_type_string(exec->prop.type));
	tlog_debug("%s        user:         %d:%d",  prefix,exec->prop.uid,exec->prop.gid);
	tlog_debug("%s        timeout:      %ld",    prefix,exec->prop.timeout);
	tlog_debug("%s    status:",prefix);
	_svc_proc_status_dump(ident+8,&exec->status);
	tlog_debug("%s    exec:",prefix);
	switch(exec->prop.type){
		case TYPE_UNKNOWN:tlog_debug("%s        (Unknown)",prefix);break;
		case TYPE_FUNCTION:tlog_debug("%s        enter point:      %p",prefix,exec->exec.func);break;
		case TYPE_COMMAND:
			if(exec->exec.cmd.path)tlog_debug("%s        path:             %p",prefix,exec->exec.cmd.path);
			if(exec->exec.cmd.args){
				tlog_debug("%s        args:",prefix);
				for(i=0;exec->exec.cmd.args[i];i++)tlog_debug("%s            %d =          %s",prefix,i,exec->exec.cmd.args[i]);
			}
			if(exec->exec.cmd.environ){
				tlog_debug("%s        environ:",prefix);
				for(i=0;exec->exec.cmd.environ[i];i++)tlog_debug("%s            %d =          %s",prefix,i,exec->exec.cmd.environ[i]);
			}
		break;
		case TYPE_LIBRARY:
			tlog_debug("%s        library:                  %s",prefix,exec->exec.lib.library);
			tlog_debug("%s        symbol:                   %s",prefix,exec->exec.lib.symbol);
		break;
	}
	MUTEX_UNLOCK(exec->lock);
	return 0;
}

static int _svc_list_dump_simple(int ident,list*svcs){
	int i;
	char prefix[BUFSIZ];
	for(i=0;i<ident&&i<BUFSIZ-1;i++)prefix[i]=' ';
	prefix[i+1]=0;
	list*next,*cur;
	if((next=list_first(svcs)))do{
		cur=next,next=cur->next;
		LIST_DATA_DECLARE(s,cur,struct service*);
		if(s)tlog_debug("%s%p (%s)",prefix,s,s->name);
	}while(next);
	return 0;
}

static int _svc_dump(int ident,struct service*svc){
	if(!svc)ERET(EINVAL);
	char prefix[BUFSIZ];
	int i;
	for(i=0;i<ident&&i<BUFSIZ-1;i++)prefix[i]=' ';
	prefix[i+1]=0;
	MUTEX_LOCK(svc->lock);
	tlog_debug("%sservice %p:",prefix,svc);
	tlog_debug("%s    name:             %s",     prefix,svc->name);
	if(svc->description)tlog_debug("%s    description:      %s",     prefix,svc->description);
	tlog_debug("%s    mode:             %s",     prefix,svc_work_string(svc->mode));
	tlog_debug("%s    status:           %s",     prefix,svc_status_string(svc->status));
	tlog_debug("%s    stop on shutdown: %s",     prefix,BOOL2STR(svc->stop_on_shutdown));
	tlog_debug("%s    auto restart:     %s",     prefix,BOOL2STR(svc->auto_restart));
	tlog_debug("%s    ignore failed:    %s",     prefix,BOOL2STR(svc->ignore_failed));
	tlog_debug("%s    stdio to syslog:  %s",     prefix,BOOL2STR(svc->stdio_syslog));
	if(svc->restart_delay>0)tlog_debug("%s    restart delay:    %ld",    prefix,svc->restart_delay);
	tlog_debug("%s    restart retry:    %d/%d",  prefix,svc->retry,svc->restart_max);
	if(svc->pid_file)tlog_debug("%s    pid file:         %s",     prefix,svc->pid_file);

	if(svc->depends_on){
		tlog_debug("%s    depend on:",prefix);
		_svc_list_dump_simple(i+8,svc->depends_on);
	}

	if(svc->depends_of){
		tlog_debug("%s    depend of:",prefix);
		_svc_list_dump_simple(i+8,svc->depends_of);
	}

	if(svc->start){
		tlog_debug("%s    start exec:",prefix);
		_svc_exec_dump(i+8,svc->start);
	}

	if(svc->stop){
		bool def=svc->stop->exec.func==svc_default_stop;
		tlog_debug("%s    stop exec:%s",prefix,def?" (default)":"");
		if(!def)_svc_exec_dump(i+8,svc->stop);
	}

	if(svc->reload){
		bool def=svc->reload->exec.func==svc_default_reload;
		tlog_debug("%s    reload exec:%s",prefix,def?" (default)":"");
		if(!def)_svc_exec_dump(i+8,svc->reload);
	}

	if(svc->restart){
		bool def=svc->restart->exec.func==svc_default_restart;
		tlog_debug("%s    restart exec:%s",prefix,def?" (default)":"");
		if(!def)_svc_exec_dump(i+8,svc->restart);
	}

	_svc_proc_status_dump(i+4,&svc->process);

	MUTEX_UNLOCK(svc->lock);
	return 0;
}

static int _svc_list_dump(int ident,list*svcs){
	list*cur,*next;
	if(!svcs)ERET(EINVAL);
	if((next=list_first(svcs)))do{
		cur=next,next=cur->next;
		LIST_DATA_DECLARE(s,cur,struct service*);
		if(s)_svc_dump(ident,s);
	}while(next);
	return 0;
}

int svc_proc_status_dump(struct proc_status*status){
	return _svc_proc_status_dump(0,status);
}

int svc_exec_dump(struct svc_exec*exec){
	return _svc_exec_dump(0,exec);
}

int svc_dump(struct service*svc){
	return _svc_dump(0,svc);
}

int svc_list_dump(list*svcs){
	return _svc_list_dump(0,svcs);
}

int svc_dump_services(){
	return svc_list_dump(services);
}
