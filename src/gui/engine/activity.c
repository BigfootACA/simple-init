/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_MXML
#include"str.h"
#include"defines.h"
#include"render_internal.h"

int render_activity_draw(struct gui_activity*act){
	if(!act||!act->data||!act->reg)ERET(EINVAL);
	xml_render*render=act->data;
	if(render->root_obj)return 0;
	if(!render->content){
		if(!act->reg->xml)return trlog_warn(
			ENUM(ENOENT),
			"xml path not set, abort"
		);
		if(strncasecmp(
			act->reg->xml,
			"<?xml",5
		)==0){
			if(!render_set_content_string(
				render,act->reg->xml
			))return trlog_warn(
				ENUM(ENOENT),
				"xml load failed, abort"
			);
		}
		if(!render_set_content_rootfs(
			render,act->reg->xml
		))return trlog_warn(
			ENUM(ENOENT),
			"xml load failed, abort"
		);
	}
	if(!render_parse(render,act->page))
		return trlog_warn(-1,"parse xml failed");
	return 0;
}

int render_activity_resize(struct gui_activity*act){
	if(!act||!act->data||!act->reg)ERET(EINVAL);
	xml_render*render=act->data;
	if(!render->root_obj)return 0;
	if(!render_resize(render))
		return trlog_warn(-1,"resize failed");
	return 0;
}

int render_activity_get_focus(struct gui_activity*act){
	list*l;
	if(!act||!act->data||!act->reg)ERET(EINVAL);
	xml_render*render=act->data;
	if(!render->root_obj)return 0;
	MUTEX_LOCK(render->lock);
	if((l=list_first(render->objects)))do{
		LIST_DATA_DECLARE(d,l,xml_render_obj*);
		if(
			!d||!d->hand||!d->id[0]||
			!lv_obj_has_flag(d->obj,LV_OBJ_FLAG_CLICKABLE)
		)continue;
		lv_group_add_obj(gui_grp,d->obj);
	}while((l=l->next));
	MUTEX_UNLOCK(render->lock);
	return 0;
}

int render_activity_lost_focus(struct gui_activity*act){
	list*l;
	if(!act||!act->data||!act->reg)ERET(EINVAL);
	xml_render*render=act->data;
	if(!render->root_obj)return 0;
	MUTEX_LOCK(render->lock);
	if((l=list_first(render->objects)))do{
		LIST_DATA_DECLARE(d,l,xml_render_obj*);
		if(
			!d||!d->hand||!d->id[0]||
			!lv_obj_has_flag(d->obj,LV_OBJ_FLAG_CLICKABLE)
		)continue;
		lv_group_remove_obj(d->obj);
	}while((l=l->next));
	MUTEX_UNLOCK(render->lock);
	return 0;
}

#endif
#endif
