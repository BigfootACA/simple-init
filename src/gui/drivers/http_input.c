/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#ifdef ENABLE_GUI
#ifdef ENABLE_MICROHTTPD
#include"gui/guidrv.h"
#include"gui_http.h"

static bool parse_input_json(const json_object*jo){
	json_object*val;
	if(!(val=json_object_object_get(jo,"type")))
		return false;
	const char*type=json_object_get_string(val);
	lv_indev_data_t data;
	memset(&data,0,sizeof(data));
	if((val=json_object_object_get(jo,"pressed")))
		data.state=json_object_get_boolean(val)?
			LV_INDEV_STATE_PRESSED:
			LV_INDEV_STATE_RELEASED;
	if((val=json_object_object_get(jo,"x"))){
		lv_coord_t x=json_object_get_int(val);
		data.point.x=MAX(0,MIN(gui_w-1,x));
	}
	if((val=json_object_object_get(jo,"y"))){
		lv_coord_t y=json_object_get_int(val);
		data.point.y=MAX(0,MIN(gui_h-1,y));
	}
	if((val=json_object_object_get(jo,"key")))
		data.key=json_object_get_int(val);
	if((val=json_object_object_get(jo,"btn")))
		data.btn_id=json_object_get_int(val);
	if((val=json_object_object_get(jo,"enc")))
		data.enc_diff=json_object_get_int(val);
	if(strcasecmp(type,"pointer")==0)
		memcpy(&state.ptr_data,&data,sizeof(data));
	else if(strcasecmp(type,"keypad")==0)
		memcpy(&state.kbd_data,&data,sizeof(data));
	else if(strcasecmp(type,"encoder")==0)
		memcpy(&state.enc_data,&data,sizeof(data));
	else return false;
	sem_wait(&state.input_wait);
	return true;
}

static bool parse_input(const char*json){
	bool ret=false;
	json_object*jo=NULL;
	if((jo=json_tokener_parse(json))){
		ret=parse_input_json(jo);
		json_object_put(jo);
	}
	return ret;
}

int gui_http_recv_input_json(
	struct http_hand_websocket_data*d __attribute__((unused)),
	struct ws_data_hand*hand __attribute__((unused)),
	const json_object*jo
){
	parse_input_json(jo);
	return 0;
}

enum MHD_Result gui_http_hand_input_trigger(struct http_hand_info*i){
	int ret=0;
	struct post_data*d=*i->con_cls;
	if(!d){
		size_t s=sizeof(struct post_data);
		if(!(d=malloc(s)))return MHD_NO;
		memset(d,0,s);
		*i->con_cls=d;
		return MHD_YES;
	}
	if(
		*i->data_size!=0&&i->data&&
		strcmp(i->method,MHD_HTTP_METHOD_POST)==0
	){
		size_t ns=d->size+(*i->data_size);
		if(d->data){
			char*data=realloc(d->data,ns+1);
			if(!data)goto done;
			d->data=data;
		}else if(!(d->data=malloc(ns+1)))goto done;
		memcpy(d->data+d->size,i->data,*i->data_size);
		d->size=ns,*i->data_size=0;
		d->data[d->size]=0;
		return MHD_YES;
	}
	ret=MHD_HTTP_BAD_REQUEST;
	if(!parse_input(d->data))goto done;
	ret=MHD_HTTP_OK;
	done:
	*i->data_size=0,i->con_cls=NULL;
	if(d->data)free(d->data);
	if(d)free(d);
	return ret>0?http_ret_code(i,ret):MHD_NO;
}

static void http_input_read(lv_indev_drv_t*drv,lv_indev_data_t*data){
	lv_indev_data_t*src=NULL;
	switch(drv->type){
		case LV_INDEV_TYPE_KEYPAD:
			src=&state.kbd_data;
			if(!lv_group_get_editing(gui_grp))switch(src->key){
				case LV_KEY_UP:case LV_KEY_LEFT:
					src->key=LV_KEY_PREV;
				break;
				case LV_KEY_DOWN:case LV_KEY_RIGHT:
					src->key=LV_KEY_NEXT;
				break;
			}
			break;
		case LV_INDEV_TYPE_ENCODER:src=&state.enc_data;break;
		case LV_INDEV_TYPE_POINTER:src=&state.ptr_data;break;
		default:break;
	}
	if(src)memcpy(
		data,src,
		sizeof(lv_indev_data_t)
	);
	sem_post(&state.input_wait);
}

static int http_input_init(){
	static lv_indev_drv_t kbd_in;
	static lv_indev_drv_t ptr_in;
	static lv_indev_drv_t enc_in;
	lv_indev_drv_init(&kbd_in);
	lv_indev_drv_init(&ptr_in);
	lv_indev_drv_init(&enc_in);
	kbd_in.type=LV_INDEV_TYPE_KEYPAD;
	ptr_in.type=LV_INDEV_TYPE_POINTER;
	enc_in.type=LV_INDEV_TYPE_ENCODER;
	kbd_in.read_cb=http_input_read;
	ptr_in.read_cb=http_input_read;
	enc_in.read_cb=http_input_read;
	lv_indev_set_group(lv_indev_drv_register(&kbd_in),gui_grp);
	lv_indev_set_group(lv_indev_drv_register(&ptr_in),gui_grp);
	lv_indev_set_group(lv_indev_drv_register(&enc_in),gui_grp);
	return 0;
}

struct input_driver indrv_http={
	.name="http-input",
	.compatible={
		"http",
		NULL
	},
	.drv_register=http_input_init,
};

#endif
#endif