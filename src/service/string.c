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
		case TYPE_UNKNOWN:return  "Unknown";
		case TYPE_FUNCTION:return "Function";
		case TYPE_COMMAND:return  "Command";
		case TYPE_LIBRARY:return  "Library";
	}
	return NULL;
}

const char*svc_status_string(enum svc_status status){
	switch(status){
		case STATUS_UNKNOWN:return  "Unknown";
		case STATUS_STOPPED:return  "Stopped";
		case STATUS_STARTING:return "Starting";
		case STATUS_STARTED:return  "Started";
		case STATUS_RUNNING:return  "Running";
		case STATUS_STOPPING:return "Stopping";
		case STATUS_FAILED:return   "Failed";
	}
	return NULL;
}


const char*svc_work_string(enum svc_work work){
	switch(work){
		case WORK_UNKNOWN:return    "Unknown";
		case WORK_FAKE:return       "Fake";
		case WORK_FOREGROUND:return "Foreground";
		case WORK_DAEMON:return     "Daemon";
		case WORK_ONCE:return       "Once";
	}
	return NULL;
}
