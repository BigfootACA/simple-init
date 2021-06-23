#include<string.h>
#include<stdlib.h>
#include<pthread.h>
#include"system.h"
#include"logger.h"
#include"service.h"
#include"defines.h"
#define TAG "service"

static const char*bool2string(bool b){
	return b?"true":"false";
}

static int _svc_proc_status_dump(int ident,struct proc_status*status){
	int i;
	char prefix[BUFSIZ];
	for(i=0;i<ident&&i<BUFSIZ-1;i++)prefix[i]=' ';
	prefix[i+1]=0;
	tlog_debug("%sproc status %p:",prefix,status);
	tlog_debug("%s    running:      %s",  prefix,bool2string(status->running));
	tlog_debug("%s    timeout:      %s",      prefix,bool2string(status->timeout));
	tlog_debug("%s    start time:   %ld",     prefix,status->start);
	tlog_debug("%s    active time:  %ld",     prefix,status->active);
	tlog_debug("%s    finish time:  %ld",     prefix,status->finish);
	tlog_debug("%s    pid:          %d",      prefix,status->pid);
	tlog_debug("%s    exit code:    %d",      prefix,status->exit_code);
	tlog_debug("%s    exit signal:  %d(%s)",  prefix,status->exit_signal,signame(status->exit_signal));
	return 0;
}

static int _svc_exec_dump(int ident,struct svc_exec*exec){
	if(!exec)ERET(EINVAL);
	int i;
	char prefix[BUFSIZ];
	for(i=0;i<ident&&i<BUFSIZ-1;i++)prefix[i]=' ';
	prefix[i+1]=0;
	pthread_mutex_lock(&exec->lock);
	tlog_debug("%sservice execute %p:",prefix,exec);
	tlog_debug("%s    prop:",prefix);
	tlog_debug("%s        name:         %s",     prefix,exec->prop.name);
	tlog_debug("%s        service:      %p (%s)",prefix,exec->prop.svc,exec->prop.svc?exec->prop.name:"null");
	tlog_debug("%s        type:         %s",     prefix,svc_exec_type_string(exec->prop.type));
	tlog_debug("%s        user:         %d:%d",  prefix,exec->prop.uid,exec->prop.gid);
	tlog_debug("%s        timeout:      %ld",    prefix,exec->prop.timeout);
	tlog_debug("%s    status:",prefix);
	_svc_proc_status_dump(ident+4,&exec->status);
	tlog_debug("%s    exec:",prefix);
	switch(exec->prop.type){
		case TYPE_UNKNOWN:tlog_debug("%s        (Unknown)",prefix);break;
		case TYPE_FUNCTION:tlog_debug("%s        enter point:      %p",prefix,exec->exec.func);break;
		case TYPE_COMMAND:
			tlog_debug("%s        path:             %p",prefix,exec->exec.cmd.path);
			tlog_debug("%s        args:",prefix);
			for(i=0;exec->exec.cmd.args[i];i++)tlog_debug("%s            %d =          %s",prefix,i,exec->exec.cmd.args[i]);
			tlog_debug("%s        environ:",prefix);
			for(i=0;exec->exec.cmd.environ[i];i++)tlog_debug("%s            %d =          %s",prefix,i,exec->exec.cmd.environ[i]);
			break;
		case TYPE_LIBRARY:
			tlog_debug("%s        library:                  %s",prefix,exec->exec.lib.library);
			tlog_debug("%s        symbol:                   %s",prefix,exec->exec.lib.symbol);
			break;
	}
	pthread_mutex_unlock(&exec->lock);
	return 0;
}

static int _svc_dump(int ident,struct service*svc){
	if(!svc)ERET(EINVAL);
	list*cur,*next;
	char prefix[BUFSIZ];
	int i;
	for(i=0;i<ident&&i<BUFSIZ-1;i++)prefix[i]=' ';
	prefix[i+1]=0;
	pthread_mutex_lock(&svc->lock);
	tlog_debug("%sservice execute %p:",prefix,svc);
	tlog_debug("%s    name:             %s",     prefix,svc->name);
	tlog_debug("%s    description:      %s",     prefix,svc->description);
	tlog_debug("%s    mode:             %s",     prefix,svc_work_string(svc->mode));
	tlog_debug("%s    status:           %s",     prefix,svc_status_string(svc->status));
	tlog_debug("%s    stop on shutdown: %s",     prefix,bool2string(svc->stop_on_shutdown));
	tlog_debug("%s    auto restart:     %s",     prefix,bool2string(svc->auto_restart));
	tlog_debug("%s    pid file:         %s",     prefix,svc->pid_file);
	tlog_debug("%s    depend on:",prefix);
	if(svc->depends_on&&(next=list_first(svc->depends_on)))do{
		cur=next,next=cur->next;
		LIST_DATA_DECLARE(s,cur,struct service*);
		if(s)tlog_debug("%s        %p (%s)",prefix,s,s->name);
	}while(next);
	tlog_debug("%s    depend of:",prefix);
	if(svc->depends_of&&(next=list_first(svc->depends_of)))do{
		cur=next,next=cur->next;
		LIST_DATA_DECLARE(s,cur,struct service*);
		if(s)tlog_debug("%s        %p (%s)",prefix,s,s->name);
	}while(next);
	tlog_debug("%s    start exec:",prefix);
	_svc_exec_dump(i+4,svc->start);
	tlog_debug("%s    stop exec:",prefix);
	_svc_exec_dump(i+4,svc->stop);
	tlog_debug("%s    reload exec:",prefix);
	_svc_exec_dump(i+4,svc->reload);
	tlog_debug("%s    restart exec:",prefix);
	_svc_exec_dump(i+4,svc->restart);
	pthread_mutex_unlock(&svc->lock);
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
