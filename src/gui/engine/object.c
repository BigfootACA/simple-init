/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_MXML
#include"render_internal.h"

static bool render_init_object(xml_render_obj*obj){
	const char*type;
	xml_obj_handle*hand;
	if(!obj->parent){
		strcpy(obj->id,"root");
		obj->obj=obj->render->root_obj;
		return true;
	}
	if(obj->obj){
		tlog_error("object already created");
		return false;
	}
	if(!(type=mxmlGetElement(obj->node))){
		tlog_error("cannot get object type");
		return false;
	}
	for(size_t s=0;(hand=&xml_obj_handles[s])->valid;s++){
		if(strcasecmp(hand->name,type)!=0)continue;
		obj->hand=hand,obj->type=hand->type;
		if(!hand->pre_hand(obj)){
			obj->obj=NULL,obj->type=OBJ_NONE;
			tlog_error("create object %s failed",type);
			return false;
		}
		if(!obj->obj)return false;
		lv_obj_set_user_data(obj->obj,obj);
		return true;
	}
	tlog_error("unsupported object type: %s",type);
	return false;
}

static int style_set_id(
	xml_render_style*style,
	const char*value
){
	if(!value[0])return trlog_error(
		-1,"invalid style id"
	);
	if(style->id[0])return trlog_error(
		-1,"style id %s already set",
		style->id
	);
	if(strlen(value)>=sizeof(style->id)-1)
		return trlog_error(-1,"style id too long");
	if(list_search_one(
		style->render->styles,
		list_render_style_cmp,
		(void*)value
	)||list_search_one(
		style->render->styles,
		list_render_style_class_cmp,
		(void*)value
	))return trlog_error(
		-1,"style %s already exists",
		value
	);
	strncpy(
		style->id,value,
		sizeof(style->id)-1
	);
	return 0;
}

static int style_set_class(
	xml_render_style*style,
	const char*value
){
	if(!value[0])return trlog_error(
		-1,"invalid style class"
	);
	if(style->class[0])return trlog_error(
		-1,"style class %s already set",
		style->class
	);
	if(strlen(value)>=sizeof(style->class)-1)
		return trlog_error(-1,"style class too long");
	if(list_search_one(
		style->render->styles,
		list_render_style_cmp,
		(void*)value
	))return trlog_error(
		-1,"style %s already exists",
		value
	);
	strncpy(
		style->class,value,
		sizeof(style->class)-1
	);
	return 0;
}

static bool render_parse_style(
	xml_render*render,
	mxml_node_t*node
){
	xml_render_style*style;
	const char*key,*value;
	if(!(style=render_style_new(render,node))){
		tlog_error("allocate style failed");
		return false;
	}
	for(int s=0;s<mxmlElementGetAttrCount(node);s++){
		key=NULL,value=mxmlElementGetAttrByIndex(node,s,&key);
		if(!key||!value)continue;
		if(strcasecmp(key,"id")==0){
			if(style_set_id(style,value)!=0)goto done;
			continue;
		}
		if(strcasecmp(key,"class")==0){
			if(style_set_class(style,value)!=0)goto done;
			continue;
		}
		if(!xml_style_apply_style(style,key,value)){
			tlog_error("add style %s failed",key);
			goto done;
		}
	}
	if(!style->id[0]){
		if(style->class[0]){
			size_t i=0;
			char name[sizeof(style->id)];
			do{
				memset(name,0,sizeof(name));
				snprintf(
					name,sizeof(name),
					"%s-%zu",style->class,i++
				);
			}while(list_search_one(
				style->render->styles,
				list_render_style_cmp,
				name
			));
			strncpy(style->id,name,sizeof(style->id)-1);
		}else EDONE(tlog_error("style id not set"));
	}
	return true;
	done:
	if(style){
		list_obj_del_data(&render->styles,style,NULL);
		render_style_free(style);
	}
	return false;
}

static bool render_parse_children(
	xml_render_obj*obj,
	mxml_node_t*node
);

static bool render_parse_if(
	xml_render*render,
	xml_render_obj*parent,
	mxml_node_t*node,
	bool is_else
){
	list*l;
	int r=0;
	static bool last=false;
	xml_render_code*code=NULL;
	const char*key,*value,*cond=NULL;
	bool use_last=is_else,reverse=is_else;
	bool found=false,result=false,ret=false;
	for(int s=0;s<mxmlElementGetAttrCount(node);s++){
		key=NULL,value=mxmlElementGetAttrByIndex(node,s,&key);
		if(!key)continue;
		if(strcasecmp(key,"condition")==0){
			if(found)goto conflict;
			if(is_else||!value)goto unknown;
			cond=value,found=true;
		}else if(strcasecmp(key,"code")==0){
			if(found)goto conflict;
			if(is_else||!value)goto unknown;
			if(!(l=list_search_one(
				render->codes,
				list_render_code_cmp,
				(void*)value
			)))EDONE(tlog_error("code snippet %s not found",value));
			code=LIST_DATA(l,xml_render_code*),found=true;
		}else if(strcasecmp(key,"reverse")==0){
			if(!value)reverse=true;
			else if(string_is_false(value))reverse=false;
			else if(string_is_true(value))reverse=true;
			else EDONE(tlog_error("invalid value %s for reverse",value));
		}else if(strcasecmp(key,"use-last")==0){
			if(!value)use_last=true;
			else if(string_is_false(value))use_last=false;
			else if(string_is_true(value))use_last=true;
			else EDONE(tlog_error("invalid value %s for use last",value));
		}else goto unknown;
	}
	if(use_last)ret=last;
	else{
		if(!found)EDONE(tlog_error("no any condition found"));
		#ifdef ENABLE_LUA
		if(code)r=render_code_exec_run(code);
		else{
			r=xlua_eval_string(render->lua,cond);
			if(r!=LUA_OK){
				tlog_error(
					"error while running lua condition '%s'",
					cond
				);
				xlua_show_error(render->lua,TAG);
			}
		}
		if(r==0){
			ret=lua_toboolean(render->lua,-1);
			lua_pop(render->lua,1);
		}
		#else
		EDONE(tlog_error("lua is disabled"));
		(void)cond;(void)code;(void)r;
		#endif
		last=ret;
	}
	if(reverse)ret=!ret;
	result=ret?render_parse_children(parent,node):true;
	done:
	return result;
	conflict:tlog_error("multiple condition specified in if");goto done;
	unknown:tlog_error("unknown attribute %s",key);goto done;
}

static bool render_parse_node(
	xml_render*render,
	xml_render_obj*parent,
	mxml_node_t*node
){
	const char*tag;
	if(!render||!node)
		return false;
	if(!(tag=mxmlGetElement(node))){
		tlog_error("invalid element");
		return false;
	}
	if(strncmp(tag,"!--",3)==0)
		return true;
	if(strcmp(tag,"Style")==0)
		return render_parse_style(render,node);
	if(strcmp(tag,"If")==0)
		return render_parse_if(render,parent,node,false);
	if(strcmp(tag,"Else")==0)
		return render_parse_if(render,parent,node,true);
	if(strcmp(tag,"Code")==0)
		return render_load_code(render,node)!=NULL;
	if(parent&&parent->render!=render){
		tlog_error("invalid parent");
		return false;
	}
	return render_parse_object(render,parent,node);
}

static bool render_parse_children(
	xml_render_obj*obj,
	mxml_node_t*node
){
	bool result=true;
	if(!obj||!obj->render||!node)return false;
	for(mxml_node_t*x=node;x;x=mxmlWalkNext(x,node,MXML_DESCEND)){
		if(mxmlGetParent(x)!=node)continue;
		if(mxmlGetType(x)!=MXML_ELEMENT)continue;
		if(!render_parse_node(obj->render,obj,x)){
			tlog_error(
				"parse sub object of %s failed",
				obj->id
			);
			result=false;
		}
	}
	return result;
}

bool render_parse_object(
	xml_render*render,
	xml_render_obj*parent,
	mxml_node_t*node
){
	list*l;
	bool result=true;
	xml_render_obj*obj;
	if(!(obj=render_obj_new(render,parent,node)))
		return false;
	if(!render_init_object(obj))goto done;
	if(!render_init_attributes(obj,false))goto done;
	if(!obj->id[0])EDONE(tlog_error("id does not set"));
	if((l=list_first(obj->styles)))do{
		LIST_DATA_DECLARE(style,l,xml_render_style*);
		lv_obj_add_style(obj->obj,&style->style,style->selector);
	}while((l=l->next));
	if(!render_parse_children(obj,node))
		result=false;
	if(
		obj->hand&&
		obj->hand->post_hand&&
		!obj->hand->post_hand(obj)
	)EDONE(tlog_error(
		"post init object %s failed",
		obj->id
	));
	return result;
	done:
	if(obj->obj&&!parent)lv_obj_del(obj->obj);
	if(obj){
		list_obj_del_data(&render->objects,obj,NULL);
		render_obj_free(obj);
	}
	return false;
}
#endif
#endif
