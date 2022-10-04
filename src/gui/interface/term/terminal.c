/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#ifdef ENABLE_GUI
#ifdef ENABLE_LIBTSM
#include<errno.h>
#include<string.h>
#include<stddef.h>
#include<stdlib.h>
#include<signal.h>
#include<stdbool.h>
#include<pthread.h>
#include<semaphore.h>
#include<sys/select.h>
#include<sys/wait.h>
#include"gui.h"
#include"lock.h"
#include"shell.h"
#include"system.h"
#include"logger.h"
#include"libtsm.h"
#include"defines.h"
#include"shl-pty.h"
#include"gui/console.h"
#include"gui/activity.h"
#include"gui/termview.h"
#define TAG "terminal"

struct terminal{
	struct console*con;
	pthread_t tid;
	int pty_bridge;
	pid_t pid;
	struct shl_pty*pty;
	sem_t cont;
};

static void terminal_pty_clean(struct terminal*term){
	if(term->tid!=0)pthread_cancel(term->tid);
	if(term->pty_bridge>0){
		if(term->pty)shl_pty_bridge_remove(
			term->pty_bridge,term->pty
		);
		shl_pty_bridge_free(term->pty_bridge);
	}
	if(term->pty)shl_pty_unref(term->pty);
	if(term->pid>0)waitpid(term->pid,NULL,0);
	term->tid=0,term->pty_bridge=0;
	term->pty=NULL,term->pid=0;
}

static void pty_dispatch_task(void*d){
	struct terminal*term=d;
	shl_pty_bridge_dispatch(term->pty_bridge,0);
	lv_termview_update(term->con->termview);
	sem_post(&term->cont);
}

static void*pty_dispatch_thread(void*data){
	int st=0;
	fd_set fds;
	bool run=true;
	struct timeval t;
	struct terminal*term=data;
	while(run){
		FD_ZERO(&fds);
		FD_SET(term->pty_bridge,&fds);
		t.tv_sec=1,t.tv_usec=0;
		if(
			term->pid>0&&
			waitpid(term->pid,&st,WNOHANG)==term->pid
		){
			MUTEX_LOCK(gui_lock);
			if(WIFSIGNALED(st))lv_termview_line_printf(
				term->con->termview,
				_("[Process %d exited by signal %s (%d)]"),
				term->pid,signame(WTERMSIG(st)),WTERMSIG(st)
			);
			else if(WIFEXITED(st))lv_termview_line_printf(
				term->con->termview,
				_("[Process %d exited with status %d]"),
				term->pid,WEXITSTATUS(st)
			);
			else lv_termview_line_printf(
				term->con->termview,
				_("[Process %d exited]"),
				term->pid
			);
			MUTEX_UNLOCK(gui_lock);
			term->con->allow_exit=true;
			term->pid=0;
			run=false;
			continue;
		}
		if(select(FD_SETSIZE,&fds,NULL,NULL,&t)<0)switch(errno){
			case EAGAIN:usleep(50000);//fallthrough
			case EINTR:continue;
			default:
				telog_warn("select failed");
				MUTEX_LOCK(gui_lock);
				lv_termview_line_printf(
					term->con->termview,
					_("[syscall select failed: %s]"),
					_(strerror(errno))
				);
				MUTEX_UNLOCK(gui_lock);
				run=false;
				continue;
		}
		MUTEX_LOCK(gui_lock);
		lv_async_call(pty_dispatch_task,term);
		MUTEX_UNLOCK(gui_lock);
		sem_wait(&term->cont);
	}
	MUTEX_LOCK(gui_lock);
	term->tid=0;
	terminal_pty_clean(term);
	MUTEX_UNLOCK(gui_lock);
	return NULL;
}

static void exec_shell(const char*shell){
	errno=0;
	if(!shell)return;
	char*sh=strrchr(shell,'/');
	if(execl(shell,sh?sh+1:"sh",NULL)!=0&&shell&&*shell)
		telog_warn("execute shell %s failed",shell);
}

static void terminal_read_cb(
	struct shl_pty*pty,
	void*data,
	char*u8,
	size_t len
){
	struct terminal*term=data;
	if(!term||!term->con||!pty||term->pty!=pty)return;
	struct tsm_vte*vte=lv_termview_get_vte(term->con->termview);
	tsm_vte_input(vte,u8,len);
	lv_termview_update(term->con->termview);
}

static void term_write(
	lv_obj_t*tv,
	const char*u8,
	size_t len
){
	struct console*con=lv_obj_get_user_data(tv);
	struct terminal*term=con->data;
	if(term->pty){
		shl_pty_write(term->pty,u8,len);
		shl_pty_dispatch(term->pty);
	}
}

static void term_resize(
	lv_obj_t*tv,
	uint32_t cols,
	uint32_t rows
){
	struct console*con=lv_obj_get_user_data(tv);
	struct terminal*term=con->data;
	if(term->pty)shl_pty_resize(term->pty,cols,rows);
}

static int terminal_launch(struct gui_activity*act){
	int r;
	struct console*con=act->data;
	struct terminal*term=con->data;
	if(term->pid>0)return 0;
	term->pid=shl_pty_open(
		&term->pty,
		terminal_read_cb,term,
		lv_termview_get_cols(con->termview),
		lv_termview_get_rows(con->termview)
	);
	if(term->pid<0)telog_error("pty open failed");
	else if(term->pid==0){
		close_all_fd(NULL,0);
		setenv("TERM","xterm-256color",1);
		setenv("COLORTERM","tsm",1);
		exec_shell(getenv("SHELL"));
		exec_shell(_PATH_BIN"/sh");
		#ifdef ENABLE_READLINE
		run_shell();
		#endif
		telog_error("run shell failed");
		_exit(1);
		return -1;
	}
	tlog_debug("child process %d started",term->pid);
	if((r=shl_pty_bridge_add(
		term->pty_bridge,
		term->pty
	))<0)tlog_error("pty bridge add failed: %d",r);
	pthread_create(
		&term->tid,NULL,
		pty_dispatch_thread,
		term
	);
	return 0;
}

static int terminal_clean(struct gui_activity*d){
	struct console*con=d->data;
	struct terminal*term=con->data;
	terminal_pty_clean(term);
	memset(term,0,sizeof(struct terminal));
	free(term);
	con->data=NULL;
	return console_clean(d);
}

static int terminal_init(struct gui_activity*act){
	int r;
	struct console*con=NULL;
	struct terminal*term=NULL;
	if((r=console_init(act))!=0)return r;
	if(
		!(con=act->data)||
		!(term=malloc(sizeof(struct terminal)))
	)goto done;
	memset(term,0,sizeof(struct terminal));
	con->data=term,term->con=con,term->pty_bridge=-1;
	if((r=shl_pty_bridge_new())<0)EDONE(tlog_warn(
		"pty bridge initialize failed: %d",r
	));
	term->pty_bridge=r;
	sem_init(&term->cont,0,0);
	return 0;
	done:
	if(term){
		if(term->pty_bridge>=0)
			shl_pty_bridge_free(term->pty_bridge);
		free(term);
		console_clean(act);
	}
	return -1;
}

static int terminal_draw(struct gui_activity*act){
	int r=0;
	if((r=console_draw(act))!=0)return r;
	struct console*con=act->data;
	lv_termview_set_write_cb(
		con->termview,
		term_write
	);
	lv_termview_set_resize_cb(
		con->termview,
		term_resize
	);
	return r;
}

struct gui_register guireg_terminal={
	.name="terminal",
	.title="Terminal",
	.show_app=true,
	.init=terminal_init,
	.draw=terminal_draw,
	.data_load=terminal_launch,
	.resize=console_resize,
	.ask_exit=console_do_back,
	.quiet_exit=terminal_clean,
	.lost_focus=console_lost_focus,
	.get_focus=console_get_focus,
	.back=true,
};
#endif
#endif
