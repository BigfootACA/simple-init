#ifndef SERVICE_H
#define SERVICE_H
#include<time.h>
#include<stdbool.h>
#include<sys/types.h>
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
	pthread_mutex_t lock;
};

struct service{
	char*name;
	char*description;
	char*pid_file;
	bool stop_on_shutdown;
	bool auto_restart;
	struct proc_status process;
	enum svc_work mode;
	list*depends_on;
	list*depends_of;
	enum svc_status status;
	struct svc_exec*start;
	struct svc_exec*stop;
	struct svc_exec*restart;
	struct svc_exec*reload;
	pthread_mutex_t lock;
};

#endif
