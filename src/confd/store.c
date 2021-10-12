#define _GNU_SOURCE
#include<string.h>
#include<stdlib.h>
#include"confd_internal.h"
#include"logger.h"
#ifdef ENABLE_UEFI
static void*store_lock;
#define LOCK_INIT(lock) (void)lock
#define LOCK(lock) (void)lock
#define UNLOCK(lock) (void)lock
#else
#include<pthread.h>
static pthread_mutex_t store_lock;
#define LOCK_INIT(lock) pthread_mutex_init(&lock,NULL)
#define LOCK(lock) pthread_mutex_lock(&lock)
#define UNLOCK(lock) pthread_mutex_unlock(&lock)
#endif

static struct conf conf_store={.type=TYPE_KEY};

struct conf*conf_get_store(){return &conf_store;}

static struct conf*conf_get(struct conf*conf,const char*name){
	errno=0;
	if(!conf)return NULL;
	if(conf->type!=TYPE_KEY)EPRET(ENOTDIR);
	list*p=list_first(conf->keys);
	if(p)do{
		LIST_DATA_DECLARE(d,p,struct conf*);
		if(d&&strcmp(d->name,name)==0)return d;
	}while((p=p->next));
	EPRET(ENOENT);
}

static struct conf*conf_create(struct conf*conf,const char*name){
	errno=0;
	if(!conf)return NULL;
	if(conf->type!=TYPE_KEY)EPRET(ENOTDIR);
	list*p=list_first(conf->keys);
	if(p)do{
		LIST_DATA_DECLARE(d,p,struct conf*);
		if(d&&strcmp(d->name,name)==0)EPRET(EEXIST);
	}while((p=p->next));
	struct conf*n=malloc(sizeof(struct conf));
	if(!n)EPRET(ENOMEM);
	memset(n,0,sizeof(struct conf));
	strcpy(n->name,name);
	n->parent=conf;
	list_obj_add_new_notnull(&conf->keys,n);
	return n;
}

static struct conf*conf_lookup(const char*path,bool create,enum conf_type type){
	errno=0;
	struct conf*cur=&conf_store,*x;
	char xpath[PATH_MAX]={0},*p=xpath,*key=p;
	if(strcmp(path,"/")==0&&type==0)return &conf_store;
	else strncpy(xpath,path,PATH_MAX-1);
	if(!path[0]&&type==0)return &conf_store;
	LOCK(store_lock);
	if(p)do{
		if(*p!='.')continue;
		*p=0;
		if(!(x=conf_get(cur,key))){
			if(!create){
				UNLOCK(store_lock);
				EPRET(ENOENT);
			}
			if(!(x=conf_create(cur,key))){
				UNLOCK(store_lock);
				return NULL;
			}
			x->type=TYPE_KEY;
		}
		cur=x,key=++p;
	}while(*p++);
	if(!key[0]){
		UNLOCK(store_lock);
		EPRET(EINVAL);
	}
	if(!(x=conf_get(cur,key))){
		if(!create){
			UNLOCK(store_lock);
			EPRET(ENOENT);
		}
		if(!(x=conf_create(cur,key))){
			UNLOCK(store_lock);
			return NULL;
		}
		x->type=type;
	}
	UNLOCK(store_lock);
	if(x->type==0)EPRET(EBADMSG);
	if(type!=0&&type!=x->type)EPRET(ENOENT);
	return x;
}

enum conf_type conf_get_type(const char*path){
	struct conf*c=conf_lookup(path,false,0);
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

const char*conf_get_type_string(const char*path){
	enum conf_type t=conf_get_type(path);
	return (((int)t)<0)?NULL:conf_type2string(t);
}

const char**conf_ls(const char*path){
	struct conf*c=conf_lookup(path,false,0);
	if(!c)return NULL;
	if(c->type!=TYPE_KEY)EPRET(ENOTDIR);
	LOCK(store_lock);
	int i=list_count(c->keys),x=0;
	if(i<0)i=0;
	size_t s=sizeof(char*)*(i+1);
	const char**r=malloc(s);
	if(!r){
		UNLOCK(store_lock);
		EPRET(ENOMEM);
	}
	memset(r,0,s);
	list*p=list_first(c->keys);
	if(p)do{
		LIST_DATA_DECLARE(d,p,struct conf*);
		r[x++]=d->name;
	}while((p=p->next));
	UNLOCK(store_lock);
	return r;
}

static int conf_del_obj(struct conf*c){
	if(!c)return -1;
	if(!c->parent)ERET(EINVAL);
	LOCK(store_lock);
	list*p;
	if(c->type==TYPE_KEY){
		list*x;
		if((p=list_first(c->keys)))do{
			x=p->next;
			conf_del_obj(LIST_DATA(p,struct conf*));
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
	UNLOCK(store_lock);
	return 0;
}

int conf_del(const char*path){
	struct conf*c=conf_lookup(path,false,0);
	return c?conf_del_obj(c):-(errno);
}

#define FUNCTION_CONF_GET_SET(_tag,_type,_func) \
	int conf_set_##_func(const char*path,_type data){\
		struct conf*c=conf_lookup(path,true,TYPE_##_tag);\
		if(!c)return -errno;\
		LOCK(store_lock);\
		if(c->type!=TYPE_##_tag){\
			UNLOCK(store_lock);\
			ERET(EBADMSG);\
		}\
		VALUE_##_tag(c)=data;\
		UNLOCK(store_lock);\
		return 0;\
	}\
	_type conf_get_##_func(const char*path,_type def){\
		struct conf*c=conf_lookup(path,false,TYPE_##_tag);\
		return c?VALUE_##_tag(c):def;\
	}

FUNCTION_CONF_GET_SET(STRING,char*,string)
FUNCTION_CONF_GET_SET(INTEGER,int64_t,integer)
FUNCTION_CONF_GET_SET(BOOLEAN,bool,boolean)
