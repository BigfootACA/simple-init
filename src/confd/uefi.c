#define _GNU_SOURCE
#include<stdlib.h>
#include<string.h>
#include<Protocol/SimpleFileSystem.h>
#include"confd_internal.h"

int confd=-1;

int open_confd_socket(bool quiet __attribute__((unused)),char*tag __attribute__((unused)),char*path __attribute__((unused))){
	return -1;
}

int set_confd_socket(int fd __attribute__((unused))){
	return -1;
}

void close_confd_socket(){}

int confd_quit(){
	return -1;
}

int confd_dump(){
	conf_dump_store();
	return 0;
}

int confd_delete(const char*path){
	return conf_del(path);
}

int confd_set_integer(const char*path,int64_t data){
	return conf_set_integer(path,data);
}

int confd_set_string(const char*path,char*data){
	char*s=strdup(data);
	if(!s)ERET(ENOMEM);
	char*c=conf_get_string(path,NULL);
	if(c)free(c);
	return conf_set_string(path,s);
}

int confd_set_boolean(const char*path,bool data){
	return conf_set_boolean(path,data);
}

char**confd_ls(const char*path){
	const char**x=conf_ls(path);
	if(!x)return NULL;
	size_t as=0,ss=0,al,vl;
	for(as=0;x[as];as++)ss+=strlen(x[as])+1;
	al=sizeof(char*)*(as+1),vl=sizeof(char)*(ss+1);
	char**rx=malloc(al),*vx=malloc(vl);
	if(!rx||!vx){
		free(x);
		if(rx)free(rx);
		if(vx)free(vx);
		EPRET(ENOMEM);
	}
	memset(rx,0,al);
	memset(vx,0,vl);
	for(size_t i=0;i<as;i++){
		const char*v=x[i];
		strcpy(vx,v);
		rx[i]=vx,vx+=strlen(v)+1;
	}
	if(as==0)free(vx);
	return rx;
}

int confd_load_file(EFI_FILE_PROTOCOL*fd,const char*file){
	return conf_load_file(fd,file);
}

int confd_save_file(EFI_FILE_PROTOCOL*fd,const char*file){
	return conf_save_file(fd,file);
}

enum conf_type confd_get_type(const char*path){
	return conf_get_type(path);
}

char*confd_get_string(const char*path,char*def){
	char*x=conf_get_string(path,def);
	if(!x)x=def;
	return x?strdup(x):NULL;
}

int64_t confd_get_integer(const char*path,int64_t def){
	return conf_get_integer(path,def);
}

bool confd_get_boolean(const char*path,bool def){
	return conf_get_boolean(path,def);
}

#define EXT_BASE(func,arg,type,ret) \
ret func##_base(const char*base,const char*path,type arg){\
	char xpath[PATH_MAX]={0};\
	snprintf(xpath,PATH_MAX-1,"%s.%s",base,path);\
	return func(xpath,arg);\
}
#define EXT_DICT(func,arg,type,ret) \
ret func##_dict(const char*base,const char*key,const char*path,type arg){\
	char xpath[PATH_MAX]={0};\
	snprintf(xpath,PATH_MAX-1,"%s.%s.%s",base,key,path);\
	return func(xpath,arg);\
}
#define EXT_ARRAY(func,arg,type,ret) \
ret func##_array(const char*base,int index,const char*path,type arg){\
	char xpath[PATH_MAX]={0};\
	snprintf(xpath,PATH_MAX-1,"%s.%d.%s",base,index,path);\
	return func(xpath,arg);\
}
#define EXT(func,arg,type,ret) \
	EXT_BASE(func,arg,type,ret) \
	EXT_DICT(func,arg,type,ret) \
	EXT_ARRAY(func,arg,type,ret)
EXT(confd_set_integer, data,int64_t,int);
EXT(confd_set_string,  data,char*,  int);
EXT(confd_set_boolean, data,bool,   int);
EXT(confd_get_string,  data,char*,  char*);
EXT(confd_get_integer, data,int64_t,int64_t);
EXT(confd_get_boolean, data,bool,   bool);
