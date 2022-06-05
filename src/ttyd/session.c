/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<grp.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/stat.h>
#include"confd.h"
#include"shell.h"
#include"logger.h"
#include"system.h"
#include"ttyd_internal.h"
#define TAG "tty"

static void tty_try_start_shell(const char*shell){
	if(!shell)return;
	char*sh=strrchr(shell,'/');
	char base[256]={0};
	snprintf(base,255,"-%s",sh?sh+1:"sh");
	errno=0;
	if(execl(shell,base,NULL)!=0&&shell&&*shell)
		telog_debug("execute shell %s failed",shell);
}

static int tty_setup_user(struct tty_data*data){
	fchown(STDOUT_FILENO,data->uid,data->gid);
	fchmod(STDOUT_FILENO,0600);
	if(chdir(data->home)!=0){
		fprintf(stderr,"chdir %s failed: %m\n",data->home);
		telog_warn("tty %s chdir %s failed",data->name,data->home);
		chdir("/");
		return 1;
	}
	int n=0;
	gid_t gl[256]={0};
	getgrouplist(data->user,data->gid,gl,&n);
	if(n>=256){
		fprintf(stderr,"too many group\n");
		telog_warn("tty %s user %s too many group",data->name,data->user);
		return 1;
	}
	setgroups(n,gl);
	endgrent();
	close_all_fd((int[]){logfd},1);
	fcntl(logfd,F_SETFD,FD_CLOEXEC);
	if(setgid(data->gid)!=0){
		fprintf(stderr,"setgid %s failed: %m\n",data->group);
		telog_warn("tty %s setgid %s failed",data->name,data->group);
		return 1;
	}
	if(setuid(data->uid)!=0){
		fprintf(stderr,"setuid %s failed: %m\n",data->user);
		telog_warn("tty %s setuid %s failed",data->name,data->user);
		return 1;
	}
	return 0;
}

static void tty_setup_environ(struct tty_data*data){
	char*pa=getenv("PATH");
	clearenv();
	setenv("PATH",pa,1);
	setenv("SHELL",data->shell,1);
	setenv("HOME",data->home,1);
	setenv("PWD",data->home,1);
	setenv("PWD",data->home,1);
	setenv("USER",data->user,1);
	char buf[PATH_MAX];
	snprintf(buf,PATH_MAX-1,"%d",data->uid);
	setenv("UID",buf,1);
	char*lang=confd_get_string("language",NULL);
	if(lang){
		setenv("LANG",lang,1);
		setenv("LANGUAGE",lang,1);
		setenv("LC_ALL",lang,1);
		free(lang);
	}
}

static int tty_start_shell(struct tty_data*data){
	tty_try_start_shell(data->shell);
	tty_try_start_shell(getenv("SHELL"));
	tty_try_start_shell("/bin/sh");
	#ifdef ENABLE_READLINE
	run_shell();
	#endif
	tlog_warn("no usable shell found");
	puts("No usable shell found.");
	putchar(7);
	sleep(10);
	return 1;
}

int tty_start_session(struct tty_data*data){
	if(
		!data->user[0]||
		!data->group[0]||
		!data->home[0]||
		!data->shell[0]
	){
		fprintf(stderr,"invalid user\n");
		tlog_warn("tty %s invalid user\n",data->name);
		sleep(5);
		return 1;
	}
	if(tty_setup_user(data)!=0){
		sleep(5);
		return 1;
	}
	tty_setup_environ(data);
	signal(SIGINT,SIG_DFL);
	return tty_start_shell(data);
}
