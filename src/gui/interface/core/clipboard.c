/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<time.h>
#include"gui.h"
#include"confd.h"
#include"logger.h"
#include"gui/clipboard.h"
#define TAG "clipboard"
#define BASE "gui.clipboard"
#define BASE_HIST BASE".history"

static enum clipboard_type clip_type=CLIP_NULL;
static char*clip_cont=NULL;

void clipboard_reset(void){
	confd_delete_base(BASE,"last");
	if(clip_cont)free(clip_cont);
	clip_type=CLIP_NULL,clip_cont=NULL;
	tlog_debug("reset clipboard");
}

void clipboard_clear(void){
	clipboard_reset();
	confd_delete(BASE);
}

void clipboard_load(const char*key){
	if(!key)return;
	clip_type=confd_get_integer_dict(BASE_HIST,key,"type",CLIP_NULL);
	clip_cont=confd_get_string_dict(BASE_HIST,key,"content",NULL);
	if(!clip_cont)clip_type=CLIP_NULL;
}

void clipboard_init(void){
	char*last=confd_get_string_base(BASE,"last",NULL);
	if(!last)return;
	clipboard_load(last);
	if(clip_type!=CLIP_NULL)tlog_debug(
		"restore clipboard type %d, content %s",
		clip_type,clip_cont
	);
	free(last);
}

int clipboard_set(enum clipboard_type type,const char*content,size_t len){
	if(type==CLIP_NULL){
		if(content)return -EINVAL;
		clipboard_reset();
		return 0;
	}
	if(!content)return -EINVAL;
	char*cont=len==0?strdup(content):strndup(content,len);
	if(!cont)return -ENOMEM;
	if(clip_cont){
		if(
			clip_type==type&&
			strcmp(clip_cont,cont)==0
		)return 0;
		free(clip_cont);
	}
	int c=1;
	char buf[255];
	time_t t;
	clip_type=type,clip_cont=cont;
	tlog_debug("set clipboard to type %d, content %s",type,cont);
	time(&t);
	for(;;){
		memset(buf,0,sizeof(buf));
		snprintf(buf,sizeof(buf)-1,"%ld%04d",(long int)t,c);
		if(confd_get_type_base(BASE_HIST,buf)!=TYPE_KEY)break;
		if(++c>=10000)return -EAGAIN;
	}
	confd_set_integer_dict(BASE_HIST,buf,"type",type);
	confd_set_string_dict(BASE_HIST,buf,"content",cont);
	confd_set_string_base(BASE,"last",buf);
	return 0;
}

enum clipboard_type clipboard_get_type(void){
	return clip_type;
}

char*clipboard_get_content(void){
	return clip_cont;
}

#endif
