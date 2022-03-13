/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<sys/stat.h>
#include"confd_internal.h"
#include"logger.h"
#include"lock.h"
#define KEY_MODE 0755
#define VAL_MODE 0644
static mutex_t store_lock;

bool conf_store_changed=false;
static struct conf conf_store={
	.type=TYPE_KEY,
	.save=true,
	.user=0,
	.group=0,
	.mode=0777
};

struct conf*conf_get_store(){return &conf_store;}

static bool conf_cmp(list*l,void*d){
	LIST_DATA_DECLARE(x,l,struct conf*);
	return x&&d&&strcmp(x->name,(char*)d)==0;
}

static struct conf*conf_get(struct conf*conf,const char*name){
	errno=0;
	if(!conf)return NULL;
	if(conf->type!=TYPE_KEY)EPRET(ENOTDIR);
	list*l=list_search_one(conf->keys,conf_cmp,(char*)name);
	if(!l)EPRET(ENOENT);
	return LIST_DATA(l,struct conf*);
}

static bool check_perm_read(struct conf*conf,uid_t u,gid_t g){
	if(u==0&&g==0)return true;
	if(conf->user==u&&(conf->mode&S_IRUSR)!=0)return true;
	else if(conf->group==g&&(conf->mode&S_IRGRP)!=0)return true;
	else if((conf->mode&S_IROTH)!=0)return true;
	else return false;
}

static bool check_perm_write(struct conf*conf,uid_t u,gid_t g){
	if(u==0&&g==0)return true;
	if(conf->user==u&&(conf->mode&S_IWUSR)!=0)return true;
	else if(conf->group==g&&(conf->mode&S_IWGRP)!=0)return true;
	else if((conf->mode&S_IWOTH)!=0)return true;
	else return false;
}

static struct conf*conf_create(struct conf*conf,const char*name,uid_t u,gid_t g){
	errno=0;
	if(!conf)return NULL;
	if(conf->type!=TYPE_KEY)EPRET(ENOTDIR);
	if(!check_perm_read(conf,u,g))EPRET(EACCES);
	if(list_search_one(conf->keys,conf_cmp,(char*)name))EPRET(EEXIST);
	if(!check_perm_write(conf,u,g))EPRET(EACCES);
	struct conf*n=malloc(sizeof(struct conf));
	if(!n)EPRET(ENOMEM);
	memset(n,0,sizeof(struct conf));
	strcpy(n->name,name);
	n->parent=conf,n->save=conf->save,n->user=u,n->group=g;
	list_obj_add_new_notnull(&conf->keys,n);
	return n;
}

static struct conf*conf_lookup(const char*path,bool create,enum conf_type type,uid_t u,gid_t g){
	errno=0;
	struct conf*cur=&conf_store,*x;
	char*xpath,*p,*key;
	if(!check_perm_read(&conf_store,u,g))EPRET(EACCES);
	if(strcmp(path,"/")==0&&type==0)return &conf_store;
	if(!path[0]&&type==0)return &conf_store;
	if(!(xpath=strdup(path)))EPRET(ENOMEM);
	p=xpath,key=xpath;
	MUTEX_LOCK(store_lock);
	if(p)do{
		if(*p!='.')continue;
		*p=0;
		if(!(x=conf_get(cur,key))){
			if(!create){
				MUTEX_UNLOCK(store_lock);
				free(xpath);
				EPRET(ENOENT);
			}
			if(!(x=conf_create(cur,key,u,g))){
				MUTEX_UNLOCK(store_lock);
				free(xpath);
				return NULL;
			}
			x->type=TYPE_KEY;
			x->mode=KEY_MODE;
		}
		if(!check_perm_read(x,u,g)){
			MUTEX_UNLOCK(store_lock);
			free(xpath);
			EPRET(EACCES);
		}
		cur=x,key=++p;
	}while(*p++);
	if(!key[0]){
		MUTEX_UNLOCK(store_lock);
		free(xpath);
		EPRET(EINVAL);
	}
	if(!(x=conf_get(cur,key))){
		if(!create){
			MUTEX_UNLOCK(store_lock);
			free(xpath);
			EPRET(ENOENT);
		}
		if(!(x=conf_create(cur,key,u,g))){
			MUTEX_UNLOCK(store_lock);
			free(xpath);
			return NULL;
		}
		x->type=type;
		x->mode=type==TYPE_KEY?KEY_MODE:VAL_MODE;
	}
	MUTEX_UNLOCK(store_lock);
	free(xpath);
	if(!check_perm_read(x,u,g))EPRET(EACCES);
	if(x->type==0)EPRET(EBADMSG);
	if(type!=0&&type!=x->type)EPRET(ENOENT);
	return x;
}

enum conf_type conf_get_type(const char*path,uid_t u,gid_t g){
	struct conf*c=conf_lookup(path,false,0,u,g);
	return c?c->type:(enum conf_type)-1;
}

const char*conf_type2string(enum conf_type type){
	switch(type){
		case TYPE_KEY:return "key";
		case TYPE_STRING:return "string";
		case TYPE_INTEGER:return "integer";
		case TYPE_BOOLEAN:return "boolean";
		default:return "unknown";
	}
}

const char*conf_get_type_string(const char*path,uid_t u,gid_t g){
	enum conf_type t=conf_get_type(path,u,g);
	return (((int)t)<0)?NULL:conf_type2string(t);
}

const char**conf_ls(const char*path,uid_t u,gid_t g){
	struct conf*c=conf_lookup(path,false,0,u,g);
	if(!c)return NULL;
	if(c->type!=TYPE_KEY)EPRET(ENOTDIR);
	MUTEX_LOCK(store_lock);
	int i=list_count(c->keys),x=0;
	if(i<0)i=0;
	size_t s=sizeof(char*)*(i+1);
	const char**r=malloc(s);
	if(!r){
		MUTEX_UNLOCK(store_lock);
		EPRET(ENOMEM);
	}
	memset(r,0,s);
	list*p=list_first(c->keys);
	if(p)do{
		LIST_DATA_DECLARE(d,p,struct conf*);
		r[x++]=d->name;
	}while((p=p->next));
	MUTEX_UNLOCK(store_lock);
	return r;
}

int conf_count(const char*path,uid_t u,gid_t g){
	struct conf*c=conf_lookup(path,false,0,u,g);
	if(!c)return -1;
	if(c->type!=TYPE_KEY)ERET(ENOTDIR);
	MUTEX_LOCK(store_lock);
	int i=list_count(c->keys);
	if(i<0)i=0;
	MUTEX_UNLOCK(store_lock);
	return i;
}

static int conf_del_obj(struct conf*c){
	if(!c)return -1;
	if(!c->parent)ERET(EINVAL);
	MUTEX_LOCK(store_lock);
	list*p;
	if(c->type==TYPE_KEY){
		list*x;
		if((p=list_first(c->keys)))do{
			x=p->next;
			MUTEX_UNLOCK(store_lock);
			conf_del_obj(LIST_DATA(p,struct conf*));
			MUTEX_LOCK(store_lock);
		}while((p=x));
		if(c->keys)free(c->keys);
		c->keys=NULL;
	}else if(c->type==TYPE_STRING&&c->value.string)free(c->value.string);
	if((p=list_first(c->parent->keys)))do{
		LIST_DATA_DECLARE(d,p,struct conf*);
		if(d!=c)continue;
		list_obj_del(&c->parent->keys,p,NULL);
		break;
	}while((p=p->next));
	free(c);
	conf_store_changed=true;
	MUTEX_UNLOCK(store_lock);
	return 0;
}

int conf_del(const char*path,uid_t u,gid_t g){
	struct conf*c=conf_lookup(path,false,0,u,g);
	return c?conf_del_obj(c):-(errno);
}

int conf_rename(const char*path,const char*name,uid_t u,gid_t g){
	struct conf*c=conf_lookup(path,false,0,u,g);
	if(!c)return -(errno);
	if(!name||!*name||strchr(name,'.'))ERET(EINVAL);
	if(strlen(name)>=sizeof(c->name)-1)ERET(EINVAL);
	if(!c->parent||!c->name[0])ERET(EACCES);
	if(strcmp(c->name,name)==0)return 0;
	if(!check_perm_read(c->parent,u,g))ERET(EACCES);
	if(list_search_one(c->parent->keys,conf_cmp,(char*)name))ERET(EEXIST);
	if(!check_perm_write(c,u,g))ERET(EACCES);
	memset(c->name,0,sizeof(c->name));
	strncpy(c->name,name,sizeof(c->name)-1);
	return 0;
}

int conf_add_key(const char*path,uid_t u,gid_t g){
	return conf_lookup(path,true,TYPE_KEY,u,g)!=NULL;
}

static int _conf_set_save(bool lock,struct conf*c,bool save,uid_t u,gid_t g){
	list*p;
	int r=0;
	if(!c)ERET(EINVAL);
	if(lock)MUTEX_LOCK(store_lock);
	if(c->type==TYPE_KEY&&(p=list_first(c->keys)))do{
		LIST_DATA_DECLARE(d,p,struct conf*);
		if(!check_perm_write(d,u,g))r=EPERM;
		else if(_conf_set_save(false,d,save,u,g)!=0)r=-errno;
	}while((p=p->next));
	if(!check_perm_write(c,u,g))r=EPERM;
	else c->save=save;
	if(lock)MUTEX_UNLOCK(store_lock);
	return r;
}

int conf_set_save(const char*path,bool save,uid_t u,gid_t g){
	return _conf_set_save(true,conf_lookup(path,false,0,u,g),save,u,g);
}

bool conf_get_save(const char*path,uid_t u,gid_t g){
	struct conf*c=conf_lookup(path,false,0,u,g);
	return c?c->save:false;
}

int conf_get_own(const char*path,uid_t*own,uid_t u,gid_t g){
	struct conf*c=conf_lookup(path,false,0,u,g);
	if(c&&own)*own=c->user;
	return c&&own;
}

int conf_get_grp(const char*path,gid_t*grp,uid_t u,gid_t g){
	struct conf*c=conf_lookup(path,false,0,u,g);
	if(c&&grp)*grp=c->group;
	return c&&grp;
}

int conf_get_mod(const char*path,mode_t*mod,uid_t u,gid_t g){
	struct conf*c=conf_lookup(path,false,0,u,g);
	if(c&&mod)*mod=c->mode;
	return c&&mod;
}

int conf_set_own(const char*path,uid_t own,uid_t u,gid_t g){
	if(u!=0&&g!=0)ERET(EPERM);
	struct conf*c=conf_lookup(path,false,0,u,g);
	if(c)c->user=own;
	return c!=NULL;
}

int conf_set_grp(const char*path,gid_t grp,uid_t u,gid_t g){
	if(u!=0&&g!=0)ERET(EPERM);
	struct conf*c=conf_lookup(path,false,0,u,g);
	if(c)c->group=grp;
	return c!=NULL;
}

int conf_set_mod(const char*path,mode_t mod,uid_t u,gid_t g){
	struct conf*c=conf_lookup(path,false,0,u,g);
	if(!c)return -1;
	if(u!=0&&u!=c->user)ERET(EPERM);
	c->mode=mod;
	return 0;
}

size_t conf_calc_size(struct conf*c){
	if(!c)return 0;
	list*p;
	size_t size=sizeof(struct conf);
	switch(c->type){
		case TYPE_KEY:
			if((p=list_first(c->keys)))do{
				LIST_DATA_DECLARE(l,p,struct conf*);
				size+=sizeof(list);
				size+=conf_calc_size(l);
			}while((p=p->next));
		break;
		case TYPE_STRING:
			if(c->value.string)size+=strlen(c->value.string)+1;
		break;
		default:;
	}
	return size;
}

#define FUNCTION_CONF_GET_SET(_tag,_type,_func) \
	int conf_set_##_func##_inc(const char*path,_type data,uid_t u,gid_t g,bool inc){\
		struct conf*c=conf_lookup(path,true,TYPE_##_tag,u,g);\
		if(!c)return -errno;\
		MUTEX_LOCK(store_lock);\
		if(c->type!=TYPE_##_tag){\
			MUTEX_UNLOCK(store_lock);\
			ERET(EBADMSG);\
		}\
                c->include=inc;\
		VALUE_##_tag(c)=data;\
		conf_store_changed=true;\
		MUTEX_UNLOCK(store_lock);\
		return 0;\
	}\
	int conf_set_##_func(const char*path,_type data,uid_t u,gid_t g){\
		return conf_set_##_func##_inc(path,data,u,g,false);\
	}\
	_type conf_get_##_func(const char*path,_type def,uid_t u,gid_t g){\
		struct conf*c=conf_lookup(path,false,TYPE_##_tag,u,g);\
		return c?VALUE_##_tag(c):def;\
	}

FUNCTION_CONF_GET_SET(STRING,char*,string)
FUNCTION_CONF_GET_SET(INTEGER,int64_t,integer)
FUNCTION_CONF_GET_SET(BOOLEAN,bool,boolean)
