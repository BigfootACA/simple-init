/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef SERVICE_H
#define SERVICE_H
#include<time.h>
#include<stdbool.h>
#include<sys/types.h>
#include"lock.h"
#include"list.h"

enum svc_status{
	STATUS_UNKNOWN=0,
	STATUS_STOPPED,
	STATUS_STARTING,
	STATUS_STARTED,
	STATUS_RUNNING,
	STATUS_STOPPING,
	STATUS_FAILED,
};

enum svc_work{
	WORK_UNKNOWN=0,
	WORK_FAKE,
	WORK_FOREGROUND,
	WORK_DAEMON,
	WORK_ONCE,
};

enum svc_exec_type{
	TYPE_UNKNOWN=0,
	TYPE_FUNCTION,
	TYPE_COMMAND,
	TYPE_LIBRARY,
};

struct service;
typedef int svc_main(struct service*);

struct proc_status{
	time_t start;
	time_t active;
	time_t finish;
	pid_t pid;
	bool running;
	bool timeout;
	int exit_code;
	int exit_signal;
};

struct svc_exec{
	struct{
		char*name;
		uid_t uid;
		gid_t gid;
		time_t timeout;
		enum svc_exec_type type;
		struct service*svc;
	}prop;
	struct proc_status status;
	union{
		svc_main*func;
		struct{
			char*path;
			char**args;
			char**environ;
		}cmd;
		struct{
			char*library;
			char*symbol;
		}lib;
	}exec;
	mutex_t lock;
};

struct service{
	char*name;
	char*description;
	char*pid_file;
	bool terminal_output_signal;
	bool stop_on_shutdown;
	bool auto_restart;
	bool wait_restart;
	bool stdio_syslog;
	bool ignore_failed;
	time_t restart_delay;
	time_t last_update;
	int restart_max,retry;
	struct proc_status process;
	enum svc_work mode;
	enum svc_status status;
	list*depends_on;
	list*depends_of;
	struct svc_exec*start;
	struct svc_exec*stop;
	struct svc_exec*restart;
	struct svc_exec*reload;
	mutex_t lock;
};

// src/service/data.c: list of all services
extern list*services;

// src/service/service.c: should do auto restart
extern bool auto_restart;

// src/service/service.c: default service for boot
extern struct service*svc_default,*svc_system,*svc_network;

// src/service/data.c: lock for variable services
extern mutex_t services_lock;

// src/service/struct.c: set service execute name
extern int svc_exec_set_name(struct svc_exec*exec,char*name);

// src/service/struct.c: new service
extern struct service*svc_new_service(char*name,enum svc_work mode);

// src/service/struct.c: new service execute
extern struct svc_exec*svc_new_exec(char*name);

// src/service/struct.c: free service execute
extern void svc_free_exec(struct svc_exec*exec);

// src/service/struct.c: free service
extern void svc_free_service(struct service*svc);

// src/service/struct.c: free all services in a list
extern int svc_free_all_services(list*l);

// src/service/struct.c: set service execute command
extern int svc_exec_set_command(struct svc_exec*exec,char*path,char**args,char**envs);

// src/service/struct.c: set service execute library (not implemented)
extern int svc_exec_set_library(struct svc_exec*exec,char*library,char*symbol);

// src/service/struct.c: set service execute function
extern int svc_exec_set_function(struct svc_exec*exec,svc_main*main);

// src/service/data.c: check service is running
extern bool svc_is_running(struct service*svc);

// src/service/data.c: get service description. return name if description is not set
extern char*svc_get_desc(struct service*svc);

// src/service/data.c: create service and add to service storage
extern struct service*svc_create_service(char*name,enum svc_work mode);

// src/service/data.c: init service start execute
extern int svc_init_exec_start(struct service*svc);

// src/service/data.c: set service start function
extern int svc_set_start_function(struct service*svc,svc_main*main);

// src/service/data.c: set service start library (not implemented)
extern int svc_set_start_library(struct service*svc,char*library,char*symbol);

// src/service/data.c: set service start command
extern int svc_set_start_command(struct service*svc,char*path,char**args,char**envs);

// src/service/data.c: init service stop execute
extern int svc_init_exec_stop(struct service*svc);

// src/service/data.c: set service stop function
extern int svc_set_stop_function(struct service*svc,svc_main*main);

// src/service/data.c: set service stop library (not implemented)
extern int svc_set_stop_library(struct service*svc,char*library,char*symbol);

// src/service/data.c: set service stop command
extern int svc_set_stop_command(struct service*svc,char*path,char**args,char**envs);

// src/service/data.c: init service restart execute
extern int svc_init_exec_restart(struct service*svc);

// src/service/data.c: set service restart function
extern int svc_set_restart_function(struct service*svc,svc_main*main);

// src/service/data.c: set service restart library (not implemented)
extern int svc_set_restart_library(struct service*svc,char*library,char*symbol);

// src/service/data.c: set service restart command
extern int svc_set_restart_command(struct service*svc,char*path,char**args,char**envs);

// src/service/data.c: init service reload execute
extern int svc_init_exec_reload(struct service*svc);

// src/service/data.c: set service reload function
extern int svc_set_reload_function(struct service*svc,svc_main*main);

// src/service/data.c: set service reload library (not implemented)
extern int svc_set_reload_library(struct service*svc,char*library,char*symbol);

// src/service/data.c: set service reload command
extern int svc_set_reload_command(struct service*svc,char*path,char**args,char**envs);

// src/service/data.c: lookup service from storage by name
extern struct service*svc_lookup_by_name(char*name);

// src/service/sigchld.c: service SIGCHLD handler
extern int svc_on_sigchld(pid_t pid,int st);

// src/service/start.c: read daemon mode pid file
extern int svc_daemon_get_pid(struct service*svc);

// src/service/start.c: direct start service without depends
extern int svc_start_service_nodep(struct service*svc);

// src/service/start.c: direct start service
extern int svc_start_service(struct service*svc);

// src/service/start.c: direct start service by name
extern int svc_start_service_by_name(char*name);

// src/service/stop.c: direct stop service without depends
extern int svc_stop_service_nodep(struct service*svc);

// src/service/stop.c: direct stop service
extern int svc_stop_service(struct service*svc);

// src/service/stop.c: direct stop service by name
extern int svc_stop_service_by_name(char*name);

// src/service/service.c: set service name
extern int svc_set_name(struct service*svc,char*name);

// src/service/service.c: set service description
extern int svc_set_desc(struct service*svc,char*desc);

// src/service/service.c: add service to service storage
extern int svc_add_service(struct service*svc);

// src/service/service.c: add service depend
extern int svc_add_depend(struct service*svc,struct service*depend);

// src/service/service.c: stop all services and wait all services stopped
extern int service_wait_all_stop(void);

// src/service/service.c: init service framework
extern int service_init(void);

// src/service/service.c: direct restart service
extern int svc_restart_service(struct service*svc);

// src/service/service.c: direct reload service
extern int svc_reload_service(struct service*svc);

// src/service/execute.c: run service execute
extern int svc_run_exec(struct svc_exec*exec);

// src/service/scheduler.c: launch service scheduler
extern int start_scheduler(void);

// src/service/scheduler.c: stop service scheduler
extern int stop_scheduler(void);

// src/service/scheduler.c: service scheduler SIGCHLD handler
extern int service_sigchld(pid_t p,int st);

// src/service/scheduler.c: stop all services
extern int service_stop_all(void);

// src/service/scheduler.c: scheduler start service
extern int service_start(struct service*svc);

// src/service/scheduler.c: scheduler start service by name
extern int service_start_by_name(char*name);

// src/service/scheduler.c: scheduler stop service
extern int service_stop(struct service*svc);

// src/service/scheduler.c: scheduler stop service by name
extern int service_stop_by_name(char*name);

// src/service/scheduler.c: scheduler restart service
extern int service_restart(struct service*svc);

// src/service/scheduler.c: scheduler restart service by name
extern int service_restart_by_name(char*name);

// src/service/scheduler.c: scheduler reload service
extern int service_reload(struct service*svc);

// src/service/scheduler.c: scheduler reload service by name
extern int service_reload_by_name(char*name);

// src/service/scheduler.c: send terminal output signal to all service
extern int service_terminal_output(void);

// src/service/default.c: default stop execute of service
extern int svc_default_stop(struct service*svc);

// src/service/default.c: default restart execute of service
extern int svc_default_restart(struct service*svc);

// src/service/default.c: default reload execute of service
extern int svc_default_reload(struct service*svc);

// src/service/string.c: convert svc_exec_type to string
extern const char*svc_exec_type_string(enum svc_exec_type type);

// src/service/string.c: convert svc_status to string
extern const char*svc_status_string(enum svc_status status);

// src/service/string.c: convert svc_work to string
extern const char*svc_work_string(enum svc_work work);

// src/service/string.c: convert svc_exec_type to short string
const char*svc_exec_type_short_string(enum svc_exec_type type);

// src/service/string.c: convert svc_status to short string
const char*svc_status_short_string(enum svc_status status);

// src/service/string.c: convert svc_work to short string
const char*svc_work_short_string(enum svc_work work);

// src/service/string.c: convert short string to svc_exec_type
bool short_string_svc_exec_type(const char*str,enum svc_exec_type*type);

// src/service/string.c: convert short string to svc_status
bool short_string_svc_status(const char*str,enum svc_status*status);

// src/service/string.c: convert short string to svc_work
bool short_string_svc_work(const char*str,enum svc_work*work);

// src/service/dump.c: dump proc_status to loggerd
extern int svc_proc_status_dump(struct proc_status*status);

// src/service/dump.c: dump svc_exec to loggerd
extern int svc_exec_dump(struct svc_exec*exec);

// src/service/dump.c: dump single service to loggerd
extern int svc_dump(struct service*svc);

// src/service/dump.c: dump service list to loggerd
extern int svc_list_dump(list*svcs);

// src/service/dump.c: dump all services from variable services to loggerd
extern int svc_dump_services(void);

// src/service/conf.c: load service from config
extern struct service*svc_conf_parse_service(const char*base,const char*name,struct service*s);

// src/service/conf.c: load services from config
extern void svc_conf_parse_services(const char*base);

#endif
