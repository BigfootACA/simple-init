/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_MXML
#define _GNU_SOURCE
#include<string.h>
#include"str.h"
#include"mxml.h"
#include"list.h"
#include"logger.h"
#include"confd_internal.h"
#define TAG "config"

static const char*type2tag(enum conf_type type){
	switch(type){
		case TYPE_KEY:    return "key";
		case TYPE_INTEGER:return "integer";
		case TYPE_STRING: return "string";
		case TYPE_BOOLEAN:return "boolean";
		default:          return NULL;
	}
}

static enum conf_type tag2type(const char*tag){
	if(!tag)return 0;
	if(strcasecmp(tag,"key")==0)return TYPE_KEY;
	if(strcasecmp(tag,"int")==0)return TYPE_INTEGER;
	if(strcasecmp(tag,"integer")==0)return TYPE_INTEGER;
	if(strcasecmp(tag,"str")==0)return TYPE_STRING;
	if(strcasecmp(tag,"string")==0)return TYPE_STRING;
	if(strcasecmp(tag,"bool")==0)return TYPE_BOOLEAN;
	if(strcasecmp(tag,"boolean")==0)return TYPE_BOOLEAN;
	return 0;
}

static void parse_number(const char*match,mxml_node_t*node,uint32_t*val,int base){
	char*end=NULL;
	const char*str=mxmlElementGetAttr(node,match);
	if(!str)return;
	errno=0;
	*val=(size_t)strtol(str,&end,base);
	if(*end||str==end||errno!=0){
		tlog_warn("invalid value for %s in xml",match);
		*val=0;
	}
}

static int include_xml(struct conf_file_hand*hand,mxml_node_t*node){
	char*val,*path;
	if(!hand||!node)return -1;
	if(!(val=(char*)mxmlGetOpaque(node)))
		val=(char*)mxmlElementGetAttr(node,"path");
	if(!(path=strdup(val)))return -1;
	trim(path);
	if(conf_include_file_depth(hand->file,path,hand->depth+1)<0)
		telog_warn("include \"%s\" failed",path);
	return 0;
}

static int load_xml(struct conf_file_hand*hand,mxml_node_t*node,const char*path){
	uid_t uid=0;
	gid_t gid=0;
	mode_t mode=0;
	enum conf_type type;
	char rpath[PATH_MAX];
	const char*title,*name,*val;
	if(!hand||!node||!path)return -1;
	if(mxmlGetType(node)!=MXML_ELEMENT)return -1;
	memset(rpath,0,sizeof(rpath));
	if(path[0])strncpy(rpath,path,sizeof(rpath)-1);
	if(!(title=mxmlGetElement(node)))return -1;
	if(strncmp(title,"!--",3)==0)return 0;
	if(strcasecmp(title,"include")==0)
		return include_xml(hand,node);
	if((type=tag2type(title))==0)
		return trlog_warn(-1,"unknown type %s",title);
	name=mxmlElementGetAttr(node,"name");
	parse_number("uid",node,(uint32_t*)&uid,10);
	parse_number("gid",node,(uint32_t*)&gid,10);
	parse_number("mode",node,(uint32_t*)&mode,8);
	if(!name||!*name){
		if(type!=TYPE_KEY)return trlog_warn(-1,"missing name");
		if(path[0])return trlog_warn(-1,"invalid key");
	}else{
		if(path[0])strlcat(rpath,".",sizeof(rpath)-1);
		strlcat(rpath,name,sizeof(rpath)-1);
	}
	val=mxmlGetOpaque(node);
	switch(type){
		case TYPE_KEY:
			for(mxml_node_t*x=node;x;x=mxmlWalkNext(x,node,MXML_DESCEND)){
				if(mxmlGetParent(x)!=node)continue;
				if(mxmlGetType(x)!=MXML_ELEMENT)continue;
				conf_add_key(rpath,uid,gid);
				if(mode!=0)conf_set_mod(rpath,mode,0,0);
				load_xml(hand,x,rpath);
			}
		break;
		case TYPE_STRING:{
			char*value=NULL;
			if(val)value=strdup(val);
			conf_set_string_inc(rpath,value,uid,gid,hand->include);
			if(mode!=0)conf_set_mod(rpath,mode,0,0);
		}break;
		case TYPE_INTEGER:{
			char*end=NULL;
			int64_t value;
			if(!val)tlog_warn("require value for integer");
			value=(int64_t)strtoll(val,&end,0);
			if(*end||val==end||errno!=0)
				return trlog_warn(-1,"invalid value for integer");
			conf_set_integer_inc(rpath,value,uid,gid,hand->include);
			if(mode!=0)conf_set_mod(rpath,mode,0,0);
		}break;
		case TYPE_BOOLEAN:{
			bool value;
			if(!val)tlog_warn("require value for boolean");
			if(string_is_false(val))value=false;
			else if(string_is_true(val))value=true;
			else return trlog_warn(-1,"invalid value for boolean");
			conf_set_boolean_inc(rpath,value,uid,gid,hand->include);
			if(mode!=0)conf_set_mod(rpath,mode,0,0);
		}break;
	}
	return 0;
}

static int conf_load(struct conf_file_hand*hand){
	int r=-1;
	mxml_node_t*root,*node;
	if(!(root=mxmlLoadString(NULL,hand->buff,MXML_OPAQUE_CALLBACK)))
		EDONE(tlog_warn("parse xml document failed"));
	if(!(node=mxmlFindElement(root,root,"key",NULL,NULL,MXML_DESCEND)))
		EDONE(tlog_warn("invalid xml config"));
	load_xml(hand,node,"");
	done:
	if(root)mxmlDelete(root);
	return r;
}

static bool print_attrs(struct conf*c,struct conf_file_hand*hand){
	char buff[512],*p;
	if(c->name[0]){
		if(!(p=xml_escape(c->name)))return false;
		memset(buff,0,sizeof(buff));
		snprintf(buff,sizeof(buff)-1," name=\"%s\"",p);
		hand->write(hand,buff,0);
		free(p);
	}
	memset(buff,0,sizeof(buff));
	snprintf(
		buff,sizeof(buff)-1,
		" uid=\"%d\" gid=\"%d\" mode=\"%04o\"",
		c->user,c->group,c->mode
	);
	hand->write(hand,buff,0);
	return true;
}

static bool print_start_tag(struct conf*c,struct conf_file_hand*hand){
	char*tag=(char*)type2tag(c->type);
	if(!tag)return false;
	hand->write(hand,"<",0);
	hand->write(hand,tag,0);
	if(!print_attrs(c,hand))return false;
	hand->write(hand,">",0);
	return true;
}

static bool print_end_tag(struct conf*c,struct conf_file_hand*hand){
	char*tag=(char*)type2tag(c->type);
	if(!tag)return false;
	hand->write(hand,"</",0);
	hand->write(hand,tag,0);
	hand->write(hand,">",0);
	return true;
}

static int save_xml(struct conf_file_hand*hand,struct conf*c,int depth){
	list*p;
	char buff[64];
	if(!hand||!c)return -1;
	if(c->include||!c->save)return 0;
	for(int i=0;i<depth;i++)hand->write(hand,"\t",0);
	if(!print_start_tag(c,hand))return -1;
	switch(c->type){
		case TYPE_KEY:
			hand->write(hand,"\n",0);
			if((p=list_first(c->keys)))do{
				LIST_DATA_DECLARE(l,p,struct conf*);
				save_xml(hand,l,depth+1);
			}while((p=p->next));
			for(int i=0;i<depth;i++)hand->write(hand,"\t",0);
		break;
		case TYPE_INTEGER:
			memset(buff,0,sizeof(buff));
			snprintf(buff,sizeof(buff)-1,"%lld",(long long int)VALUE_INTEGER(c));
			hand->write(hand,buff,0);
		break;
		case TYPE_STRING:
			if(VALUE_STRING(c)){
				char*x=xml_escape(VALUE_STRING(c));
				if(x){
					hand->write(hand,x,0);
					free(x);
				}
			}
		break;
		case TYPE_BOOLEAN:
			hand->write(hand,BOOL2STR(VALUE_BOOLEAN(c)),0);
		break;
		default:return -1;
	}
	print_end_tag(c,hand);
	hand->write(hand,"\n",0);
	return 0;
}

static int conf_save(struct conf_file_hand*hand){
	hand->write(hand,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n",0);
	save_xml(hand,conf_get_store(),0);
	return 0;
}

struct conf_file_hand conf_hand_xml={
	.ext=(char*[]){"xml",NULL},
	.load=conf_load,
	.save=conf_save,
};
#endif
