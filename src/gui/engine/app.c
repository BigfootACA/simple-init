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
#include"render_internal.h"
#include"defines.h"
#include"assets.h"

#define LOAD_XSTR(key,member,expr) \
	if((x=mxmlFindElement(root,doc,#key,NULL,NULL,MXML_DESCEND))){\
		if(!(val=mxmlGetOpaque(x))||!*val)\
			EDONE(tlog_error("invalid value for "#key));\
                size_t len=strlen(val);\
		if(sizeof(reg.member)>sizeof(char*)&&(len>=sizeof(reg.member)-1||len<=0))\
			EDONE(tlog_error("value "#key" too long"));\
                expr;\
		strcpy(reg.member,val);\
	}
#define LOAD_ASTR(key,member)\
	LOAD_XSTR(key,member,\
		if(!(reg.member=malloc(len+1)))EDONE();\
		memset(reg.member,0,len+1);\
	)
#define LOAD_STR(key,member)\
	LOAD_XSTR(key,member,)
#define LOAD_BOOL(key,member)\
	if((x=mxmlFindElement(root,doc,#key,NULL,NULL,MXML_DESCEND))){\
		if(!(val=mxmlGetOpaque(x))||!*val)\
			EDONE(tlog_error("invalid value for "#key));\
		if(string_is_false(val))reg.member=false;\
		else if(string_is_true(val))reg.member=true;\
		else EDONE(tlog_error("unknown value %s for "#key,val));\
	}

bool xml_string_load_activity(const char*content){
	uint32_t cl=0;
	bool res=false;
	char*comp,*end;
	const char*val=NULL;
	mxml_node_t*doc,*root,*x;
	struct gui_register reg;
	memset(&reg,0,sizeof(struct gui_register));
	if(!(doc=mxmlLoadString(
		NULL,content,
		MXML_OPAQUE_CALLBACK
	)))EDONE(tlog_error("parse xml failed"));
	if(!(root=mxmlFindElement(
		doc,doc,
		"SimpleInitActivity",
		NULL,NULL,MXML_DESCEND
	)))EDONE(tlog_error("invalid xml activity config"));
	if(!(comp=(char*)mxmlElementGetAttr(
		root,"compatible"
	)))EDONE(tlog_error("missing compatibility level"));
	errno=0,cl=(uint32_t)strtol(comp,&end,0);
	if(*end||end==comp||errno!=0)
		EDONE(tlog_error("invalid compatibility level"));
	if(cl>ACTIVITY_COMPATIBLE_LEVEL)
		EDONE(tlog_error("incompatible new version xml"));
	if(cl<ACTIVITY_COMPATIBLE_LEVEL)
		tlog_warn("found an old version of xml, need to upgrade");
	LOAD_STR(Name,name);
	LOAD_STR(Title,title);
	LOAD_STR(Icon,icon);
	LOAD_BOOL(ShowApp,show_app);
	LOAD_BOOL(OpenFile,open_file);
	LOAD_BOOL(FullScreen,full_screen);
	LOAD_BOOL(AllowBack,back);
	LOAD_BOOL(AllowExclusive,allow_exclusive);
	LOAD_BOOL(MaskBackground,mask);
	LOAD_ASTR(LayoutFile,xml);
	if(!reg.name[0]||!reg.title[0]||!reg.xml)
		EDONE(tlog_error("incomplete activity xml"));
	if(guiact_add_register(&reg)!=0)
		EDONE(telog_warn("add gui register failed"));
	res=true;
	done:
	if(doc)mxmlDelete(doc);
	if(reg.xml)free(reg.xml);
	return res;
}

bool xml_sstring_load_activity(const char*content,size_t len){
	if(!content||len<=0)return false;
	char*str=malloc(len+1);
	if(!str)return false;
	memset(str,0,len+1);
	strncpy(str,content,len);
	bool res=xml_string_load_activity(str);
	free(str);
	return res;
}

bool xml_assets_file_load_activity(entry_file*file){
	if(!file||!file->content||file->length<=0)return false;
	return xml_sstring_load_activity(file->content,file->length);
}

bool xml_assets_load_all_activity(entry_dir*dir){
	size_t len;
	entry_dir*d;
	entry_file*f;
	bool res=true;
	if(!dir)return false;
	if(dir->subfiles)for(size_t s=0;(f=dir->subfiles[s]);s++){
		if(!f->content||f->length<=0)continue;
		if((len=strlen(f->info.name))<=4)continue;
		if(strcasecmp(f->info.name+len-4,".xml")!=0)continue;
		if(!xml_assets_file_load_activity(f))res=false;
	}
	if(dir->subdirs)for(size_t s=0;(d=dir->subdirs[s]);s++)
		if(!xml_assets_load_all_activity(d))res=false;
	return res;
}

bool xml_assets_dir_load_activity(entry_dir*dir,const char*path){
	return xml_assets_file_load_activity(get_assets_file(dir,path));
}

bool xml_assets_dir_load_all_activity(entry_dir*dir,const char*path){
	return xml_assets_load_all_activity(get_assets_dir(dir,path));
}

bool xml_rootfs_load_activity(const char*path){
	return xml_assets_dir_load_activity(&assets_rootfs,path);
}

bool xml_rootfs_load_all_activity(const char*path){
	return xml_assets_dir_load_all_activity(&assets_rootfs,path);
}

#endif
#endif
