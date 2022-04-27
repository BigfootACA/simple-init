/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"service.h"
#define TAG "service"

const char*svc_exec_type_string(enum svc_exec_type type){
	switch(type){
		case TYPE_FUNCTION:return "Function";
		case TYPE_COMMAND:return  "Command";
		case TYPE_LIBRARY:return  "Library";
		default:return            "Unknown";
	}
	return NULL;
}

const char*svc_status_string(enum svc_status status){
	switch(status){
		case STATUS_STOPPED:return  "Stopped";
		case STATUS_STARTING:return "Starting";
		case STATUS_STARTED:return  "Started";
		case STATUS_RUNNING:return  "Running";
		case STATUS_STOPPING:return "Stopping";
		case STATUS_FAILED:return   "Failed";
		default:return              "Unknown";
	}
	return NULL;
}

const char*svc_work_string(enum svc_work work){
	switch(work){
		case WORK_FAKE:return       "Fake";
		case WORK_FOREGROUND:return "Foreground";
		case WORK_DAEMON:return     "Daemon";
		case WORK_ONCE:return       "Once";
		default:return              "Unknown";
	}
	return NULL;
}

const char*svc_exec_type_short_string(enum svc_exec_type type){
	switch(type){
		case TYPE_FUNCTION:return "function";
		case TYPE_COMMAND:return  "command";
		case TYPE_LIBRARY:return  "library";
		default:return            "unknown";
	}
	return NULL;
}

const char*svc_status_short_string(enum svc_status status){
	switch(status){
		case STATUS_STOPPED:return  "stopped";
		case STATUS_STARTING:return "starting";
		case STATUS_STARTED:return  "started";
		case STATUS_RUNNING:return  "running";
		case STATUS_STOPPING:return "stopping";
		case STATUS_FAILED:return   "failed";
		default:return              "unknown";
	}
	return NULL;
}

const char*svc_work_short_string(enum svc_work work){
	switch(work){
		case WORK_FAKE:return       "fake";
		case WORK_FOREGROUND:return "foreground";
		case WORK_DAEMON:return     "daemon";
		case WORK_ONCE:return       "once";
		default:return              "unknown";
	}
	return NULL;
}

bool short_string_svc_exec_type(const char*str,enum svc_exec_type*type){
	if(!str||!*str||!type)return false;
	else if(strcasecmp(str,"function")==0)
		*type=TYPE_FUNCTION;
	else if(strcasecmp(str,"func")==0)
		*type=TYPE_FUNCTION;
	else if(strcasecmp(str,"command")==0)
		*type=TYPE_COMMAND;
	else if(strcasecmp(str,"cmd")==0)
		*type=TYPE_COMMAND;
	else if(strcasecmp(str,"library")==0)
		*type=TYPE_LIBRARY;
	else if(strcasecmp(str,"lib")==0)
		*type=TYPE_LIBRARY;
	else return false;
	return true;
}

bool short_string_svc_status(const char*str,enum svc_status*status){
	if(!str||!*str||!status)return false;
	else if(strcasecmp(str,"stopped")==0)
		*status=STATUS_STOPPED;
	else if(strcasecmp(str,"starting")==0)
		*status=STATUS_STARTING;
	else if(strcasecmp(str,"started")==0)
		*status=STATUS_STARTED;
	else if(strcasecmp(str,"running")==0)
		*status=STATUS_RUNNING;
	else if(strcasecmp(str,"stopping")==0)
		*status=STATUS_STOPPING;
	else if(strcasecmp(str,"failed")==0)
		*status=STATUS_FAILED;
	else return false;
	return true;
}

bool short_string_svc_work(const char*str,enum svc_work*work){
	if(!str||!*str||!work)return false;
	else if(strcasecmp(str,"fake")==0)
		*work=WORK_FAKE;
	else if(strcasecmp(str,"virtual")==0)
		*work=WORK_FAKE;
	else if(strcasecmp(str,"foreground")==0)
		*work=WORK_FOREGROUND;
	else if(strcasecmp(str,"fg")==0)
		*work=WORK_FOREGROUND;
	else if(strcasecmp(str,"daemon")==0)
		*work=WORK_DAEMON;
	else if(strcasecmp(str,"background")==0)
		*work=WORK_DAEMON;
	else if(strcasecmp(str,"bg")==0)
		*work=WORK_DAEMON;
	else if(strcasecmp(str,"once")==0)
		*work=WORK_ONCE;
	else if(strcasecmp(str,"oneshot")==0)
		*work=WORK_ONCE;
	else return false;
	return true;
}
