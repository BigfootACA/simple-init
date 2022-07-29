/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#define _GNU_SOURCE
#include<stdio.h>
#include<fcntl.h>
#include<errno.h>
#include<unistd.h>
#include<stddef.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<sys/epoll.h>
#include<sys/ioctl.h>
#include<linux/input.h>
#define TAG "input"
#include"str.h"
#include"gui.h"
#include"array.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/guidrv.h"
struct in_data{
	bool enabled,mouse;
	int fd,type;
	char path[64],name[256];
	lv_indev_drv_t indrv;
	lv_indev_t*indev;
	bool down;
	int16_t last_x,last_y;
	uint32_t key;
	char _pad[104];
	uint32_t xmax,ymax;
};
static bool mouse=false;
static struct epoll_event*evs;
static struct in_data*indatas[32];
static size_t
	es=sizeof(struct epoll_event),
	is=sizeof(struct input_event),
	ds=sizeof(struct in_data);
static pthread_t inp=0;
static int efd=-1;
static uint32_t keymap(uint16_t key){
	if(lv_group_get_editing(gui_grp))switch(key){
		case KEY_ENTER:
		case '\r':
		case KEY_POWER:return LV_KEY_ENTER;
		case KEY_UP:
		case KEY_PAGEUP:return LV_KEY_UP;
		case KEY_LEFT:
		case KEY_VOLUMEUP:return LV_KEY_LEFT;
		case KEY_DOWN:
		case KEY_PAGEDOWN:return LV_KEY_DOWN;
		case KEY_RIGHT:
		case KEY_VOLUMEDOWN:return LV_KEY_RIGHT;
		default:return key;
	}else switch(key){
		case KEY_ENTER:
		case '\r':
		case KEY_POWER:return LV_KEY_ENTER;
		case KEY_LEFT:
		case KEY_UP:
		case KEY_PAGEUP:
		case KEY_VOLUMEUP:return LV_KEY_PREV;
		case KEY_DOWN:
		case KEY_RIGHT:
		case KEY_PAGEDOWN:
		case KEY_TAB:
		case KEY_VOLUMEDOWN:return LV_KEY_NEXT;
		default:return key;
	}
}
static void*input_handler(void*args __attribute__((unused))){
	for(;;){
		int r=epoll_wait(efd,evs,64,-1);
		if(r<0){
			if(errno==EINTR)continue;
			telog_error("epoll failed");
			break;
		}else for(int i=0;i<r;i++){
			struct in_data*d=evs[i].data.ptr;
			struct input_event event;
			ssize_t c=read(d->fd,&event,is);
			if(c<=0){
				telog_warn("read %s failed",d->path);
				epoll_ctl(efd,EPOLL_CTL_DEL,d->fd,NULL);
				close(d->fd);
				d->enabled=false;
			}
			if(!gui_sleep)switch(d->indrv.type){
				case LV_INDEV_TYPE_POINTER:switch(event.type){
					case EV_REL:switch(event.code){
						case REL_X:
							d->last_x+=event.value;
							if(d->last_x>=gui_w)d->last_x=gui_w-1;
							if(d->last_x<0)d->last_x=0;
							mouse=true;
						break;
						case REL_Y:
							d->last_y+=event.value;
							if(d->last_y>=gui_h)d->last_y=gui_h-1;
							if(d->last_y<0)d->last_y=0;
							mouse=true;
						break;
					}
					break;
					case EV_ABS:switch(event.code){
						case ABS_X:d->last_x=gui_w*event.value/d->xmax,mouse=true;break;
						case ABS_Y:d->last_y=gui_h*event.value/d->ymax,mouse=true;break;
						case ABS_MT_POSITION_X:d->last_x=event.value,mouse=false;break;
						case ABS_MT_POSITION_Y:d->last_y=event.value,mouse=false;break;
					}break;
					case EV_KEY:switch(event.code){
						case BTN_TOUCH:mouse=false;//fallthrough
						case BTN_LEFT:d->down=event.value==1;break;
					}break;
				}break;
				case LV_INDEV_TYPE_KEYPAD:switch(event.type){
					case EV_KEY:
						d->down=event.value>0?LV_INDEV_STATE_PR:LV_INDEV_STATE_REL;
						d->key=keymap(event.code);
					break;
					case EV_SW:;break;
				}break;
				default:;
			}
			gui_quit_sleep();
		}
	}
	return NULL;
}
static void input_read(lv_indev_drv_t*indev_drv,lv_indev_data_t*data){
	struct in_data*d=indev_drv->user_data;
	if(!d->enabled||indev_drv->user_data!=d)return;
	switch(indev_drv->type){
		case LV_INDEV_TYPE_POINTER:
			data->point.x=lv_coord_border(d->last_x,gui_w-1,0);
			data->point.y=lv_coord_border(d->last_y,gui_h-1,0);
			data->state=d->down?LV_INDEV_STATE_PR:LV_INDEV_STATE_REL;
		break;
		case LV_INDEV_TYPE_KEYPAD:
			data->key=d->key;
			data->state=d->down?LV_INDEV_STATE_PR:LV_INDEV_STATE_REL;
		break;
		default:;
	}
	if(gui_cursor&&symbol_font&&indev_drv->type==LV_INDEV_TYPE_POINTER){
		static bool old=false;
		if(!d->indev->cursor){
			lv_indev_set_cursor(d->indev,gui_cursor);
			lv_obj_clear_flag(gui_cursor,LV_OBJ_FLAG_HIDDEN);
		}
		if(old!=mouse){
			old=mouse;
			if(mouse)lv_obj_clear_flag(gui_cursor,LV_OBJ_FLAG_HIDDEN);
			else lv_obj_add_flag(gui_cursor,LV_OBJ_FLAG_HIDDEN);
		}
	}
}
static int init_epoll(){
	if(efd>=0)return 0;
	if((efd=epoll_create(64))<0){
		telog_error("epoll_create failed");
		return -1;
	}
	if(!(evs=malloc(es*64))){
		telog_error("malloc failed");
		close(efd);
		return -1;
	}
	memset(evs,0,es*64);
	return 0;
}
static struct in_data*get_unused_in_data(){
	size_t x;
	struct in_data*d;
	for(x=0;x<ARRLEN(indatas);x++){
		d=indatas[x];
		if(!d){
			indatas[x]=d=malloc(ds);
			if(!d)telog_error("malloc failed");
		}else if(d->enabled)continue;
		memset(d,0,ds);
		return d;
	}
	telog_warn("too many input device open");
	return NULL;
}
static int input_init(char*dev,int fd){
	if(fd<0||!dev)return -1;
	bool support=false,abs=false;
	unsigned char mask[EV_MAX/8+1];
	struct in_data*d=get_unused_in_data();
	if(!d)return -1;
	if(init_epoll()<0)return -1;
	lv_indev_drv_init(&d->indrv);
	if(ioctl(fd,EVIOCGNAME(255),d->name)<0){
		telog_warn("failed to %s ioctl EIOCGNAME",dev);
		strcpy(d->name,"unknown");
	}
	if(ioctl(fd,EVIOCGBIT(0,sizeof(mask)),mask)<0)
		telog_warn("failed to %s ioctl EVIOCGBIT",dev);
	else for(int j=0;j<EV_MAX;j++)if(mask[j/8]&(1<<(j%8)))switch(j){
		case EV_ABS:abs=true;//fallthrough
		case EV_REL:
			support=true;
			d->indrv.type=LV_INDEV_TYPE_POINTER;
		break;
		case EV_SW:
		case EV_KEY:
			support=true;
			d->indrv.type=LV_INDEV_TYPE_KEYPAD;
		break;
	}
	if(!support)return -1;
	d->xmax=65536,d->ymax=65536;
	if(abs){
		struct input_absinfo info;
		memset(&info,0,sizeof(info));
		if(ioctl(fd,EVIOCGABS(ABS_X),&info)>=0&&info.maximum>0)
			d->xmax=info.maximum;
		memset(&info,0,sizeof(info));
		if(ioctl(fd,EVIOCGABS(ABS_Y),&info)>=0&&info.maximum>0)
			d->ymax=info.maximum;
	}
	tlog_debug("found input device %s (%s)",dev,d->name);
	d->indrv.read_cb=input_read;
	d->indrv.user_data=d;
	d->indev=lv_indev_drv_register(&d->indrv);
	lv_indev_set_group(d->indev,gui_grp);
	strncpy(d->path,dev,63);
	d->fd=fd;
	d->enabled=true;
	errno=0;
	epoll_ctl(efd,EPOLL_CTL_ADD,d->fd,&(struct epoll_event){.events=EPOLLIN,.data.ptr=d});
	if(inp!=0)return 0;
	tlog_info("starting input device thread");
	if(pthread_create(&inp,NULL,input_handler,NULL)!=0)telog_error("create thread failed");
	else pthread_setname_np(inp,"Input Device Thread");
	return 0;
}
static int input_scan_init(void){
	tlog_info("probing input devices");
	bool found=false;
	char path[32]={0};
	int fd;
	memset(indatas,0,sizeof(indatas));
	for(int i=0;i<32;i++){
		memset(path,0,32);
		snprintf(path,31,_PATH_DEV"/input/event%d",i);
		if((fd=open(path,O_RDONLY|O_CLOEXEC))<0){
			if(errno!=ENOENT)telog_warn("failed to open %s",path);
			continue;
		}
		if(input_init(path,fd)<0)close(fd);
		else found=true;
	}
	if(!found)tlog_error("no input devices found");
	return found?0:-1;
}
void input_register(char*dev){input_init(dev,open(dev,O_RDONLY|O_CLOEXEC));}
void input_scan_register(void){input_scan_init();}
struct input_driver indrv_event={
	.name="event-input",
	.compatible={
		"drm",
		"fbdev",
		NULL
	},
	.drv_register=input_scan_init,
};
#endif
