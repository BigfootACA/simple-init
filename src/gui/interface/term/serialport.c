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
#ifdef ENABLE_UEFI
#include<Protocol/SerialIo.h>
#include<Library/UefiBootServicesTableLib.h>
#else
#include<termios.h>
#include<pthread.h>
#include<semaphore.h>
#include<sys/select.h>
#endif
#include"gui.h"
#include"lock.h"
#include"serial.h"
#include"system.h"
#include"logger.h"
#include"libtsm.h"
#include"defines.h"
#include"gui/console.h"
#include"gui/activity.h"
#include"gui/termview.h"
#define TAG "serial"

#ifdef ENABLE_UEFI
#define ERRSTR _(efi_status_to_string(st))
#else
#define ERRSTR _(strerror(errno))
#endif
struct serial_port{
	struct console*con;
	bool open;
	int fd;
	unsigned int speed;
	char port[256];
	#ifdef ENABLE_UEFI
	lv_timer_t*task;
	EFI_SERIAL_IO_PROTOCOL*proto;
	#else
	pthread_t tid;
	struct termios old_opt;
	#endif
};

static void serial_port_close(struct serial_port*port){
	#ifdef ENABLE_UEFI
	if(port->task)lv_timer_del(port->task);
	port->proto=NULL,port->task=NULL;
	#else
	if(port->tid!=0)pthread_cancel(port->tid);
	if(port->fd>0)close(port->fd);
	port->tid=0,port->fd=-1;
	#endif
	lv_termview_line_printf(
		port->con->termview,
		_("[port closed]")
	);
}

#ifdef ENABLE_UEFI
static void serial_port_read_task(lv_timer_t*tsk){
	UINTN bs;
	EFI_STATUS st;
	char buf[256];
	struct serial_port*port=tsk->user_data;
	if(!port->open||!port->proto||port->task!=tsk)return;
	struct tsm_vte*vte=lv_termview_get_vte(
		port->con->termview
	);
	do{
		bs=sizeof(buf);
		st=port->proto->Read(port->proto,&bs,buf);
		if(bs>0){
			tsm_vte_input(vte,buf,(size_t)bs);
			lv_termview_update(port->con->termview);
		}
		if(st!=EFI_TIMEOUT)lv_termview_line_printf(
			port->con->termview,
			_("[receive data failed: %s]"),
			ERRSTR
		);
	}while(bs>0);
}
#else
static void*serial_port_read_thread(void*data){
	fd_set fds;
	ssize_t bs=0;
	bool run=true;
	char buf[256];
	struct timeval t;
	struct serial_port*port=data;
	struct tsm_vte*vte=lv_termview_get_vte(
		port->con->termview
	);
	while(run){
		FD_ZERO(&fds);
		FD_SET(port->fd,&fds);
		t.tv_sec=1,t.tv_usec=0;
		if(select(FD_SETSIZE,&fds,NULL,NULL,&t)<0)switch(errno){
			case EAGAIN:usleep(50000);//fallthrough
			case EINTR:continue;
			default:
				telog_warn("select failed");
				MUTEX_LOCK(gui_lock);
				lv_termview_line_printf(
					port->con->termview,
					_("[syscall select failed: %s]"),
					ERRSTR
				);
				MUTEX_UNLOCK(gui_lock);
				run=false;
				continue;
		}
		if(FD_ISSET(port->fd,&fds)){
			memset(buf,0,sizeof(buf));
			bs=read(port->fd,buf,sizeof(buf));
			MUTEX_LOCK(gui_lock);
			if(bs<0){
				if(errno==EINTR)continue;
				if(errno==EAGAIN)continue;
				lv_termview_line_printf(
					port->con->termview,
					_("[receive data failed: %s]"),
					ERRSTR
				);
				run=false;
			}else if(bs==0)run=false;
			else if(bs>0){
				tsm_vte_input(vte,buf,(size_t)bs);
				lv_termview_update(port->con->termview);
			}
			MUTEX_UNLOCK(gui_lock);
		}
	}
	MUTEX_LOCK(gui_lock);
	port->tid=0;
	serial_port_close(port);
	MUTEX_UNLOCK(gui_lock);
	return NULL;
}
#endif

static void term_write(
	lv_obj_t*tv,
	const char*u8,
	size_t len
){
	struct console*con=lv_obj_get_user_data(tv);
	struct serial_port*port=con->data;
	if(!port->open)return;
	#ifdef ENABLE_UEFI
	EFI_STATUS st;
	if(port->proto){
		UINTN wr=len;
		st=port->proto->Write(port->proto,&wr,(VOID*)u8);
		if(EFI_ERROR(st))goto fail;
	}
	#else
	if(port->fd>=0){
		errno=0;
		ssize_t r=full_write(port->fd,(void*)u8,len);
		if(r<0&&errno>0)goto fail;
	}
	#endif
	return;
	fail:lv_termview_line_printf(
		tv,_("[send data failed: %s]"),
		ERRSTR
	);
}

static int serial_set_speed(struct serial_port*port){
	#ifdef ENABLE_UEFI
	EFI_STATUS st;
	#endif
	struct baudrate*b,*t=NULL;
	if(!port||port->open||port->speed==0)return 0;
	for(size_t i=0;(b=&serial_baudrates[i])->speed;i++)
		if(port->speed==b->number)t=b;
	if(!t){
		#ifdef ENABLE_UEFI
		st=EFI_INVALID_PARAMETER;
		#else
		errno=EINVAL;
		#endif
		goto fail;
	}
	#ifdef ENABLE_UEFI
	if(!port->proto)return 0;
	st=port->proto->SetAttributes(
		port->proto,
		t->number,0,0,
		DefaultParity,0,
		DefaultStopBits
	);
	if(EFI_ERROR(st))goto fail;
	#else
	errno=0;
	struct termios pts;
	if(port->fd<0)return 0;
	if(tcgetattr(port->fd,&pts)<0)goto fail;
	if(cfsetospeed(&pts,t->speed)<0)goto fail;
	if(cfsetispeed(&pts,t->speed)<0)goto fail;
	if(tcsetattr(port->fd,TCSANOW,&pts)<0)goto fail;
	#endif
	return 0;
	fail:lv_termview_line_printf(
		port->con->termview,
		_("[setup serial port speed failed: %s]"),
		ERRSTR
	);
	return -1;
}

#define FAIL(fmt...) {lv_termview_line_printf(con->termview,fmt);return 0;}
static int serial_port_launch(struct gui_activity*act){
	struct console*con=act->data;
	struct serial_port*port=con->data;
	struct tsm_screen*scr=lv_termview_get_screen(con->termview);
	if(port->open)return 0;
	tsm_screen_clear_sb(scr);
	tsm_screen_reset(scr);
	if(act->args){
		struct serial_port_cfg*cfg=act->args;
		#ifdef ENABLE_UEFI
		port->proto=cfg->proto;
		#else
		strncpy(
			port->port,cfg->port,
			sizeof(port->port)-1
		);
		#endif
		port->speed=cfg->baudrate;
	}
	#ifdef ENABLE_UEFI
	if(!port->proto)FAIL(_("[no serial port specified]"));
	serial_set_speed(port);
	port->open=true;
	port->task=lv_timer_create(serial_port_read_task,50,port);
	#else
	if(port->port[0]){
		errno=0;
		if(port->fd<0&&(port->fd=open(
			port->port,
			O_RDWR|O_NONBLOCK|O_CLOEXEC
		))<0)FAIL(
			_("[open port %s failed: %s]"),
			port->port,ERRSTR
		);
		serial_set_speed(port);
		port->open=true;
		tlog_debug(
			"open serial port %s as %d",
			port->port,port->fd
		);
		lv_termview_line_printf(
			con->termview,
			_("[serial port %s opened]"),
			port->port
		);
	}else FAIL(_("[no serial port specified]"));
	pthread_create(
		&port->tid,NULL,
		serial_port_read_thread,port
	);
	#endif
	return 0;
}

static int serial_port_clean(struct gui_activity*d){
	struct console*con=d->data;
	struct serial_port*port=con->data;
	serial_port_close(port);
	memset(port,0,sizeof(struct serial_port));
	free(port);
	con->data=NULL;
	return console_clean(d);
}

static int serial_port_init(struct gui_activity*act){
	int r;
	struct console*con=NULL;
	struct serial_port*port=NULL;
	if((r=console_init(act))!=0)return r;
	if(
		!(con=act->data)||
		!(port=malloc(sizeof(struct serial_port)))
	)goto done;
	memset(port,0,sizeof(struct serial_port));
	con->data=port,port->con=con,port->fd=-1;
	return 0;
	done:
	if(port){
		free(port);
		console_clean(act);
	}
	return -1;
}

static int serial_port_draw(struct gui_activity*act){
	int r=0;
	if((r=console_draw(act))!=0)return r;
	struct console*con=act->data;
	lv_termview_set_write_cb(
		con->termview,
		term_write
	);
	return r;
}

struct gui_register guireg_serial_port={
	.name="serial-port",
	.title="Serial Port",
	.init=serial_port_init,
	.draw=serial_port_draw,
	.data_load=serial_port_launch,
	.resize=console_resize,
	.ask_exit=console_do_back,
	.quiet_exit=serial_port_clean,
	.lost_focus=console_lost_focus,
	.get_focus=console_get_focus,
	.back=true,
};
#endif
#endif
