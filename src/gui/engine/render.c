/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_MXML
#include<stdlib.h>
#include"str.h"
#include"defines.h"
#include"render_internal.h"

bool render_resize(xml_render*render){
	list*l;
	bool result=true;
	if(!render)return false;
	MUTEX_LOCK(render->lock);
	if((l=list_first(render->objects)))do{
		LIST_DATA_DECLARE(d,l,xml_render_obj*);
		if(!d||!d->hand||!d->id[0])continue;
		if(!render_init_attributes(d,true))result=false;
		if(
			d->hand&&
			d->hand->post_hand&&
			!d->hand->post_hand(d)
		)result=false;
	}while((l=l->next));
	MUTEX_UNLOCK(render->lock);
	return result;
}

bool render_parse_doc(
	xml_render*render,
	xml_render_obj*parent,
	char*content
){
	xml_render_doc*d;
	char*end=NULL,*comp;
	if(!(d=malloc(sizeof(xml_render_doc))))return false;
	memset(d,0,sizeof(xml_render_doc));
	if(!(d->document=mxmlLoadString(
		NULL,content,
		MXML_OPAQUE_CALLBACK
	))){
		tlog_error("parse xml document failed");
		return false;
	}
	if(!(d->root_node=mxmlFindElement(
		d->document,d->document,
		"SimpleInitGUI",
		NULL,NULL,MXML_DESCEND
	))){
		tlog_error("invalid xml render config");
		return false;
	}
	if(!(comp=(char*)mxmlElementGetAttr(
		d->root_node,"compatible"
	))){
		tlog_error("missing compatibility level");
		return false;
	}
	errno=0,render->compatible_level=(uint32_t)strtol(comp,&end,0);
	if(*end||end==comp||errno!=0){
		tlog_error("invalid compatibility level");
		return false;
	}
	if(render->compatible_level>RENDER_COMPATIBLE_LEVEL){
		tlog_error("incompatible new version xml");
		return false;
	}
	if(render->compatible_level<RENDER_COMPATIBLE_LEVEL)
		tlog_warn("found an old version of xml, need to upgrade");
	if(!render_parse_object(render,parent,d->root_node)){
		tlog_error("error while render xml");
		return false;
	}
	list_obj_add_new(&render->docs,d);
	return true;
}

bool render_parse(xml_render*render,lv_obj_t*root){
	if(
		!render||!root||
		!render->content||
		render->root_obj
	)return false;
	MUTEX_LOCK(render->lock);
	render->root_obj=root;
	if(!render_parse_doc(render,NULL,render->content)){
		tlog_error("error while parse xml document");
		MUTEX_UNLOCK(render->lock);
		return false;
	}
	if(!render_move_callbacks(render)){
		tlog_error("error while initialize callbacks");
		MUTEX_UNLOCK(render->lock);
		return false;
	}
	render->initialized=true;
	MUTEX_UNLOCK(render->lock);
	return true;
}
#endif
#endif
