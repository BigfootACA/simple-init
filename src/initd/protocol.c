#define _GNU_SOURCE
#include<stdbool.h>
#include<sys/socket.h>
#include <string.h>
#include"system.h"
#include"logger.h"
#include"defines.h"
#include"init_internal.h"
#define TAG "init"

bool init_check_msg(struct init_msg*msg){
	return msg&&
		msg->magic0==INITD_MAGIC0&&
		msg->magic1==INITD_MAGIC1;
}

void init_initialize_msg(struct init_msg*msg,enum init_action act){
	if(!msg)return;
	memset(msg,0,sizeof(struct init_msg));
	msg->magic0=INITD_MAGIC0;
	msg->magic1=INITD_MAGIC1;
	msg->action=act;
}

bool init_check_privilege(enum init_action act,struct ucred*cred){
	if(!cred||cred->pid<=0)return false;
	if(cred->uid!=0||cred->gid!=0){
		char s[BUFSIZ];
		tlog_debug(
			"unprivileged init control request %s from %s",
			action2string(act),
			ucred2string(cred,s,BUFSIZ,true)
		);
		return false;
	}
	return true;
}

int init_process_data(int cfd,struct ucred*u,struct init_msg*msg){
	char s[BUFSIZ];
	struct init_msg res;
	init_initialize_msg(&res,ACTION_OK);
	if(!init_check_msg(msg))ERET(EINVAL);
	memset(&actiondata,0,sizeof(actiondata));
	memcpy(&actiondata,&msg->data,sizeof(union action_data));
		tlog_debug(
			"receive %s request from %s",
			action2string(msg->action),
			ucred2string(u,s,BUFSIZ,true)
		);
	if(init_check_privilege(msg->action,u))switch(msg->action){
		case ACTION_POWEROFF:case ACTION_HALT:case ACTION_REBOOT:
			action=msg->action,status=INIT_SHUTDOWN;
		break;
		case ACTION_SWITCHROOT:{
			#define init (msg->data.newroot.init)
			#define root (msg->data.newroot.root)
			if(root[0]==0){
				res.data.ret=errno=EINVAL;
				break;
			}
			char*realinit=init;
			if(root[sizeof(root)-1]!=0)
				tlog_warn("stack overflow detected on 'root'");
			if(root[sizeof(init)-1]!=0)
				tlog_warn("stack overflow detected on 'init'");
			if(!is_folder(root))res.data.ret=errno;
			else action=msg->action,status=INIT_SHUTDOWN;
			if(realinit[0]==0)realinit=NULL;
			if(!search_init(realinit,root))telog_error("check newroot init");
		}break;
		case ACTION_NONE:case ACTION_OK:case ACTION_FAIL:break;
		default:res.action=ACTION_FAIL,res.data.ret=ENOSYS;
	}
	init_send_data(cfd,&res);
	return 0;
}

const char*action2string(enum init_action act){
	switch(act){
		case ACTION_POWEROFF:return "Power Off";
		case ACTION_HALT:return "Halt";
		case ACTION_REBOOT:return "Reboot";
		case ACTION_SWITCHROOT:return "Switch Root";
		default:return "Unknown";
	}
}

int init_send_data(int fd,struct init_msg*send){
	ssize_t s;
	if(fd<0||!init_check_msg(send))ERET(EINVAL);
	while(true){
		s=write(fd,send,sizeof(struct init_msg));
		if(s<0)switch(errno){
			case EINTR:continue;
			default:return -1;
		}else break;
	}
	if(s!=sizeof(struct init_msg))ERET(EIO);
	return 0;
}

int init_recv_data(int fd,struct init_msg*response){
	ssize_t s;
	if(!response||fd<0)ERET(EINVAL);
	while(true){
		memset(response,0,sizeof(struct init_msg));
		s=read(fd,response,sizeof(struct init_msg));
		if(s<0)switch(errno){
			case EINTR:continue;
			default:return -1;
		}else break;
	}
	if(s!=sizeof(struct init_msg))ERET(EIO);
	if(!init_check_msg(response))ERET(EPROTO);
	return 0;
}