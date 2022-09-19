/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>
#include<sys/un.h>
#include<sys/stat.h>
#include<sys/epoll.h>
#include<sys/prctl.h>
#include<sys/socket.h>
#include"list.h"
#include"getopt.h"
#include"logger.h"
#include"system.h"
#include"output.h"
#include"confd_internal.h"
#include"proctitle.h"
#define TAG "confd"

static pthread_t save_thread;
static char*def_path=NULL;
static char*sock=DEFAULT_CONFD;
static bool clean=false,protect=false;
static int efd=-1;

static void ctl_fd(int op,int fd){
	static struct epoll_event ev;
	ev.events=EPOLLIN,ev.data.fd=fd;
	epoll_ctl(efd,op,fd,&ev);
	if(op==EPOLL_CTL_DEL)close(fd);
}

static void confd_cleanup(int s __attribute__((unused))){
	if(clean)return;
	clean=true;
	unlink(sock);
}

static void signal_handler(int s,siginfo_t*info,void*c __attribute__((unused))){
	if(info->si_pid<=1&&protect)return;
	tlog_info("initconfd exiting");
	confd_cleanup(s);
	exit(0);
}

static void do_ls(int fd,struct confd_msg*msg,struct confd_msg*ret,struct ucred*cred){
	size_t s=0,i;
	char**ls=NULL,*l=NULL,*p=NULL;
	if(!(ls=(char**)conf_ls(msg->path,cred->uid,cred->pid)))goto fail;
	for(i=0;ls[i];i++)s+=strlen(ls[i])+1;
	if(s<=0||!(p=l=malloc(s)))goto fail;
	for(i=0;ls[i];i++){
		size_t c=strlen(ls[i]);
		strcpy(p,ls[i]);
		p+=c+1;
	}
	ret->data.data_len=s;
	confd_internal_send(fd,ret);
	write(fd,&i,sizeof(i));
	write(fd,l,s);
	free(ls);
	free(l);
	return;
	fail:
	if(ls)free(ls);
	if(l)free(l);
	ret->data.data_len=0;
	confd_internal_send(fd,ret);
}

static void do_get_string(int fd,struct confd_msg*msg,struct confd_msg*ret,struct ucred*cred){
	char*re=conf_get_string(msg->path,NULL,cred->uid,cred->gid);
	ret->data.data_len=re?strlen(re):0;
	confd_internal_send(fd,ret);
	if(re)full_write(fd,re,ret->data.data_len);
}

static int do_set_string(int fd,struct confd_msg*msg,struct ucred*cred){
	size_t s=msg->data.data_len,r;
	char*data=malloc(s+1);
	if(!data){
		if(s>0)lseek(fd,s,SEEK_CUR);
		return 0;
	}
	memset(data,0,s+1);
	do{errno=0;r=(size_t)read(fd,data,s);}while(errno==EAGAIN);
	if(r!=s){
		free(data);
		return 0;
	}
	char*old=conf_get_string(msg->path,NULL,cred->uid,cred->gid);
	if(old)free(old);
	int retdata=-conf_set_string(msg->path,data,cred->uid,cred->gid);
	if(retdata!=0)free(data);
	return retdata;
}

static int do_rename(int fd,struct confd_msg*msg,struct ucred*cred){
	char*n=strchr(msg->path,':');
	if(!n)ERET(EINVAL);
	*n++=0;
	if(!msg->path[0]||!*n)ERET(EINVAL);
	return conf_rename(msg->path,n,cred->uid,cred->gid);
}

struct async_load_save_data{
	int fd;
	char path[PATH_MAX];
	pthread_t tid;
	bool include;
};

static void*_async_load_thread(void*d){
	if(!d)return NULL;
	struct async_load_save_data*data=d;
	struct confd_msg ret;
	confd_internal_init_msg(&ret,CONF_OK);
	ret.code=-(data->include?
		conf_include_file(NULL,data->path[0]?data->path:def_path):
		conf_load_file(NULL,data->path[0]?data->path:def_path)
	);
	if(ret.code==0&&errno!=0)ret.code=errno;
	confd_internal_send(data->fd,&ret);
	free(data);
	return NULL;
}

static void*_async_save_thread(void*d){
	if(!d)return NULL;
	struct async_load_save_data*data=d;
	struct confd_msg ret;
	confd_internal_init_msg(&ret,CONF_OK);
	ret.code=-conf_save_file(NULL,data->path[0]?data->path:def_path);
	if(ret.code==0){
		if(errno!=0)ret.code=errno;
		else if(!data->path[0])conf_store_changed=false;
	}
	confd_internal_send(data->fd,&ret);
	free(data);
	return NULL;
}

static int do_async_load(int fd,const char*path,bool inc){
	if(!path)return -1;
	struct async_load_save_data*d=malloc(sizeof(struct async_load_save_data));
	if(!d)return -1;
	d->fd=fd;
	d->include=inc;
	strcpy(d->path,path);
	int r=pthread_create(&d->tid,NULL,_async_load_thread,d);
	if(r!=0)free(d);
	return r;
}

static int do_async_save(int fd,const char*path){
	if(!path)return -1;
	struct async_load_save_data*d=malloc(sizeof(struct async_load_save_data));
	if(!d)return -1;
	d->fd=fd;
	strcpy(d->path,path);
	int r=pthread_create(&d->tid,NULL,_async_save_thread,d);
	if(r!=0)free(d);
	return r;
}

static int confd_read(int fd){
	if(fd<0)ERET(EINVAL);
	errno=0;
	struct confd_msg msg;
	int e=confd_internal_read_msg(fd,&msg);
	if(e<0)return e;
	else if(e==0)return 0;
	struct confd_msg ret;
	confd_internal_init_msg(&ret,CONF_OK);
	int retdata=0;
	socklen_t len=sizeof(struct ucred);
	struct ucred cred;
	if(getsockopt(fd,SOL_SOCKET,SO_PEERCRED,&cred,&len)<0)goto fail;
	if(len!=sizeof(struct ucred)||cred.pid<=0){
		errno=EIO;
		goto fail;
	}
	switch(msg.action){
		// command response
		case CONF_OK:case CONF_FAIL:break;

		// terminate confd
		case CONF_QUIT:
			if(cred.uid!=0||cred.gid!=0)errno=EACCES;
			else{
				tlog_notice("receive exit request");
				e=-4;
			}
		break;

		// dump config store
		case CONF_DUMP:
			if(cred.uid!=0||cred.gid!=0)errno=EACCES;
			else conf_dump_store(msg.code==0?LEVEL_DEBUG:msg.code);
		break;

		// delete item
		case CONF_DELETE:
			conf_del(msg.path,cred.uid,cred.gid);
		break;

		// create config key
		case CONF_ADD_KEY:
			conf_add_key(msg.path,cred.uid,cred.gid);
		break;

		// rename config item
		case CONF_RENAME:
			retdata=-do_rename(fd,&msg,&cred);
		break;

		// set config should save
		case CONF_SET_SAVE:
			conf_set_save(msg.path,msg.data.boolean,cred.uid,cred.gid);
		break;

		// get config should save
		case CONF_GET_SAVE:
			ret.data.boolean=conf_get_save(msg.path,cred.uid,cred.gid);
		break;

		// set owner
		case CONF_SET_OWNER:
			conf_set_own(msg.path,msg.data.uid,cred.uid,cred.gid);
		break;

		// set group
		case CONF_SET_GROUP:
			conf_set_grp(msg.path,msg.data.gid,cred.uid,cred.gid);
		break;

		// set mode
		case CONF_SET_MODE:
			conf_set_mod(msg.path,msg.data.mode,cred.uid,cred.gid);
		break;

		// get owner
		case CONF_GET_OWNER:
			conf_get_own(msg.path,&ret.data.uid,cred.uid,cred.gid);
		break;

		// get group
		case CONF_GET_GROUP:
			conf_get_grp(msg.path,&ret.data.gid,cred.uid,cred.gid);
		break;

		// get mode
		case CONF_GET_MODE:
			conf_get_mod(msg.path,&ret.data.mode,cred.uid,cred.gid);
		break;

		// list items in key
		case CONF_LIST:
			do_ls(fd,&msg,&ret,&cred);
		return e;

		// get item type
		case CONF_GET_TYPE:
			ret.data.type=conf_get_type(msg.path,cred.uid,cred.gid);
		break;

		// get item as string
		case CONF_GET_STRING:
			do_get_string(fd,&msg,&ret,&cred);
		return e;

		// get item as integer
		case CONF_GET_INTEGER:
			ret.data.integer=conf_get_integer(msg.path,msg.data.integer,cred.uid,cred.gid);
		break;

		// get item as boolean
		case CONF_GET_BOOLEAN:
			ret.data.boolean=conf_get_boolean(msg.path,msg.data.boolean,cred.uid,cred.gid);
		break;

		// put item as string
		case CONF_SET_STRING:
			retdata=do_set_string(fd,&msg,&cred);
		break;

		// put item as integer
		case CONF_SET_INTEGER:
			retdata=-conf_set_integer(msg.path,msg.data.integer,cred.uid,cred.gid);
		break;

		// put item as boolean
		case CONF_SET_BOOLEAN:
			retdata=-conf_set_boolean(msg.path,msg.data.boolean,cred.uid,cred.gid);
		break;

		// get config item keys count
		case CONF_COUNT:
			ret.data.integer=conf_count(msg.path,cred.uid,cred.gid);
			if(ret.data.integer<0)retdata=-ret.data.integer,ret.data.integer=0;
		break;

		// set default config path
		case CONF_SET_DEFAULT:
			if(cred.uid!=0||cred.gid!=0){
				errno=EACCES;
				break;
			}
			if(!msg.path[0]){
				errno=EINVAL;
				break;
			}
			if(def_path)free(def_path);
			def_path=strdup(msg.path);
			if(!def_path)errno=ENOMEM;
		break;

		// load config
		case CONF_LOAD:
			if(cred.uid!=0||cred.gid!=0)errno=EACCES;
			else if(do_async_load(fd,msg.path,false)==0)return e;
			break;

		// load config
		case CONF_INCLUDE:
			if(cred.uid!=0||cred.gid!=0)errno=EACCES;
			else if(do_async_load(fd,msg.path,true)==0)return e;
		break;

		// save config
		case CONF_SAVE:
			if(cred.uid!=0||cred.gid!=0)errno=EACCES;
			else if(do_async_save(fd,msg.path)==0)return e;
		break;

		// unknown
		default:telog_warn(
			"action %s(0x%X) not implemented",
			confd_action2name(msg.action),msg.action
		);
	}
	fail:
	if(retdata==0&&errno!=0)retdata=errno;
	ret.code=retdata;
	confd_internal_send(fd,&ret);
	return e;
}

static int listen_confd_socket(){
	int fd,er;
	struct sockaddr_un un={.sun_family=AF_UNIX};
	if(strlen(sock)>=sizeof(un.sun_path))return trlog_error(-ENAMETOOLONG,"invalid socket path");
	strcpy(un.sun_path,sock);
	if(access(un.sun_path,F_OK)==0)return trlog_error(-EEXIST,"socket %s exists",un.sun_path);
	else if(errno!=ENOENT)return terlog_error(-errno,"failed to access %s",un.sun_path);
	if((fd=socket(AF_UNIX,SOCK_STREAM|SOCK_NONBLOCK,0))<0)return terlog_error(-errno,"cannot create socket");
	if(bind(fd,(struct sockaddr*)&un,sizeof(un))<0){
		telog_error("cannot bind socket");
		goto fail;
	}
	if(listen(fd,1)<0){
		telog_error("cannot listen socket");
		goto fail;
	}
	chmod(un.sun_path,0666);
	tlog_info("listen socket %s as %d",sock,fd);
	return fd;
	fail:
	er=errno;
	close(fd);
	unlink(un.sun_path);
	ERET(er);
}

static void*confd_save_thread(void*d __attribute__((unused))){
	for(;;){
		sleep(conf_get_integer("confd.save_interval",10,0,0));
		if(!def_path)continue;
		if(conf_store_changed){
			int r=confd_save_file(def_path);
			if(r==0&&errno==0)conf_store_changed=false;
			sync();
		}
	}
	return NULL;
}

int confd_thread(int cfd){
	static size_t es=sizeof(struct epoll_event);
	int r,e=0,fd;
	open_socket_logfd_default();
	tlog_info("confd start with pid %d",getpid());
	if((fd=listen_confd_socket())<0)return -1;
	struct epoll_event*evs;
	setproctitle("confd");
	prctl(PR_SET_NAME,"Config Daemon",0,0,0);
	action_signals(
		(int[]){SIGINT,SIGHUP,SIGQUIT,SIGTERM},
		4,signal_handler
	);
	if((efd=epoll_create(64))<0)
		return terlog_error(-errno,"epoll_create failed");
	if(!(evs=malloc(es*64))){
		telog_error("malloc failed");
		e=-errno;
		goto ex;
	}
	memset(evs,0,es*64);
	ctl_fd(EPOLL_CTL_ADD,fd);
	if(cfd>=0){
		confd_internal_send_code(cfd,CONF_OK,0);
		close(cfd);
	}
	pthread_create(&save_thread,NULL,confd_save_thread,NULL);
	while(1){
		r=epoll_wait(efd,evs,64,-1);
		if(r==-1){
			if(errno==EINTR)continue;
			telog_error("epoll failed");
			e=-1;
			goto ex;
		}else if(r==0)continue;
		else for(int i=0;i<r;i++){
			int f=evs[i].data.fd;
			if(f==fd){
				int n=accept(f,NULL,NULL);
				if(n<0){
					ctl_fd(EPOLL_CTL_DEL,fd);
					continue;
				}
				fcntl(n,F_SETFL,O_RDWR|O_NONBLOCK);
				ctl_fd(EPOLL_CTL_ADD,n);
			}else{
				int x=confd_read(f);
				if(x==EOF)ctl_fd(EPOLL_CTL_DEL,f);
				else if(x==-4)goto ex;
			}
		}
	}
	ex:
	confd_cleanup(0);
	exit(e);
}

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: iniconfd [OPTION]...\n"
		"Start Init Config daemon.\n\n"
		"Options:\n"
		"\t-s, --socket <SOCKET>  Listen custom control socket (default is %s)\n"
		"\t-d, --daemon           Run in daemon\n"
		"\t-h, --help             Display this help and exit\n",
		DEFAULT_CONFD
	);
}

int initconfd_main(int argc __attribute__((unused)),char**argv __attribute__((unused))){
	static const struct option lo[]={
		{"help",    no_argument,       NULL,'h'},
		{"daemon",  no_argument,       NULL,'d'},
		{"socket",  required_argument, NULL,'s'},
		{NULL,0,NULL,0}
	};
	int o;
	bool daemon=false;
	while((o=b_getlopt(argc,argv,"hqdD:s:",lo,NULL))>0)switch(o){
		case 'h':return usage(0);
		case 'd':daemon=true;break;
		case 's':sock=b_optarg;break;
		default:return 1;
	}
	if(daemon){
		chdir("/");
		switch(fork()){
			case 0:break;
			case -1:return re_err(1,"fork");
			default:_exit(0);
		}
		if(setsid()<0)return re_err(1,"setsid");
		switch(fork()){
			case 0:break;
			case -1:return re_err(1,"fork");
			default:_exit(0);
		}
	}
	return confd_thread(-1);
}

int start_confd(char*tag,pid_t*p){
	int fds[2],r;
	if(confd>=0)ERET(EEXIST);
	if(pipe(fds)<0)return -errno;
	pid_t pid=fork();
	switch(pid){
		case -1:return -errno;
		case 0:
			close_all_fd((int[]){fds[1]},1);
			protect=true;
			r=confd_thread(fds[1]);
			exit(r);
	}
	close(fds[1]);
	struct confd_msg msg;
	do{if(confd_internal_read_msg(fds[0],&msg)<0)ERET(EIO);}
	while(msg.action!=CONF_OK);
	if(p)*p=pid;
	close(fds[0]);
	return open_default_confd_socket(false,tag);
}
