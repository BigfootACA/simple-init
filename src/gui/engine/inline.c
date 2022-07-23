/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_MXML
#include<sys/stat.h>
#include"assets.h"
#include"render_internal.h"

void*render_get_user_data(xml_render*render){
	return render?render->data:NULL;
}

void render_set_user_data(xml_render*render,void*data){
	if(render)render->data=data;
}

const char*render_obj_get_id(xml_render_obj*obj){
	return obj?obj->id:NULL;
}

lv_obj_t*render_obj_get_object(xml_render_obj*obj){
	return obj?obj->obj:NULL;
}

xml_obj_type render_obj_get_type(xml_render_obj*obj){
	return obj?obj->type:OBJ_NONE;
}

void*render_obj_get_user_data(xml_render_obj*obj){
	return obj?obj->data:NULL;
}

void render_obj_set_user_data(xml_render_obj*obj,void*data){
	if(obj)obj->data=data;
}

const char*render_event_get_event_id(xml_render_event*event){
	return (event&&event->info)?event->info->event_id:NULL;
}

const char*render_event_get_obj_id(xml_render_event*event){
	return (event&&event->info&&event->info->obj)?event->info->obj->id:NULL;
}

lv_obj_t*render_event_get_lvgl_object(xml_render_event*event){
	return (event&&event->info&&event->info->obj)?event->info->obj->obj:NULL;
}

xml_render_obj*render_event_get_object(xml_render_event*event){
	return (event&&event->info)?event->info->obj:NULL;
}

lv_event_t*render_event_get_event(xml_render_event*event){
	return event?event->event:NULL;
}

void*render_event_get_initial_data(xml_render_event*event){
	return (event&&event->info)?event->info->data:NULL;
}

void*render_event_get_trigger_data(xml_render_event*event){
	return event?event->data:NULL;
}

bool render_set_content_string(
	xml_render*render,
	const char*content
){
	return render&&content?
		render_set_content_sstring(
			render,content,
			strlen(content)
		):false;
}

bool render_set_content_assets(
	xml_render*render,
	entry_file*file
){
	return render&&file?
		render_set_content_sstring(
			render,
			file->content,
			file->length
		):false;
}
#endif
#endif
