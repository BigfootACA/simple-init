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

bool conf_store_changed=false;
static rwlock_t store_lock;

static struct conf conf_store={
	.type=TYPE_KEY,
	.save=true,
	.user=0,
	.group=0,
	.mode=0777
};

struct conf*conf_get_store(){
	RWLOCK_RDLOCK(store_lock);
	return &conf_store;
}

void conf_put_store(){
	RWLOCK_UNLOCK(store_lock);
}

static long conf_cmp(const struct rb_node*a,const struct rb_node*b){
	struct conf*confa = rb_to_conf(a);
	struct conf*confb = rb_to_conf(b);
	return strcmp(confa->name, confb->name);
}

static long conf_find(const struct rb_node*n,const void*d){
	struct conf*conf = rb_to_conf(n);
	return strcmp((char*)d,conf->name);
}

static struct conf*conf_get(struct conf*conf,const char*name){
	errno=0;
	if(!conf)return NULL;
	if(conf->type!=TYPE_KEY)EPRET(ENOTDIR);
	struct rb_node*n=rb_find(&conf->keys,(char*)name,conf_find);
	if(!n)EPRET(ENOENT);
	return rb_to_conf(n);
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
	if(rb_find(&conf->keys,(char*)name,conf_find))EPRET(EEXIST);
	if(!check_perm_write(conf,u,g))EPRET(EACCES);
	struct conf*n=malloc(sizeof(struct conf));
	if(!n)EPRET(ENOMEM);
	memset(n,0,sizeof(struct conf));
	strcpy(n->name,name);
	n->parent=conf,n->save=conf->save,n->user=u,n->group=g;
	rb_insert(&conf->keys,&n->node,conf_cmp);
	conf->count++;
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
	if(p)do{
		if(*p!='.')continue;
		*p=0;
		if(!(x=conf_get(cur,key))){
			if(!create){
				free(xpath);
				EPRET(ENOENT);
			}
			if(!(x=conf_create(cur,key,u,g))){
				free(xpath);
				return NULL;
			}
			x->type=TYPE_KEY;
			x->mode=KEY_MODE;
		}
		if(!check_perm_read(x,u,g)){
			free(xpath);
			EPRET(EACCES);
		}
		cur=x,key=++p;
	}while(*p++);
	if(!key[0]){
		free(xpath);
		EPRET(EINVAL);
	}
	if(!(x=conf_get(cur,key))){
		if(!create){
			free(xpath);
			EPRET(ENOENT);
		}
		if(!(x=conf_create(cur,key,u,g))){
			free(xpath);
			return NULL;
		}
		x->type=type;
		x->mode=type==TYPE_KEY?KEY_MODE:VAL_MODE;
	}
	free(xpath);
	if(!check_perm_read(x,u,g))EPRET(EACCES);
	if(x->type==0)EPRET(EBADMSG);
	if(type!=0&&type!=x->type)EPRET(ENOENT);
	return x;
}

static int conf_del_obj(struct conf*c) {
	if(!c)return -1;
	if(!c->parent)ERET(EINVAL);
	struct conf*d,*t;
	if(c->type==TYPE_KEY){
		rb_post_for_each_entry_safe(d,t,&c->keys,node) {
			conf_del_obj(d);
			free(d);
		}
	} else if(c->type==TYPE_STRING&&c->value.string)free(c->value.string);
	conf_store_changed=true;
	return 0;
}

static int _conf_set_save(struct conf*c,bool save,uid_t u,gid_t g){
	struct conf*d;
	int r=0;
	if(!c)ERET(EINVAL);
	if(c->type==TYPE_KEY)rb_for_each_entry(d,&c->keys,node){
		if(!check_perm_write(d,u,g))r=EPERM;
		else if(_conf_set_save(d,save,u,g)!=0)r=-errno;
	}
	if(!check_perm_write(c,u,g))r=EPERM;
	else c->save=save;
	return r;
}

enum conf_type conf_get_type(const char*path,uid_t u,gid_t g){
	RWLOCK_RDLOCK(store_lock);
	struct conf*c=conf_lookup(path,false,0,u,g);
	enum conf_type type=c?c->type:(enum conf_type)-1;
	RWLOCK_UNLOCK(store_lock);
	return type;
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
	RWLOCK_RDLOCK(store_lock);
	enum conf_type t=conf_get_type(path,u,g);
	const char* string=(((int)t)<0)?NULL:conf_type2string(t);
	RWLOCK_UNLOCK(store_lock);
	return string;
}

const char**conf_ls(const char*path,uid_t u,gid_t g){
	RWLOCK_RDLOCK(store_lock);
	struct conf*c=conf_lookup(path,false,0,u,g);
	if(!c){
		RWLOCK_UNLOCK(store_lock);
		return NULL;
	}
	if(c->type!=TYPE_KEY){
		RWLOCK_UNLOCK(store_lock);
		EPRET(ENOTDIR);
	}
	int i=c->count,x=0;
	if(i<0)i=0;
	size_t l,s=sizeof(char*)*(i+1);
	struct conf*d;
	rb_for_each_entry(d,&c->keys,node)
		s+=strlen(d->name)+1;
	const char**r=malloc(s);
	char*p;
	if(!r){
		RWLOCK_UNLOCK(store_lock);
		EPRET(ENOMEM);
	}
    p=(void *)(r+i+1);
	rb_for_each_entry(d,&c->keys,node){
        l=strlen(d->name)+1;
        memcpy(p,d->name,l);
        r[x++]=p;
        p+=l;
    }
    r[x]=NULL;
	RWLOCK_UNLOCK(store_lock);
	return r;
}

int conf_count(const char*path,uid_t u,gid_t g){
	RWLOCK_RDLOCK(store_lock);
	struct conf*c=conf_lookup(path,false,0,u,g);
	if(!c){
		RWLOCK_UNLOCK(store_lock);
		return -1;
	}
	if(c->type!=TYPE_KEY){
		RWLOCK_UNLOCK(store_lock);
		ERET(ENOTDIR);
	}
	int i=c->count;
	if(i<0)i=0;
	RWLOCK_UNLOCK(store_lock);
	return i;
}

int conf_del(const char*path,uid_t u,gid_t g){
	RWLOCK_WRLOCK(store_lock);
	struct conf*c=conf_lookup(path,false,0,u,g);
	if (!c){
		RWLOCK_UNLOCK(store_lock);
		return -(errno);
	}
	int i = conf_del_obj(c);
	if (i){
		RWLOCK_UNLOCK(store_lock);
		return i;
	}
	rb_delete(&c->parent->keys, &c->node);
	c->parent->count--;
	RWLOCK_UNLOCK(store_lock);
	free(c);
	return 0;
}

int conf_rename(const char*path,const char*name,uid_t u,gid_t g){
	RWLOCK_WRLOCK(store_lock);
	struct conf*c=conf_lookup(path,false,0,u,g);
	if(!c){
		RWLOCK_UNLOCK(store_lock);
		return -(errno);
	}
	if(!name||!*name||strchr(name,'.')){
		RWLOCK_UNLOCK(store_lock);
		ERET(EINVAL);
	}
	if(strlen(name)>=sizeof(c->name)-1){
		RWLOCK_UNLOCK(store_lock);
		ERET(EINVAL);
	}
	if(!c->parent||!c->name[0]){
		RWLOCK_UNLOCK(store_lock);
		ERET(EACCES);
	}
	if(strcmp(c->name,name)==0){
		RWLOCK_UNLOCK(store_lock);
		return 0;
	}
	if(!check_perm_read(c->parent,u,g)){
		RWLOCK_UNLOCK(store_lock);
		ERET(EACCES);
	}
	if(rb_find(&c->parent->keys,(char*)name,conf_find)){
		RWLOCK_UNLOCK(store_lock);
		ERET(EEXIST);
	}
	if(!check_perm_write(c,u,g)){
		RWLOCK_UNLOCK(store_lock);
		ERET(EACCES);
	}
	rb_delete(&c->parent->keys, &c->node);
	strncpy(c->name,name,sizeof(c->name)-1);
	rb_insert(&c->parent->keys, &c->node, conf_cmp);
	RWLOCK_UNLOCK(store_lock);
	return 0;
}

int conf_add_key(const char*path,uid_t u,gid_t g){
	RWLOCK_WRLOCK(store_lock);
	int ret=conf_lookup(path,true,TYPE_KEY,u,g)!=NULL;
	RWLOCK_UNLOCK(store_lock);
	return ret;
}

int conf_set_save(const char*path,bool save,uid_t u,gid_t g){
	RWLOCK_WRLOCK(store_lock);
	int ret=_conf_set_save(conf_lookup(path,false,0,u,g),save,u,g);
	RWLOCK_UNLOCK(store_lock);
	return ret;
}

bool conf_get_save(const char*path,uid_t u,gid_t g){
	RWLOCK_WRLOCK(store_lock);
	struct conf*c=conf_lookup(path,false,0,u,g);
	bool ret=c?c->save:false;
	RWLOCK_UNLOCK(store_lock);
	return ret;
}

int conf_get_own(const char*path,uid_t*own,uid_t u,gid_t g){
	RWLOCK_RDLOCK(store_lock);
	struct conf*c=conf_lookup(path,false,0,u,g);
	if(c&&own)*own=c->user;
	RWLOCK_UNLOCK(store_lock);
	return c&&own;
}

int conf_get_grp(const char*path,gid_t*grp,uid_t u,gid_t g){
	RWLOCK_RDLOCK(store_lock);
	struct conf*c=conf_lookup(path,false,0,u,g);
	if(c&&grp)*grp=c->group;
	RWLOCK_UNLOCK(store_lock);
	return c&&grp;
}

int conf_get_mod(const char*path,mode_t*mod,uid_t u,gid_t g){
	RWLOCK_RDLOCK(store_lock);
	struct conf*c=conf_lookup(path,false,0,u,g);
	if(c&&mod)*mod=c->mode;
	RWLOCK_UNLOCK(store_lock);
	return c&&mod;
}

int conf_set_own(const char*path,uid_t own,uid_t u,gid_t g){
	if(u!=0&&g!=0)ERET(EPERM);
	RWLOCK_WRLOCK(store_lock);
	struct conf*c=conf_lookup(path,false,0,u,g);
	if(c)c->user=own;
	RWLOCK_UNLOCK(store_lock);
	return c!=NULL;
}

int conf_set_grp(const char*path,gid_t grp,uid_t u,gid_t g){
	if(u!=0&&g!=0)ERET(EPERM);
	RWLOCK_WRLOCK(store_lock);
	struct conf*c=conf_lookup(path,false,0,u,g);
	if(c)c->group=grp;
	RWLOCK_UNLOCK(store_lock);
	return c!=NULL;
}

int conf_set_mod(const char*path,mode_t mod,uid_t u,gid_t g){
	RWLOCK_WRLOCK(store_lock);
	struct conf*c=conf_lookup(path,false,0,u,g);
	if(!c){
		RWLOCK_UNLOCK(store_lock);
		return -1;
	}
	if(u!=0&&u!=c->user){
		RWLOCK_UNLOCK(store_lock);
		ERET(EPERM);
	}
	c->mode=mod;
	RWLOCK_UNLOCK(store_lock);
	return 0;
}

size_t conf_calc_size(struct conf*c){
	if(!c)return 0;
	struct conf*l;
	size_t size=sizeof(struct conf);
	switch(c->type){
		case TYPE_KEY:
			rb_for_each_entry(l,&c->keys,node){
				size+=sizeof(struct rb_root);
				size+=conf_calc_size(l);
			}
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
		RWLOCK_WRLOCK(store_lock);\
		struct conf*c=conf_lookup(path,true,TYPE_##_tag,u,g);\
		if(!c)return -errno;\
		if(c->type!=TYPE_##_tag){\
			RWLOCK_UNLOCK(store_lock);\
			ERET(EBADMSG);\
		}\
		c->include=inc;\
		VALUE_##_tag(c)=data;\
		conf_store_changed=true;\
		RWLOCK_UNLOCK(store_lock);\
		return 0;\
	}\
	int conf_set_##_func(const char*path,_type data,uid_t u,gid_t g){\
		return conf_set_##_func##_inc(path,data,u,g,false);\
	}\
	_type conf_get_##_func(const char*path,_type def,uid_t u,gid_t g){\
		RWLOCK_RDLOCK(store_lock);\
		struct conf*c=conf_lookup(path,false,TYPE_##_tag,u,g);\
		_type ret=c?VALUE_##_tag(c):def;\
		RWLOCK_UNLOCK(store_lock);\
		return ret;\
	}

FUNCTION_CONF_GET_SET(STRING,char*,string)
FUNCTION_CONF_GET_SET(INTEGER,int64_t,integer)
FUNCTION_CONF_GET_SET(BOOLEAN,bool,boolean)
