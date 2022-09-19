/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<mxml.h>
#include"str.h"
#include"confd.h"
#include"logger.h"
#include"gui/image.h"
#include"gui/icon_theme.h"
#define TAG "theme"
list*gui_icon_themes=NULL;

static bool fill_string_field(
	mxml_node_t*root,
	const char*path,
	char*dest,
	size_t len
){
	const char*val;
	mxml_node_t*node;
	if(!(node=mxmlFindElement(
		root,root,path,NULL,
		NULL,MXML_DESCEND_FIRST
	)))return false;
	if(!(val=mxmlGetOpaque(node)))return false;
	strncpy(dest,val,len-1);
	trim(dest);
	return dest[0]!=0;
}

static void fill_search_path(mxml_node_t*node,icon_theme*theme){
	int r=0;
	const char*val,*id,*path;
	icon_theme_search_path search;
	if(!(val=mxmlGetElement(node)))return;
	if(strcmp(val,"Path")!=0)return;
	id=mxmlElementGetAttr(node,"id");
	path=mxmlElementGetAttr(node,"path");
	if(!path)return;
	memset(&search,0,sizeof(search));
	if(!(search.path=strdup(path)))goto done;
	if(id&&!(search.id=strdup(id)))goto done;
	r=fs_open(theme->root,&search.folder,path,FILE_FLAG_FOLDER);
	if(r!=0)EDONE(tlog_warn("open %s failed: %s",path,strerror(r)));
	r=list_obj_add_new_dup(&theme->search_path,&search,sizeof(search));
	if(r==0)return;
	done:
	if(search.folder)fs_close(&search.folder);
	if(search.path)free(search.path);
	if(search.id)free(search.id);
}

static void fill_name_mapping(mxml_node_t*node,icon_theme*theme){
	int r=0;
	const char*val,*name,*regex,*path,*type,*search,*x;
	icon_theme_name_mapping map;
	if(!(val=mxmlGetElement(node)))return;
	if(strcmp(val,"Icon")!=0)return;
	type=mxmlElementGetAttr(node,"type");
	name=mxmlElementGetAttr(node,"name");
	regex=mxmlElementGetAttr(node,"regex");
	path=mxmlElementGetAttr(node,"path");
	search=mxmlElementGetAttr(node,"searchpath");
	if(!path)return;
	if((!name&&!regex)||(name&&regex))return;
	memset(&map,0,sizeof(map));
	if(regex&&!regexp_comp(regex,REG_ICASE,NULL))goto done;
	if(name&&!(map.name=strdup(name)))goto done;
	if(!(map.path=strdup(path)))goto done;
	if(!type){
		if(!(x=strrchr(path,'.'))||strchr(path,'/'))goto done;
		if(!(map.type=strdup(x+1)))goto done;
	}else if(!(map.type=strdup(type)))goto done;
	if(search&&!(map.search=strdup(search)))goto done;
	if(!image_get_decoder(map.type))
		EDONE(tlog_warn("skip unsupported image %s",map.path));
	r=list_obj_add_new_dup(&theme->name_mapping,&map,sizeof(map));
	if(r==0)return;
	tlog_debug("add failed: %d",r);
	done:
	if(map.name)free(map.name);
	if(map.type)free(map.type);
	if(map.path)free(map.path);
	if(map.search)free(map.search);
}

static void fill_search_paths(mxml_node_t*root,icon_theme*theme){
	mxml_node_t*node=mxmlFindElement(root,root,"SearchPath",NULL,NULL,MXML_DESCEND_FIRST);
	for(mxml_node_t*x=node;x;x=mxmlWalkNext(x,node,MXML_DESCEND_FIRST)){
		if(mxmlGetParent(x)!=node)continue;
		if(mxmlGetType(x)!=MXML_ELEMENT)continue;
		fill_search_path(x,theme);
	}
}

static void fill_name_mappings(mxml_node_t*root,icon_theme*theme){
	mxml_node_t*node=mxmlFindElement(root,root,"NameMapping",NULL,NULL,MXML_DESCEND_FIRST);
	for(mxml_node_t*x=node;x;x=mxmlWalkNext(x,node,MXML_DESCEND_FIRST)){
		if(mxmlGetParent(x)!=node)continue;
		if(mxmlGetType(x)!=MXML_ELEMENT)continue;
		fill_name_mapping(x,theme);
	}
}

static bool fill_theme_info(mxml_node_t*root,icon_theme*theme){
	if(!fill_string_field(root,"Id",theme->id,sizeof(theme->id)))return false;
	if(!fill_string_field(root,"Version",theme->version,sizeof(theme->version)))return false;
	fill_string_field(root,"Name",theme->name,sizeof(theme->name));
	fill_string_field(root,"Icon",theme->icon,sizeof(theme->icon));
	fill_string_field(root,"Author",theme->author,sizeof(theme->author));
	fill_string_field(root,"Description",theme->desc,sizeof(theme->desc));
	if(!theme->author[0])strcpy(theme->name,"(unknown)");
	if(!theme->name[0])strcpy(theme->name,theme->id);
	if(!theme->desc[0])strcpy(theme->desc,theme->id);
	return true;
}

static bool load_theme_xml(char*buffer,icon_theme*theme){
	int level;
	char*val,*end=NULL;
	mxml_node_t*doc=NULL,*root;
	if(!(doc=mxmlLoadString(NULL,buffer,MXML_OPAQUE_CALLBACK)))goto inv;
	if(!(root=mxmlFindElement(doc,doc,"SimpleInitIconTheme",NULL,NULL,MXML_DESCEND)))goto inv;
	if(!(val=(char*)mxmlElementGetAttr(root,"compatible")))goto inv;
	errno=0,level=(uint32_t)strtol(val,&end,0);
	if(*end||end==val||errno!=0)EDONE(tlog_error("invalid compatibility level"));
	if(level>ICON_THEME_COMPATIBLE_LEVEL)EDONE(tlog_error("incompatible new version icon theme xml"));
	if(level<ICON_THEME_COMPATIBLE_LEVEL)tlog_warn("found an old version of icon theme xml, need to upgrade");
	if(!fill_theme_info(root,theme))goto inv;
	fill_search_paths(root,theme);
	fill_name_mappings(root,theme);
	mxmlDelete(doc);
	return true;
	done:
	if(theme)free(theme);
	if(buffer)free(buffer);
	if(doc)mxmlDelete(doc);
	return false;
	inv:EDONE(tlog_error("invalid icon theme xml"));
}

static void print_theme(icon_theme*theme){
	int cnt=list_count(theme->name_mapping);
	if(cnt<0)cnt=0;
	tlog_debug(
		"loaded icon theme%s %s v%s (%s) by %s have %d icons",
		theme->zip?" package":"",
		theme->name,theme->version,
		theme->id,theme->author,cnt
	);
}

static bool load_xml(icon_theme*theme,fsh*f){
	int r=0;
	bool ret=false;
	size_t size=0;
	char*buffer=NULL;
	r=fs_read_all(f,(void**)&buffer,&size);
	if(r!=0)EDONE(tlog_error("load icon theme xml failed: %s",strerror(r)));
	if(!buffer||size<=0)EDONE(tlog_error("invalid icon theme xml"));
	if(!load_theme_xml(buffer,theme))goto done;
	ret=true;
	done:
	if(buffer)free(buffer);
	if(f)fs_close(&f);
	return ret;
}

bool icon_theme_load_fsh(fsh*f){
	int r=0;
	url*u=NULL;
	fsh*nf=NULL;
	size_t size=0;
	char magic[4];
	static int z=0;
	icon_theme*theme=NULL;
	if(!fsh_check(f))return false;
	memset(magic,0,sizeof(magic));
	r=fs_read(f,magic,2,&size);
	if(r!=0||size!=2)EDONE(tlog_error("load icon theme failed: %s",strerror(r)));
	if(!(theme=malloc(sizeof(icon_theme))))goto done;
	memset(theme,0,sizeof(icon_theme));
	size=0;
	if(strcmp(magic,"PK")==0){
		#ifndef ENABLE_LIBZIP
		EDONE(tlog_error("zip support not enabled"));
		#else
		theme->zip=true;
		snprintf(theme->zid,sizeof(theme->zid)-1,"icon-theme-%d",z++);
		r=fs_register_zip(f,theme->zid);
		if(r!=0)EDONE(tlog_error("open icon theme package failed: %s",strerror(r)));
		if(!(u=url_new()))EDONE();
		url_set_scheme(u,"zip",0);
		url_set_host(u,theme->zid,0);
		url_set_path(u,"/",0);
		r=fs_open_uri(&theme->root,u,FILE_FLAG_FOLDER);
		url_free(u);
		if(r!=0)EDONE(tlog_error("open icon theme package root failed: %s",strerror(r)));
		r=fs_open(theme->root,&nf,"icon-theme.xml",FILE_FLAG_READ);
		if(r!=0)EDONE(tlog_error("open icon theme xml failed: %s",strerror(r)));
		#endif
	}else nf=f;
	if(!load_xml(theme,nf))goto done;
	print_theme(theme);
	list_obj_add_new(&gui_icon_themes,theme);
	return true;
	done:
	if(theme)free(theme);
	return false;
}

bool icon_theme_load_url(url*u){
	fsh*f=NULL;
	bool ret=false;
	if(fs_open_uri(&f,u,FILE_FLAG_READ)!=0)return false;
	ret=icon_theme_load_fsh(f);
	if(!ret)fs_close(&f);
	return ret;
}

bool icon_theme_load(const char*path){
	fsh*f=NULL;
	bool ret=false;
	if(fs_open(NULL,&f,path,FILE_FLAG_READ)!=0)return false;
	ret=icon_theme_load_fsh(f);
	if(!ret)fs_close(&f);
	return ret;
}

void icon_theme_load_from_confd(){
	char**l,*r,*path="gui.theme.icons";
	if((l=confd_ls(path))){
		for(size_t i=0;l[i];i++){
			if(!(r=confd_get_string_base(path,l[i],NULL)))continue;
			icon_theme_load(r);
			free(r);
		}
		if(l[0])free(l[0]);
		free(l);
	}
	icon_theme_load("rootfs:///usr/share/simple-init/icon.xml");
}
