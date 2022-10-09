/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<string.h>
#include<stdlib.h>
#include"init_internal.h"
#include"service.h"
#include"version.h"
#include"system.h"
#include"gadget.h"
#include"logger.h"
#include"confd.h"
#include"adbd.h"
#include"ttyd.h"
#define TAG "gadget"

static char*base="gadget.func";

static void init_gadget_conf(){
	char*udc=confd_get_string("runtime.cmdline.udc",NULL);
	char*serial=confd_get_string("runtime.cmdline.serial","1234567890");
	if(!serial)return;
	if(!udc)udc=strdup(gadget_find_udc());

	confd_set_string("gadget.name","gadget");
	confd_set_integer("gadget.id_vendor",0x0519);
	confd_set_integer("gadget.id_product",0x5016);
	confd_set_string("gadget.manufacturer","Linux");
	confd_set_string("gadget.product",NAME);
	confd_set_string("gadget.serial",serial);
	confd_set_string("gadget.config",NAME);
	if(udc)confd_set_string("gadget.udc",udc);

	confd_set_string_array(base,0,"name","usb0");
	confd_set_string_array(base,0,"func","rndis");
	confd_set_string_array(base,0,"mode","generic");

	confd_set_boolean_array(base,1,"console",true);
	confd_set_string_array(base,1,"name","ttyGS0");
	confd_set_string_array(base,1,"func","acm");
	confd_set_string_array(base,1,"mode","console");

	confd_set_string_array(base,2,"name","adb");
	confd_set_string_array(base,2,"func","ffs");
	confd_set_string_array(base,2,"path",_PATH_RUN"/adb");
	confd_set_string_array(base,2,"mode","adbd");

	if(udc)free(udc);
	free(serial);
}

static int gadget_init_console(char*item,gadget*g,gadget_func*f){
	char buf[256]={0},tty[512]={0};
	if(gadget_add_function(g,f)<0)
		return trlog_warn(-1,"add gadget console func %s failed",item);
	if(!confd_get_boolean_dict(base,item,"console",false))return 0;
	if(fd_read_file(
		g->dir_fd,buf,sizeof(buf),false,
		"functions/%s.%s/port_num",
		f->function,f->name
	)<0)return trlog_warn(-1,"read console func %s port number failed",item);
	snprintf(tty,511,"ttyGS%s",buf);
	confd_set_string_dict(base,item,"tty",tty);
	if(confd_get_type_base("runtime.ttyd.tty",tty)==TYPE_KEY)return 0;
	if(confd_get_type_base("ttyd.tty",tty)==TYPE_KEY)return 0;
	tlog_debug("add console %s to ttyd",tty);
	confd_set_boolean_dict("runtime.ttyd.tty",tty,"enabled",true);
	confd_set_boolean_dict("runtime.ttyd.tty",tty,"start_msg",true);
	check_open_default_ttyd_socket(false,TAG);
	ttyd_reload();
	return 0;
}

static int gadget_init_generic(char*item,gadget*g,gadget_func*f){
	if(gadget_add_function(g,f)<0)tlog_warn("add gadget func %s failed",item);
	return 0;
}

static int gadget_init_adbd(char*item,gadget*g,gadget_func*f){
	char*path=confd_get_string_dict(base,item,"path",NULL);
	if(!path){
		tlog_warn("no path specified for adbd");
		return -1;
	}
	if(gadget_add_func_adbd(g,f->name,path)<0)
		tlog_warn("add gadget adbd func %s failed",item);
	if(path)free(path);
	return 0;
}

static int gadget_init_mass(char*item,gadget*g,gadget_func*f){
	struct stat st;
	bool removable=confd_get_boolean_dict(base,item,"removable",true);
	bool cdrom=confd_get_boolean_dict(base,item,"cdrom",false);
	bool ro=confd_get_boolean_dict(base,item,"ro",false);
	char*path=confd_get_string_dict(base,item,"path",NULL);
	if(!path)return trlog_warn(-1,"no path specified for mass storage");
	if(path[0]!='/')return trlog_warn(-1,"block path is not absolute");
	if(stat(path,&st)!=0)return terlog_warn(-1,"stat path failed");
	if(!S_ISBLK(st.st_mode)&&!(S_ISREG(st.st_mode)&&st.st_size!=0))
		return trlog_warn(-1,"block is invalid");
	f->values=(keyval*[]){
		&KV("lun.0/removable",removable?"1":"0"),
		&KV("lun.0/cdrom",cdrom?"1":"0"),
		&KV("lun.0/ro",ro?"1":"0"),
		&KV("lun.0/file",path),
		NULL
	};
	if(gadget_add_function(g,f)<0)
		tlog_warn("add gadget mass storage func %s failed",item);
	if(path)free(path);
	return 0;
}

static int gadget_startup(struct service*svc __attribute__((unused))){
	#define XERR(msg...) {tlog_error(msg);goto fail;}
	struct gadget_string gs={
		.id=0x409,.serialnumber=NULL,
		.product=NULL,.manufacturer=NULL,
	};
	gadget_cfgstr gcs={.id=0x409,.configuration=NULL};
	gadget_cfg gc={
		.strings=GADGET_CFGSTRARRAY{&gcs,NULL},
		.max_power=500,.type="a",.id=1,
	};
	struct gadget g={
		.strings = GADGET_STRARRAY{&gs,NULL},
		.configs = GADGET_CFGARRAY{&gc,NULL},
	};
	char**items,*item,*udc;
	open_default_confd_socket(false,TAG);
	if(confd_get_type("gadget")!=TYPE_KEY)init_gadget_conf();
	if(!(g.name=confd_get_string("gadget.name",NULL)))XERR("invalid gadget name")
	if((g.vendor=confd_get_integer("gadget.id_vendor",0))==0)XERR("invalid vendor id")
	if((g.product=confd_get_integer("gadget.id_product",0))==0)XERR("invalid product id")
	if(!(gs.manufacturer=confd_get_string("gadget.manufacturer",NULL)))XERR("invalid manufacturer")
	if(!(gs.product=confd_get_string("gadget.product",NULL)))XERR("invalid product")
	if(!(gs.serialnumber=confd_get_string("gadget.serial",NULL)))XERR("invalid serial")
	if(!(gcs.configuration=confd_get_string("gadget.config",NULL)))XERR("invalid config")
	tlog_info("register gadget");
	if(gadget_register(&g)<0)XERR("failed to register gadget");
	if(!(items=confd_ls(base)))XERR("failed to read functions list")
	for(int i=0;(item=items[i]);i++){
		gadget_func f={0};
		char*mode;
		f.name=confd_get_string_dict(base,item,"name",NULL);
		f.function=confd_get_string_dict(base,item,"func",NULL);
		mode=confd_get_string_dict(base,item,"mode","generic");
		if(!f.name||!f.function||!mode)tlog_warn("invalid name or func, skip func %d",i);
		else if(strcmp(mode,"generic")==0)gadget_init_generic(item,&g,&f);
		else if(strcmp(mode,"console")==0)gadget_init_console(item,&g,&f);
		else if(strcmp(mode,"adbd")==0)gadget_init_adbd(item,&g,&f);
		else if(strcmp(mode,"mass")==0)gadget_init_mass(item,&g,&f);
		else tlog_warn("unknown gadget mode %s, skip func %s",mode,item);
		if(f.name)free(f.name);
		if(f.function)free(f.function);
		if(mode)free(mode);
	}
	if(items[0])free(items[0]);
	free(items);
	if(!(udc=confd_get_string("gadget.udc",gadget_find_udc())))XERR("get gadget udc failed")
	int r=gadget_start(&g,udc);
	if(r<0)tlog_error("start gadget failed with %s",udc);
	free(udc);
	if(r<0)goto fail;
	tlog_info("usb gadget initialized");
	return 0;
	fail:
	if(g.name)free(g.name);
	if(gs.product)free(gs.product);
	if(gs.manufacturer)free(gs.manufacturer);
	if(gs.serialnumber)free(gs.serialnumber);
	if(gcs.configuration)free(gcs.configuration);
	return -1;
}

static int gadget_shutdown(struct service*svc __attribute__((unused))){
	open_socket_initfd(DEFAULT_INITD,false);
	open_default_confd_socket(false,TAG);
	pid_t p=(pid_t)confd_get_integer("runtime.pid.adbd",0);
	if(p>0)kill(p,SIGTERM);
	bool changed=false;
	char**items,*item;
	if((items=confd_ls(base)))for(int i=0;(item=items[i]);i++){
		if(!confd_get_boolean_dict(base,item,"console",false))continue;
		char*tty=confd_get_string_dict(base,item,"tty",NULL);
		if(!tty)continue;
		confd_delete_base("runtime.ttyd.tty",tty);
		pid_t xp=(pid_t)confd_get_integer_base("runtime.ttyd.client",tty,0);
		if(xp>0)kill(xp,SIGTERM);
		changed=true;
		free(tty);
	}
	if(changed){
		struct init_msg msg,response;
		init_initialize_msg(&msg,ACTION_SVC_RESTART);
		strcpy(msg.data.data,"ttyd");
		init_send(&msg,&response);
	}
	char*gadget=confd_get_string("gadget.name",NULL);
	if(gadget){
		gadget_unregister(gadget);
		free(gadget);
	}
	return 0;
}

int register_gadget_service(){
	struct service*gadget=svc_create_service("usb-gadget",WORK_ONCE);
	if(gadget){
		svc_set_desc(gadget,"USB Gadget Startup");
		svc_set_start_function(gadget,gadget_startup);
		svc_set_stop_function(gadget,gadget_shutdown);
		svc_add_depend(svc_system,gadget);
	}
	return 0;
}
