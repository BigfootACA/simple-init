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
#include<termios.h>
#define TAG "input"
#include"str.h"
#include"gui.h"
#include"array.h"
#include"logger.h"
#include"gui/guidrv.h"

static bool initialized=false;
static struct termios oldtios;

static void stdin_read(lv_indev_drv_t*indev_drv,lv_indev_data_t*data){
	ssize_t r;
	char c=0,buff[8];
	bool edit=lv_group_get_editing(gui_grp);
	if(!initialized)return;
	data->continue_reading=false;
	data->state=LV_INDEV_STATE_RELEASED;
	r=read(STDIN_FILENO,&c,1);
	if(r<=0||r>1)return;
	switch(c){
		case '\e':{
			size_t s=sizeof(buff)-1;
			memset(buff,0,sizeof(buff));
			r=read(STDIN_FILENO,buff,s);
			if(r<0)return;
			if(buff[0]!='['||r==0)data->key=LV_KEY_ESC;
			else if(strcmp(buff,"[A")==0)data->key=edit?LV_KEY_UP:LV_KEY_PREV;
			else if(strcmp(buff,"[B")==0)data->key=edit?LV_KEY_DOWN:LV_KEY_NEXT;
			else if(strcmp(buff,"[C")==0)data->key=edit?LV_KEY_RIGHT:LV_KEY_PREV;
			else if(strcmp(buff,"[D")==0)data->key=edit?LV_KEY_LEFT:LV_KEY_NEXT;
			else if(strcmp(buff,"[F")==0)data->key=LV_KEY_END;
			else if(strcmp(buff,"[H")==0)data->key=LV_KEY_HOME;
			else if(strcmp(buff,"[3~")==0)data->key=LV_KEY_DEL;
			else if(strcmp(buff,"[5~")==0)data->key=LV_KEY_PREV;
			else if(strcmp(buff,"[6~")==0)data->key=LV_KEY_NEXT;
			else return;
		}break;
		case '\t':data->key=LV_KEY_NEXT;break;
		case '\b':case 0x7F:data->key=LV_KEY_BACKSPACE;break;
		case '\r':case '\n':data->key=LV_KEY_ENTER;break;
		default:data->key=c;
	}
	data->state=LV_INDEV_STATE_PRESSED;
	data->continue_reading=true;
}

static int stdin_init(){
	int f=0;
	struct termios tios;
	static lv_indev_drv_t indrv;
	static lv_indev_t*indev=NULL;
	if(indev)return -EBUSY;
	if(!isatty(STDIN_FILENO))return -ENOTTY;
	if((f=fcntl(STDIN_FILENO,F_GETFL))<0)
		return terlog_warn(-1,"fcntl F_GETFL failed");
	if(fcntl(STDIN_FILENO,F_SETFL,f|O_NONBLOCK)<0)
		return terlog_warn(-1,"fcntl F_SETFL failed");
	if(tcgetattr(STDIN_FILENO,&oldtios)<0)
		return terlog_warn(-1,"tcgetattr failed");
	memcpy(&tios,&oldtios,sizeof(tios));
	tios.c_lflag&=~(ICANON|ECHO);
	tios.c_cc[VMIN]=0;
	tios.c_cc[VTIME]=0;
	if(tcsetattr(STDIN_FILENO,TCSANOW,&tios)<0)
		return terlog_warn(-1,"tcsetattr failed");
	lv_indev_drv_init(&indrv);
	indrv.read_cb=stdin_read;
	indrv.type=LV_INDEV_TYPE_KEYPAD;
	indev=lv_indev_drv_register(&indrv);
	lv_indev_set_group(indev,gui_grp);
	initialized=true;
	return 0;
}

static void stdin_exit(){
	if(!initialized)return;
	tcsetattr(STDIN_FILENO,TCSANOW,&oldtios);
}

struct input_driver indrv_stdin={
	.name="stdin-input",
	.compatible={
		"drm",
		"fbdev",
		"sdl2",
		"vnc",
		"gtk",
		"dummy",
		"http",
		NULL
	},
	.drv_register=stdin_init,
	.drv_exit=stdin_exit,
};
#endif
