/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdlib.h>
#include<string.h>
#include<sys/un.h>
#include<sys/socket.h>
#include"logger.h"
#include"confd_internal.h"

int confd=-1;

int open_confd_socket(bool quiet,char*tag,char*path){
	if(confd>=0)close(confd);
	struct sockaddr_un n={0};
	n.sun_family=AF_UNIX;
	strncpy(n.sun_path,path,sizeof(n.sun_path)-1);
	if((confd=socket(AF_UNIX,SOCK_STREAM,0))<0)
		return quiet?-errno:erlog_error(-errno,tag,"cannot create socket");
	if(connect(confd,(struct sockaddr*)&n,sizeof(n))<0){
		if(!quiet)elog_error(tag,"cannot connect confd socket %s",n.sun_path);
		close(confd);
		confd=-1;
	}
	return confd;
}

int check_open_confd_socket(bool quiet,char*tag,char*path){
	return confd>=0?confd:open_confd_socket(quiet,tag,path);
}

int set_confd_socket(int fd){
	return confd=fd;
}

void close_confd_socket(){
	if(confd<0)return;
	close(confd);
	confd=-1;
}

static int confd_command(enum confd_action action,int code){
	struct confd_msg msg;
	confd_internal_send_code(confd,action,code);
	return confd_internal_read_msg(confd,&msg)&&msg.action==CONF_OK?0:msg.code;
}

int confd_quit(){return confd_command(CONF_QUIT,0);}

int confd_dump(){return confd_command(CONF_DUMP,0);}

int confd_delete(const char*path){
	if(!path||confd<0)ERET(EINVAL);
	errno=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_DELETE);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	if(confd_internal_send(confd,&msg)<0)return -1;
	if(confd_internal_read_msg(confd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	return res.code;
}

int confd_add_key(const char*path){
	if(!path||confd<0)ERET(EINVAL);
	errno=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_ADD_KEY);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	if(confd_internal_send(confd,&msg)<0)return -1;
	if(confd_internal_read_msg(confd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	return res.code;
}

int confd_set_integer(const char*path,int64_t data){
	if(!path||confd<0)ERET(EINVAL);
	errno=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_SET_INTEGER);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	msg.data.integer=data;
	if(confd_internal_send(confd,&msg)<0)return -1;
	if(confd_internal_read_msg(confd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	return res.code;
}

int confd_set_string(const char*path,char*data){
	if(!path||confd<0)ERET(EINVAL);
	errno=0;
	size_t size=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_SET_STRING);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	if(data)msg.data.data_len=size=strlen(data);
	if(confd_internal_send(confd,&msg)<0)return -1;
	if(data&&(size_t)write(confd,data,size)!=size)return -1;
	if(confd_internal_read_msg(confd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	return res.code;
}

int confd_set_boolean(const char*path,bool data){
	if(!path||confd<0)ERET(EINVAL);
	errno=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_SET_BOOLEAN);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	msg.data.boolean=data;
	if(confd_internal_send(confd,&msg)<0)return -1;
	if(confd_internal_read_msg(confd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	return res.code;
}

int64_t confd_count(const char*path){
	if(!path||confd<0)ERET(EINVAL);
	errno=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_COUNT);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	if(confd_internal_send(confd,&msg)<0)return -1;
	if(confd_internal_read_msg(confd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	return res.data.integer;
}

char**confd_ls(const char*path){
	if(!path)EPRET(EINVAL);
	if(confd<0)return NULL;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_LIST);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	if(confd_internal_send(confd,&msg)<0)return NULL;
	if(confd_internal_read_msg(confd,&res)<0)return NULL;
	size_t s=res.data.data_len;
	if(res.data.data_len==0)return NULL;
	size_t i=0,p=0;
	char*ret=malloc(s);
	if(!ret){
		lseek(confd,sizeof(i)+s,SEEK_CUR);
		EPRET(ENOMEM);
	}
	memset(ret,0,s);
	if(
		(size_t)read(confd,&i,sizeof(i))!=sizeof(i)||
		(size_t)read(confd,ret,s)!=s
	){
		free(ret);
		return NULL;
	}
	char**ls=malloc((i+1)*sizeof(char*));
	if(!ls){
		free(ret);
		EPRET(ENOMEM);
	}
	ls[p++]=ret;
	for(size_t x=0;x<s;x++)if(!ret[x])ls[p++]=ret+x+1;
	ls[p-1]=0;
	if(res.code>0)errno=res.code;
	return ls;
}

enum conf_type confd_get_type(const char*path){
	if(!path||confd<0)ERET(EINVAL);
	errno=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_GET_TYPE);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	if(confd_internal_send(confd,&msg)<0)return -1;
	if(confd_internal_read_msg(confd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	return res.data.type;
}

char*confd_get_string(const char*path,char*def){
	if(!path)EPRET(EINVAL);
	char*xdef=def?strdup(def):NULL;
	if(def&&!xdef)EPRET(ENOMEM);
	if(confd<0)return xdef;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_GET_STRING);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	if(confd_internal_send(confd,&msg)<0)return xdef;
	if(confd_internal_read_msg(confd,&res)<0)return xdef;
	size_t s=res.data.data_len;
	if(res.data.data_len==0)return xdef;
	char*ret=malloc(s+1);
	if(!ret){
		if(def)free(xdef);
		EPRET(ENOMEM);
	}
	memset(ret,0,s+1);
	if(s>0&&(size_t)read(confd,ret,s)!=s){
		free(ret);
		return xdef;
	}
	if(xdef)free(xdef);
	if(res.code>0)errno=res.code;
	return ret;
}

char*confd_get_sstring(const char*path,char*def,char*buf,size_t len){
	char*r=confd_get_string(path,def);
	memset(buf,0,len);
	if(r||def)strncpy(buf,r?r:def,len);
	return buf;
}

int64_t confd_get_integer(const char*path,int64_t def){
	if(!path)ERET(EINVAL);
	if(confd<0)return def;
	errno=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_GET_INTEGER);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	msg.data.integer=def;
	if(confd_internal_send(confd,&msg)<0)return -1;
	if(confd_internal_read_msg(confd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	return res.data.integer;
}

bool confd_get_boolean(const char*path,bool def){
	if(!path)ERET(EINVAL);
	if(confd<0)return def;
	errno=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_GET_BOOLEAN);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	msg.data.boolean=def;
	if(confd_internal_send(confd,&msg)<0)return -1;
	if(confd_internal_read_msg(confd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	return res.data.boolean;
}

static int _confd_file(const char*file,enum confd_action act){
	if(act==CONF_SET_DEFAULT&&!file)ERET(EINVAL);
	if(confd<0)ERET(ENOTCONN);
	errno=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,act);
	if(file)realpath(file,msg.path);
	if(confd_internal_send(confd,&msg)<0)return -1;
	if(confd_internal_read_msg(confd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	return res.code;
}

int confd_load_file(const char*file){
	return _confd_file(file,CONF_LOAD);
}

int confd_include_file(const char*file){
	return _confd_file(file,CONF_INCLUDE);
}

int confd_save_file(const char*file){
	return _confd_file(file,CONF_SAVE);
}

int confd_set_default_config(const char*file){
	return _confd_file(file,CONF_SET_DEFAULT);
}

int confd_set_save(const char*path,bool save){
	if(!path||confd<0)ERET(EINVAL);
	errno=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_SET_SAVE);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	msg.data.boolean=save;
	if(confd_internal_send(confd,&msg)<0)return -1;
	if(confd_internal_read_msg(confd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	return res.code;
}

bool confd_get_save(const char*path){
	if(!path||confd<0)ERET(EINVAL);
	errno=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_GET_SAVE);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	if(confd_internal_send(confd,&msg)<0)return -1;
	if(confd_internal_read_msg(confd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	return res.data.boolean;
}

int confd_get_own(const char*path,uid_t*own){
	if(!path||confd<0||!own)ERET(EINVAL);
	errno=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_GET_OWNER);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	if(confd_internal_send(confd,&msg)<0)return -1;
	if(confd_internal_read_msg(confd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	*own=res.data.uid;
	return res.code;
}

int confd_get_grp(const char*path,uid_t*grp){
	if(!path||confd<0||!grp)ERET(EINVAL);
	errno=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_GET_GROUP);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	if(confd_internal_send(confd,&msg)<0)return -1;
	if(confd_internal_read_msg(confd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	*grp=res.data.gid;
	return res.code;
}

int confd_get_mod(const char*path,mode_t*mod){
	if(!path||confd<0||!mod)ERET(EINVAL);
	errno=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_GET_MODE);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	if(confd_internal_send(confd,&msg)<0)return -1;
	if(confd_internal_read_msg(confd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	*mod=res.data.mode;
	return res.code;
}

int confd_set_own(const char*path,uid_t own){
	if(!path||confd<0||!own)ERET(EINVAL);
	errno=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_SET_OWNER);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	msg.data.uid=own;
	if(confd_internal_send(confd,&msg)<0)return -1;
	if(confd_internal_read_msg(confd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	return res.code;
}

int confd_set_grp(const char*path,uid_t grp){
	if(!path||confd<0||!grp)ERET(EINVAL);
	errno=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_SET_GROUP);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	msg.data.gid=grp;
	if(confd_internal_send(confd,&msg)<0)return -1;
	if(confd_internal_read_msg(confd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	return res.code;
}

int confd_set_mod(const char*path,mode_t mod){
	if(!path||confd<0||!mod)ERET(EINVAL);
	errno=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_SET_MODE);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	msg.data.mode=mod;
	if(confd_internal_send(confd,&msg)<0)return -1;
	if(confd_internal_read_msg(confd,&res)<0)return -1;
	if(res.code>0)errno=res.code;
	return res.code;
}

#define _EXT_BASE(ret,func,ret_func,...) \
ret func##_base(const char*base,const char*path __VA_ARGS__){\
	char xpath[PATH_MAX]={0};\
	snprintf(xpath,PATH_MAX-1,"%s.%s",base,path);\
	return ret_func;\
}
#define _EXT_DICT(ret,func,ret_func,...) \
ret func##_dict(const char*base,const char*key,const char*path __VA_ARGS__){\
	char xpath[PATH_MAX]={0};\
	snprintf(xpath,PATH_MAX-1,"%s.%s.%s",base,key,path);\
	return ret_func;\
}
#define _EXT_ARRAY(ret,func,ret_func,...) \
ret func##_array(const char*base,int index,const char*path __VA_ARGS__){\
	char xpath[PATH_MAX]={0};\
	snprintf(xpath,PATH_MAX-1,"%s.%d.%s",base,index,path);\
	return ret_func;\
}

#define EXT_BASE(func,arg,type,ret) _EXT_BASE(ret,func,func(xpath,arg),,type arg)
#define EXT_DICT(func,arg,type,ret) _EXT_DICT(ret,func,func(xpath,arg),,type arg)
#define EXT_ARRAY(func,arg,type,ret) _EXT_ARRAY(ret,func,func(xpath,arg),,type arg)
#define XEXT_BASE(func,ret) _EXT_BASE(ret,func,func(xpath),)
#define XEXT_DICT(func,ret) _EXT_DICT(ret,func,func(xpath),)
#define XEXT_ARRAY(func,ret) _EXT_ARRAY(ret,func,func(xpath),)

#define EXT(func,arg,type,ret) \
	EXT_BASE(func,arg,type,ret) \
	EXT_DICT(func,arg,type,ret) \
	EXT_ARRAY(func,arg,type,ret)
#define XEXT(func,ret) \
	XEXT_BASE(func,ret) \
	XEXT_DICT(func,ret) \
	XEXT_ARRAY(func,ret)

_EXT_BASE(char*,confd_get_sstring,confd_get_sstring(xpath,def,buf,len),,char*def,char*buf,size_t len)
_EXT_DICT(char*,confd_get_sstring,confd_get_sstring(xpath,def,buf,len),,char*def,char*buf,size_t len)
_EXT_ARRAY(char*,confd_get_sstring,confd_get_sstring(xpath,def,buf,len),,char*def,char*buf,size_t len)
EXT(confd_set_integer, data,int64_t,int);
EXT(confd_set_string,  data,char*,  int);
EXT(confd_set_boolean, data,bool,   int);
EXT(confd_get_string,  data,char*,  char*);
EXT(confd_get_integer, data,int64_t,int64_t);
EXT(confd_get_boolean, data,bool,   bool);
EXT(confd_set_save,    save,bool,   int);
EXT(confd_get_own,     own,uid_t*,  int);
EXT(confd_get_grp,     grp,gid_t*,  int);
EXT(confd_get_mod,     mod,mode_t*, int);
EXT(confd_set_own,     own,uid_t,   int);
EXT(confd_set_grp,     grp,gid_t,   int);
EXT(confd_set_mod,     mod,mode_t,  int);
XEXT(confd_get_save,   bool);
XEXT(confd_add_key,    int);
XEXT(confd_delete,     int);
XEXT(confd_count,      int64_t);
XEXT(confd_ls,         char**);
XEXT(confd_get_type,   enum conf_type);
