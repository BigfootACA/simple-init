#include<stdlib.h>
#include<string.h>
#include<sys/un.h>
#include<sys/socket.h>
#include"logger.h"
#include"confd_internal.h"

int confd=-1;

int open_confd_socket(char*tag,char*path){
	if(confd>=0)close(confd);
	struct sockaddr_un n={0};
	n.sun_family=AF_UNIX;
	strncpy(n.sun_path,path,sizeof(n.sun_path)-1);
	if((confd=socket(AF_UNIX,SOCK_STREAM,0))<0)
		return erlog_error(-errno,tag,"cannot create socket");
	if(connect(confd,(struct sockaddr*)&n,sizeof(n))<0){
		elog_error(tag,"cannot connect confd socket %s",n.sun_path);
		close(confd);
		confd=-1;
	}
	return confd;
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
	if(confd<0)return def;
	size_t size=0;
	struct confd_msg msg,res;
	confd_internal_init_msg(&msg,CONF_GET_STRING);
	strncpy(msg.path,path,sizeof(msg.path)-1);
	if(def)msg.data.data_len=size=strlen(def);
	if(confd_internal_send(confd,&msg)<0)return def;
	if(def&&size>0&&(size_t)write(confd,def,size)!=size)return def;
	if(confd_internal_read_msg(confd,&res)<0)return def;
	size_t s=res.data.data_len;
	if(res.data.data_len==0)return def;
	char*ret=malloc(s+1);
	if(!ret)EPRET(ENOMEM);
	memset(ret,0,s+1);
	if(s>0&&(size_t)read(confd,ret,s)!=s){
		free(ret);
		return def;
	}
	if(res.code>0)errno=res.code;
	return ret;
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

#define EXT_BASE(func,arg,type,ret) \
ret func##_base(const char*base,const char*path,type arg){\
	char xpath[PATH_MAX]={0};\
	snprintf(xpath,PATH_MAX-1,"%s.%s",base,path);\
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
	EXT_ARRAY(func,arg,type,ret)
EXT(confd_set_integer, data,int64_t,int);
EXT(confd_set_string,  data,char*,  int);
EXT(confd_set_boolean, data,bool,   int);
EXT(confd_get_string,  data,char*,  char*);
EXT(confd_get_integer, data,int64_t,int64_t);
EXT(confd_get_boolean, data,bool,   bool);
