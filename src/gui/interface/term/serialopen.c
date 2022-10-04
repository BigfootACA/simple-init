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
#include<ctype.h>
#include<errno.h>
#include<string.h>
#include<stddef.h>
#include<stdlib.h>
#include<stdbool.h>
#ifdef ENABLE_UEFI
#include<Protocol/SerialIo.h>
#include<Library/UefiBootServicesTableLib.h>
#else
#include<dirent.h>
#endif
#include"gui.h"
#include"list.h"
#include"confd.h"
#include"serial.h"
#include"logger.h"
#include"defines.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#include"gui/termview.h"
#define TAG "serial"

#define DEFAULT_SPEED 115200

struct serial_port{
	uint32_t id;
	char name[256-sizeof(void*)-sizeof(uint32_t)];
	#ifdef ENABLE_UEFI
	EFI_SERIAL_IO_PROTOCOL*proto;
	#else
	char port[256+sizeof(void*)];
	#endif
};

struct serial_open{
	lv_obj_t*box,*title;
	lv_obj_t*port,*speed;
	lv_obj_t*ok,*cancel;
	bool changed;
	list*ports;
};

static int serial_open_clean(struct gui_activity*d){
	struct serial_open*so=d->data;
	if(!so)return 0;
	free(so);
	d->data=NULL;
	return 0;
}

static int serial_open_init(struct gui_activity*act){
	struct serial_open*so=NULL;
	if(!(so=malloc(sizeof(struct serial_open))))return -ENOMEM;
	memset(so,0,sizeof(struct serial_open));
	act->data=so;
	return 0;
}

static void ok_cb(lv_event_t*e){
	list*l;
	struct serial_port*port=NULL;
	static struct serial_port_cfg cfg;
	struct serial_open*so=e->user_data;
	uint32_t p=lv_dropdown_get_selected(so->port);
	uint32_t s=lv_dropdown_get_selected(so->speed);
	memset(&cfg,0,sizeof(cfg));
	if(p!=0&&(l=list_first(so->ports)))do{
		LIST_DATA_DECLARE(n,l,struct serial_port*);
		if(n->id==p)port=n;
	}while((l=l->next));
	if(!port){
		msgbox_alert("Please select a serial port");
		return;
	}
	if(s>0)cfg.baudrate=serial_baudrates[s-1].number;
	#ifdef ENABLE_UEFI
	cfg.proto=port->proto;
	tlog_debug("use port %p with baudrate %d",cfg.proto,cfg.baudrate);
	#else
	strncpy(cfg.port,port->port,sizeof(cfg.port)-1);
	tlog_debug("use port %s with baudrate %d",cfg.port,cfg.baudrate);
	#endif
	if(confd_get_boolean("gui.serial.save_status",true)){
		#ifndef ENABLE_UEFI
		confd_set_string("gui.serial.port",cfg.port);
		#endif
		confd_set_integer("gui.serial.speed",cfg.baudrate);
	}
	guiact_start_activity(&guireg_serial_port,&cfg);
}

#ifndef ENABLE_UEFI
static bool dev_sort(list*f1,list*f2){
	LIST_DATA_DECLARE(p1,f1,struct serial_port*);
	LIST_DATA_DECLARE(p2,f2,struct serial_port*);
	return strcmp(p1->name,p2->name)>0;
}
#endif

static int serial_open_load(struct gui_activity*act){
	list*l;
	uint32_t p=0;
	struct baudrate*b;
	struct serial_port port;
	struct serial_open*so=act->data;
	unsigned int def_speed=DEFAULT_SPEED;
	char*def_port=NULL;
	if(!so)return -1;
	def_speed=confd_get_integer("gui.serial.default_speed",def_speed);
	def_speed=confd_get_integer("gui.serial.speed",def_speed);
	def_port=confd_get_string("gui.serial.default_port",def_port);
	def_port=confd_get_string("gui.serial.port",def_port);
	list_free_all_def(so->ports);
	so->ports=NULL;
	#ifdef ENABLE_UEFI
	UINTN cnt=0;
	EFI_STATUS st;
	EFI_HANDLE*hands=NULL;
	st=gBS->LocateHandleBuffer(
		ByProtocol,
		&gEfiSerialIoProtocolGuid,
		NULL,&cnt,&hands
	);
	if(!EFI_ERROR(st)&&hands){
		for(UINTN i=0;i<cnt;i++){
			EFI_SERIAL_IO_PROTOCOL*sio=NULL;
			st=gBS->HandleProtocol(
				hands[i],
				&gEfiSerialIoProtocolGuid,
				(VOID**)&sio
			);
			if(EFI_ERROR(st)||!sio)continue;
			memset(&port,0,sizeof(port));
			snprintf(
				port.name,sizeof(port.name)-1,
				"%p (SerialIO)",sio
			);
			port.proto=sio;
			list_obj_add_new_dup(&so->ports,&port,sizeof(port));
		}
	}else tlog_error(
		"locate Serial IO Protocol failed: %s",
		efi_status_to_string(st)
	);
	#else
	DIR*d=opendir(_PATH_DEV);
	if(d){
		struct dirent*r;
		while((r=readdir(d))){
			if(r->d_type!=DT_CHR)continue;
			if(strlen(r->d_name)<4)continue;
			if(strncmp(r->d_name,"tty",3)!=0)continue;
			if(isdigit(r->d_name[3]))continue;
			memset(&port,0,sizeof(port));
			strncpy(port.name,r->d_name,sizeof(port.name)-1);
			snprintf(
				port.port,sizeof(port.port)-1,
				_PATH_DEV"/%s",r->d_name
			);
			list_obj_add_new_dup(&so->ports,&port,sizeof(port));
		}
		closedir(d);
	}else telog_warn("open "_PATH_DEV" failed");
	#endif
	lv_dropdown_clear_options(so->port);
	lv_dropdown_add_option(so->port,_("(none)"),0);
	if(so->ports){
		tlog_debug(
			"found %d serial port device",
			list_count(so->ports)
		);
		#ifndef ENABLE_UEFI
		list_sort(so->ports,dev_sort);
		#endif
		if((l=list_first(so->ports)))do{
			LIST_DATA_DECLARE(n,l,struct serial_port*);
			n->id=++p;
			lv_dropdown_add_option(so->port,n->name,n->id);
			#ifndef ENABLE_UEFI
			if(!so->changed){
				char*x=act->args;
				if(!x)x=def_port;
				if(!x)continue;
				if(x&&x[1]==':'&&x[2]=='/')x+=2;
				if(strcmp(x,n->port)==0)
					lv_dropdown_set_selected(so->port,n->id);
			}
			#endif
		}while((l=l->next));
	}else tlog_warn("no any serial port found");
	lv_dropdown_clear_options(so->speed);
	lv_dropdown_add_option(so->speed,_("(Keep default)"),0);
	for(size_t i=0;(b=&serial_baudrates[i])->speed;i++){
		lv_dropdown_add_option(so->speed,b->name,i+1);
		if(!so->changed&&b->number==def_speed)
			lv_dropdown_set_selected(so->speed,i+1);
	}
	return 0;
}

static int serial_open_draw(struct gui_activity*act){
	struct serial_open*so=act->data;
	if(!so)return -1;
	so->box=lv_draw_dialog_box(act->page,&so->title,"Open Serial Port");
	lv_draw_dropdown(so->box,"Port:",&so->port);
	lv_draw_dropdown(so->box,"Speed:",&so->speed);
	lv_draw_btns_ok_cancel(so->box,&so->ok,&so->cancel,ok_cb,so);
	return 0;
}

static int serial_open_get_focus(struct gui_activity*act){
	struct serial_open*so=act->data;
	if(!so)return 0;
	lv_group_add_obj(gui_grp,so->port);
	lv_group_add_obj(gui_grp,so->speed);
	lv_group_add_obj(gui_grp,so->ok);
	lv_group_add_obj(gui_grp,so->cancel);
	return 0;
}

static int serial_open_lost_focus(struct gui_activity*act){
	struct serial_open*so=act->data;
	if(!so)return 0;
	lv_group_remove_obj(so->port);
	lv_group_remove_obj(so->speed);
	lv_group_remove_obj(so->ok);
	lv_group_remove_obj(so->cancel);
	return 0;
}

struct gui_register guireg_serial_open={
	.name="serial-open",
	.title="Open Serial Port",
	#ifndef ENABLE_UEFI
	.open_file=true,
	.open_regex=(char*[]){
		"^([A-Za-z]:|)/dev/tty[A-Za-z]+[0-9]+$",
		NULL
	},
	#endif
	.show_app=true,
	.init=serial_open_init,
	.draw=serial_open_draw,
	.data_load=serial_open_load,
	.quiet_exit=serial_open_clean,
	.lost_focus=serial_open_lost_focus,
	.get_focus=serial_open_get_focus,
	.back=true,
	.mask=true,
};
#endif
#endif
