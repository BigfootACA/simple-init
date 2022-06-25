/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<stdlib.h>
#include"gui.h"
#include"list.h"
#include"confd.h"
#include"logger.h"
#include"defines.h"
#include"gui/guidrv.h"
#define TAG "guidrv"

static struct gui_driver*drv;
static list*indrvs=NULL;

int guidrv_getsize(lv_coord_t*w,lv_coord_t*h){
	errno=ENOENT;
	if(!drv)errno=ENOENT;
	else if(!drv->drv_getsize)errno=ENOSYS;
	else drv->drv_getsize(w,h);
	return errno==0?0:-1;
}

int guidrv_getdpi(int*dpi){
	errno=ENOENT;
	if(gui_dpi_force>0){
		*dpi=gui_dpi_force;
		return 0;
	}
	*dpi=gui_dpi_def;
	if(!drv)errno=ENOENT;
	else if(!drv->drv_getdpi)errno=ENOSYS;
	else drv->drv_getdpi(dpi);
	return errno==0?0:-1;
}

lv_coord_t guidrv_get_width(){
	lv_coord_t w=0;
	guidrv_getsize(&w,NULL);
	return w;
}

lv_coord_t guidrv_get_height(){
	lv_coord_t h=0;
	guidrv_getsize(NULL,&h);
	return h;
}

int guidrv_get_dpi(){
	int d=0;
	guidrv_getdpi(&d);
	return d;
}

const char*guidrv_getname(){
	errno=ENOENT;
	if(!drv)return "No Driver";
	errno=ENOSYS;
	if(!drv->name[0])return "Unknown";
	errno=0;
	return (char*)&drv->name;
}

bool guidrv_isname(const char*name){
	errno=ENOENT;
	if(!drv)return false;
	errno=ENOSYS;
	if(!drv->name[0])return false;
	errno=0;
	return strcasecmp(name,drv->name)==0;
}

int guidrv_taskhandler(){
	errno=0;
	if(!drv)ERET(ENOENT);
	if(!drv->drv_taskhandler)ERET(ENOSYS);
	drv->drv_taskhandler();
	return 0;
}

uint32_t guidrv_tickget(){
	errno=ENOENT;
	if(!drv)errno=ENOENT;
	else if(!drv->drv_tickget)errno=ENOSYS;
	else return drv->drv_tickget();
	return -1;
}

int guidrv_set_brightness(int percent){
	errno=0;
	if(!drv)ERET(ENOENT);
	else if(!drv->drv_setbrightness)ERET(ENOSYS);
	else drv->drv_setbrightness(percent);
	return errno==0?0:-1;
}

int guidrv_get_brightness(){
	errno=0;
	if(!drv)ERET(ENOENT);
	else if(!drv->drv_getbrightness)ERET(ENOSYS);
	else return drv->drv_getbrightness();
}

bool guidrv_can_sleep(){
	if(!drv)return false;
	if(!drv->drv_cansleep)return true;
	return drv->drv_cansleep();
}

int guidrv_get_modes(int*cnt,struct display_mode**modes){
	errno=0;
	if(!cnt||!modes)ERET(EINVAL);
	else if(!drv)ERET(ENOENT);
	else if(!drv->drv_get_modes)ERET(ENOSYS);
	else return drv->drv_get_modes(cnt,modes);
}

int guidrv_register(){
	errno=0;
	if(!drv)ERET(ENOENT);
	if(!drv->drv_register)ERET(ENOSYS);
	return drv->drv_register();
}

static int guidrv_try_init(lv_coord_t*w,lv_coord_t*h,int*dpi){
	if(!drv->drv_getsize)return -1;
	const char*name=guidrv_getname();
	tlog_debug("try to init gui driver %s",name);
	if(guidrv_register()<0){
		telog_error("failed to start gui driver %s",name);
		return -1;
	}
	guidrv_getsize(w,h);
	guidrv_getdpi(dpi);
	if(*w<=0||*h<=0){
		tlog_error("failed to get screen size with gui driver %s",name);
		abort();
	}
	return 0;
}

struct gui_driver*guidrv_get_by_name(const char*name){
	if(!name)EPRET(EINVAL);
	struct gui_driver*d;
	for(int i=0;(d=gui_drvs[i]);i++)
		if(d->name[0]&&strcmp(d->name,name)==0)return d;
	EPRET(ENOENT);
}

int indrv_init(){
	char*n;
	struct input_driver*in;
	for(int x=0;(in=input_drvs[x]);x++){
		bool compatible=false;
		for(int y=0;(n=in->compatible[y]);y++)
			if(guidrv_isname(n))compatible=true;
		if(!compatible||!in->drv_register)continue;
		tlog_debug("try init input driver %s",in->name);
		if(in->drv_register()==0)
			list_obj_add_new(&indrvs,in);
	}
	return 0;
}

int guidrv_init(lv_coord_t*w,lv_coord_t*h,int*dpi){
	errno=0;
	if(drv){
		guidrv_getsize(w,h);
		guidrv_getdpi(dpi);
		return 0;
	}
	#ifndef ENABLE_UEFI
	if(
		(drv=guidrv_get_by_name(getenv("GUIDRV")))&&
		guidrv_try_init(w,h,dpi)==0
	)return 0;
	#endif
	if(
		(drv=guidrv_get_by_name(confd_get_string("gui.driver",NULL)))&&
		guidrv_try_init(w,h,dpi)==0
	)return 0;
	errno=0;
	for(int i=0;(drv=gui_drvs[i]);i++)
		if(guidrv_try_init(w,h,dpi)==0)return 0;
	tlog_warn("no available gui drivers found");
	drv=NULL;
	return -1;
}

void guidrv_exit(){
	list*l=list_first(indrvs);
	if(l)do{
		LIST_DATA_DECLARE(d,l,struct input_driver*);
		if(d&&d->drv_exit)d->drv_exit();
	}while((l=l->next));
	if(drv&&drv->drv_exit)drv->drv_exit();
}

void guidrv_set_driver(struct gui_driver*driver){
	drv=driver;
}

struct gui_driver*guidrv_get_driver(){
	return drv;
}
#endif
