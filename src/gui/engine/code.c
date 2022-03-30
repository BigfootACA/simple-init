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

xml_render_code*render_new_scode(
	xml_render*render,
	const char*name,
	const char*code,
	size_t len
){
	xml_render_code*c;
	if(!render||!name||!*name)EPRET(EINVAL);
	if(!code||len<=0)EPRET(EINVAL);
	if(strlen(name)>=sizeof(c->id)-1){
		tlog_warn("name too long");
		EPRET(EINVAL);
	}
	if(list_search_one(
		render->codes,
		list_render_code_cmp,
		(void*)name
	)){
		tlog_warn("code snippet %s already exists",name);
		EPRET(EEXIST);
	}
	if(!(c=malloc(sizeof(xml_render_code))))EPRET(ENOMEM);
	memset(c,0,sizeof(xml_render_code));
	c->render=render;
	strncpy(c->id,name,sizeof(c->id)-1);
	if(!(c->code=malloc(len+1)))goto done;
	memset(c->code,0,len+1);
	strncpy(c->code,code,len);
	list_obj_add_new(&render->codes,c);
	return c;
	done:
	if(c)render_code_free(c);
	return NULL;
}

xml_render_code*render_new_code(
	xml_render*render,
	const char*name,
	const char*code
){
	if(!render)EPRET(EINVAL);
	if(!name||!*name)EPRET(EINVAL);
	if(!code||!*code)EPRET(EINVAL);
	return render_new_scode(
		render,name,
		code,strlen(code)
	);
}

xml_render_code*render_load_code(
	xml_render*render,
	mxml_node_t*node
){
	bool autorun=false;
	xml_render_code*code;
	const char*key,*value;
	const char*cont=NULL,*id=NULL;
	if(!render||!node)EPRET(EINVAL);
	if(!(key=mxmlGetElement(node)))EPRET(EINVAL);
	if(strcasecmp(key,"Code")!=0)EPRET(EINVAL);
	for(int i=0;i<mxmlElementGetAttrCount(node);i++){
		key=NULL,value=mxmlElementGetAttrByIndex(node,i,&key);
		if(!key||!*key||!value)continue;
		if(strcasecmp(key,"id")==0)id=value;
		else if(strcasecmp(key,"autorun")==0){
			if(string_is_false(value))autorun=false;
			else if(string_is_true(value))autorun=true;
			else tlog_warn("invalid boolean attribute: %s",value);
		}else tlog_warn("unknown attribute: %s",key);
	}
	if(!id||!*id){
		tlog_error("code snippet id not set");
		return NULL;
	}
	if(!(cont=mxmlGetOpaque(node))){
		tlog_error("code content not set");
		return NULL;
	}
	if(!(code=render_new_code(render,id,cont)))return NULL;
	if(autorun&&render_code_exec_run(code)!=LUA_OK){
		tlog_error("auto run code snippet %s failed",code->id);
		list_obj_del_data(&code->render->codes,code,NULL);
		render_code_free(code);
		return NULL;
	}
	return code;
}

int render_code_exec_run(xml_render_code*code){
	int r=0;
	if(!code)ERET(EINVAL);
	#ifdef ENABLE_LUA
	lua_settop(code->render->lua,0);
	if((r=luaL_loadstring(code->render->lua,code->code))!=LUA_OK)
		EDONE(tlog_error("load lua code %s failed",code->id));
	if((r=lua_pcall(code->render->lua,0,LUA_MULTRET,0))!=LUA_OK)
		EDONE(tlog_error("error while running lua code %s",code->id));
	done:
	if(r!=LUA_OK)xlua_show_error(code->render->lua,TAG);
	#else
	tlog_error("lua is disabled");
	r=-1;
	#endif
	return r;
}
#endif
#endif
