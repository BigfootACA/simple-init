#include<stdbool.h>
#include<string.h>
#include<errno.h>
#include"defines.h"
#include"confd_internal.h"

bool confd_internal_check_magic(struct confd_msg*msg){
	return msg&&msg->magic0==CONFD_MAGIC0&&msg->magic1==CONFD_MAGIC1;
}

void confd_internal_init_msg(struct confd_msg*msg,enum confd_action action){
	if(!msg)return;
	memset(msg,0,sizeof(struct confd_msg));
	msg->magic0=CONFD_MAGIC0;
	msg->magic1=CONFD_MAGIC1;
	msg->action=action;
}

int confd_internal_send(int fd,struct confd_msg*msg){
	static size_t xs=sizeof(struct confd_msg);
	return ((size_t)write(fd,msg,xs))==xs?(int)xs:-1;
}

int confd_internal_send_code(int fd,enum confd_action action,int code){
	struct confd_msg msg;
	if(fd<0)ERET(EINVAL);
	confd_internal_init_msg(&msg,action);
	msg.code=code;
	return confd_internal_send(fd,&msg);
}

int confd_internal_read_msg(int fd,struct confd_msg*buff){
	if(!buff||fd<0)ERET(EINVAL);
	size_t size=sizeof(struct confd_msg),s;
	memset(buff,0,size);
	errno=0;
	while(1){
		errno=0;
		s=read(fd,buff,size);
		if(errno==0)break;
		switch(errno){
			case EINTR:continue;
			case EAGAIN:return 0;
			default:return -2;
		}
	}
	if(s==0)return EOF;
	return (s!=size||!(confd_internal_check_magic(buff)))?-2:1;
}

const char*confd_action2name(enum confd_action action){
	switch(action){
		case CONF_OK:          return "OK";
		case CONF_FAIL:        return "Failed";
		case CONF_QUIT:        return "Quit";
		case CONF_DELETE:      return "Delete";
		case CONF_DUMP:        return "Dump";
		case CONF_LIST:        return "List";
		case CONF_GET_TYPE:    return "Get Type";
		case CONF_GET_STRING:  return "Get String";
		case CONF_GET_INTEGER: return "Get Integer";
		case CONF_GET_BOOLEAN: return "Get Boolean";
		case CONF_SET_STRING:  return "Set String";
		case CONF_SET_INTEGER: return "Set Integer";
		case CONF_SET_BOOLEAN: return "Set Boolean";
		default:               return "Unknown";
	}
}
