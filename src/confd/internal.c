/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<stdbool.h>
#include<string.h>
#include<errno.h>
#include"system.h"
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
	if(fd<0)ERET(EINVAL);
	static size_t xs=sizeof(struct confd_msg);
	return (int)full_write(fd,msg,xs);
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
	size_t size=sizeof(struct confd_msg),s,len=0;
	memset(buff,0,size);
	errno=0;
	do{
		errno=0;
		s=read(fd,buff+len,size-len);
		if(s==0)break;
		else if(s>0)len+=s;
		else switch(errno){
			case EINTR:continue;
			case EAGAIN:return 0;
			default:return -2;
		}
	}while(len<size);
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
		case CONF_GET_OWNER:   return "Get Owner";
		case CONF_GET_GROUP:   return "Get Group";
		case CONF_GET_MODE:    return "Get Mode";
		case CONF_GET_SAVE:    return "Get Can Save";
		case CONF_SET_STRING:  return "Set String";
		case CONF_SET_INTEGER: return "Set Integer";
		case CONF_SET_BOOLEAN: return "Set Boolean";
		case CONF_SET_OWNER:   return "Set Owner";
		case CONF_SET_GROUP:   return "Set Group";
		case CONF_SET_MODE:    return "Set Mode";
		case CONF_SET_SAVE:    return "Set Can Save";
		case CONF_SET_DEFAULT: return "Set Default Config";
		case CONF_ADD_KEY:     return "Add Key";
		case CONF_INCLUDE:     return "Include Config";
		case CONF_LOAD:        return "Load Config";
		case CONF_SAVE:        return "Save Config";
		case CONF_COUNT:       return "Count Values";
		default:               return "Unknown";
	}
}
