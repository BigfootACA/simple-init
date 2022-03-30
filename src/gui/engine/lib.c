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
#include"assets.h"
#include"defines.h"
#include"render_internal.h"

lv_coord_t render_resolve_coord(
	xml_render_obj*obj,
	lv_coord_t parent,
	const char*name
){
	size_t len=0;
	bool percent=false;
	char*val,*end=NULL;
	lv_coord_t result=0;
	if(!obj||!name)return 0;
	if(!(val=strdup(name)))goto done;
	if((len=strlen(val))<=0)goto done;
	if(val[len-1]=='%')percent=true,val[len-1]=0;
	if(val[0]=='?'){
		if(percent||len<=1)
			EDONE(tlog_error("invalid expression: %s",name));
		#ifdef ENABLE_LUA
		if(xlua_eval_string(obj->render->lua,val+1)!=LUA_OK){
			tlog_error("error while running lua expression %s",name);
			xlua_show_error(obj->render->lua,TAG);
		}
		result=lua_tointeger(obj->render->lua,-1);
		lua_pop(obj->render->lua,1);
		#else
		EDONE(tlog_error("lua is disabled"));
		#endif
	}else{
		if(strcmp(val,"0")!=0){
			errno=0;
			result=(lv_coord_t)strtol(val,&end,0);
			if(*end||val==end||errno!=0){
				tlog_error("invalid value: %s",name);
				result=0;
				goto done;
			}
		}
		if(percent)result=result*parent/100;
	}
	done:
	if(val)free(val);
	return result;
}

xml_render_obj*render_lookup_object(
	xml_render*render,
	const char*name
){
	list*l;
	if(!render||!name||!name[0])return false;
	MUTEX_LOCK(render->lock);
	l=list_search_one(
		render->objects,
		list_render_obj_cmp,
		(void*)name
	);
	MUTEX_UNLOCK(render->lock);
	return l?LIST_DATA(l,xml_render_obj*):NULL;
}

xml_render_style*render_lookup_style(
	xml_render*render,
	const char*name
){
	list*l;
	if(!render||!name||!name[0])return false;
	MUTEX_LOCK(render->lock);
	l=list_search_one(
		render->styles,
		list_render_style_cmp,
		(void*)name
	);
	MUTEX_UNLOCK(render->lock);
	return l?LIST_DATA(l,xml_render_style*):NULL;
}

xml_render_code*render_lookup_code(
	xml_render*render,
	const char*name
){
	list*l;
	if(!render||!name||!name[0])return false;
	MUTEX_LOCK(render->lock);
	l=list_search_one(
		render->codes,
		list_render_code_cmp,
		(void*)name
	);
	MUTEX_UNLOCK(render->lock);
	return l?LIST_DATA(l,xml_render_code*):NULL;
}

bool render_set_content_sstring(
	xml_render*render,
	const char*content,
	size_t len
){
	if(!render||!content||len<=0)return false;
	MUTEX_LOCK(render->lock);
	render->length=len;
	if(!(render->content=malloc(len+1))){
		tlog_error("allocate for file content failed");
		MUTEX_UNLOCK(render->lock);
		return false;
	}
	memset(render->content,0,len+1);
	memcpy(render->content,content,len);
	MUTEX_UNLOCK(render->lock);
	return true;
}

bool render_set_content_rootfs(
	xml_render*render,
	const char*path
){
	char rpath[PATH_MAX];
	entry_file*file=NULL;
	if(!render||!path)return false;
	memset(rpath,0,sizeof(rpath));
	if(path[0]!='/')strncpy(
		rpath,
		XML_ASSET_ROOT,
		sizeof(rpath)-1
	);
	strlcat(rpath,path,sizeof(rpath)-1);
	if(!(file=rootfs_get_assets_file((char*)rpath))){
		tlog_error("file %s not found in rootfs",path);
		return false;
	}
	return render_set_content_assets(render,file);
}
#endif
#endif
