#define _GNU_SOURCE
#include<string.h>
#include<stdlib.h>
#include"service.h"
#include"version.h"
#include"gadget.h"
#include"logger.h"
#include"confd.h"
#include"adbd.h"
#define TAG "gadget"

static char*base="gadget.func";

static void init_gadget_conf(){
	char*udc=confd_get_string("runtime.cmdline.udc",NULL);
	char*serial=confd_get_string("runtime.cmdline.serial","1234567890");
	if(!serial)return;
	if(!udc)udc=gadget_find_udc();

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

	confd_set_string_array(base,1,"name","ttyGS0");
	confd_set_string_array(base,1,"func","acm");
	confd_set_string_array(base,1,"mode","generic");

	confd_set_string_array(base,2,"name","adb");
	confd_set_string_array(base,2,"func","ffs");
	confd_set_string_array(base,2,"path",_PATH_RUN"/adb");
	confd_set_string_array(base,2,"mode","adbd");

	free(serial);
}

static int gadget_startup(struct service*svc __attribute__((unused))){
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
	open_default_confd_socket(TAG);
	if(confd_get_type("gadget")!=TYPE_KEY)init_gadget_conf();
	if(
		!(g.name=confd_get_string("gadget.name",NULL))||
		(g.vendor=confd_get_integer("gadget.id_vendor",0))==0||
		(g.product=confd_get_integer("gadget.id_product",0))==0||
		!(gs.manufacturer=confd_get_string("gadget.manufacturer",NULL))||
		!(gs.product=confd_get_string("gadget.product",NULL))||
		!(gs.serialnumber=confd_get_string("gadget.serial",NULL))||
		!(gcs.configuration=confd_get_string("gadget.config",NULL))
	){
		tlog_error("invalid config");
		goto fail;
	}
	tlog_info("register gadget");
	if(gadget_register(&g)<0){
		tlog_error("failed to register gadget");
		goto fail;
	}
	if(!(items=confd_ls(base))){
		tlog_error("failed to read functions list");
		goto fail;
	}
	for(int i=0;(item=items[i]);i++){
		gadget_func f={0};
		char*path,*mode;
		f.name=confd_get_string_dict(base,item,"name",NULL);
		f.function=confd_get_string_dict(base,item,"func",NULL);
		path=confd_get_string_dict(base,item,"path",NULL);
		mode=confd_get_string_dict(base,item,"mode","generic");
		if(!f.name||!f.function||!mode){
			tlog_warn("invalid name or func, skip func %d",i);
			goto cont;
		}
		if(strcmp(mode,"generic")==0){
			if(gadget_add_function(&g,&f)<0){
				tlog_warn("add gadget func %s failed",item);
			}
		}else if(strcmp(mode,"adbd")==0){
			if(!path){
				tlog_warn("no path specified for adbd");
				goto cont;
			}
			if(gadget_add_func_adbd(&g,f.name,path)<0){
				tlog_warn("add gadget adbd func %s failed",item);
			}
		}else tlog_warn("unknown gadget mode %s, skip func %s",mode,item);
		cont:
		if(f.name)free(f.name);
		if(f.function)free(f.function);
		if(path)free(path);
		if(mode)free(mode);
	}
	if((udc=confd_get_string("gadget.udc",NULL))){
		int r=gadget_start(&g,udc);
		if(r!=0)tlog_error("start gadget failed with %s",udc);
		free(udc);
		if(r!=0)return -1;
	}
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

int register_gadget_service(){
	struct service*gadget=svc_create_service("usb-gadget",WORK_ONCE);
	if(gadget){
		svc_set_desc(gadget,"USB Gadget Startup");
		svc_set_start_function(gadget,gadget_startup);
		svc_add_depend(svc_system,gadget);
	}
	return 0;
}
