#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<stdbool.h>
#include<signal.h>
#include<string.h>
#include<sys/un.h>
#include<sys/socket.h>
#include<sys/select.h>
#include"list.h"
#include"defines.h"
#include"logger_internal.h"
#define TAG "loggerd"

static bool clean=false;
struct fd_item{
	bool socket,delete;
	char path[PATH_MAX];
	int fd;
};
static list*fds;

static int fd_item_add(int fd,char*path,bool socket){
	if(fd<0)ERET(EINVAL);
	size_t s=sizeof(struct fd_item);
	struct fd_item*p=malloc(s);
	if(!p)goto fail;
	memset(p,0,s);
	p->fd=fd;
	p->socket=socket;
	if(path){
		strncpy(p->path,path,sizeof(p->path)-1);
		p->delete=true;
	}
	list*new=list_new(p);
	if(!new)goto fail;
	if(!fds)fds=new;
	else list_push(fds,new);
	return (errno=0);
	fail:
	logger_internal_printf(LEVEL_ALERT,TAG,"cannot allocate new fd");
	close(fd);
	if(path)unlink(path);
	ERET(ENOMEM);
}

static int fd_item_remove(void*d){
	if(!d)ERET(EINVAL);
	struct fd_item*f=(struct fd_item*)d;
	close(f->fd);
	if(f->delete)unlink(f->path);
	free(f);
	return 0;
}

static int loggerd_add_listen(char*path){
	if(!path)ERET(EINVAL);
	int fd;
	struct sockaddr_un un;
	memset(&un,0,sizeof(un));
	un.sun_family=AF_UNIX;
	strncpy(un.sun_path,path,sizeof(un.sun_path)-1);
	if(access(path,F_OK)==0){
		logger_internal_printf(LEVEL_ERROR,"loggerd","socket %s exists",path);
		ERET(EEXIST);
	}else if(errno!=ENOENT){
		logger_internal_printf(LEVEL_ERROR,"loggerd","failed to access %s: %m",path);
		return -errno;
	}
	if((fd=socket(AF_UNIX,SOCK_STREAM,0))<0){
		logger_internal_printf(LEVEL_ERROR,"loggerd","cannot create socket: %m");
		return -errno;
	}
	if(bind(fd,(struct sockaddr*)&un,sizeof(un))<0){
		int er=errno;
		logger_internal_printf(LEVEL_ERROR,"loggerd","cannot bind socket: %m");
		close(fd);
		unlink(path);
		ERET(er);
	}
	if(listen(fd,1)<0){
		int er=errno;
		logger_internal_printf(LEVEL_ERROR,"loggerd","cannot listen socket: %m");
		close(fd);
		unlink(path);
		ERET(er);
	}
	logger_internal_printf(LEVEL_ALERT,"loggerd","add new listen %s",path);
	return fd_item_add(fd,path,true);
}

static int loggerd_read(int fd){
	if(fd<0)ERET(EINVAL);
	errno=0;
	int e;
	size_t s;
	void*data=NULL;
	struct log_msg msg;
	if((e=logger_internal_read_msg(fd,&msg))<0)goto ex;
	if(msg.size!=0){
		if(!(data=malloc(msg.size+2)))return -2;
		memset(data,0,msg.size+2);
		if((s=read(fd,data,msg.size))<=0&&errno!=EAGAIN){
			e=EOF;
			goto ex;
		}
		if(s!=msg.size){
			e=-2;
			goto ex;
		}
	}
	enum log_oper ret=LOG_OK;
	int retdata=0;
	char*ss=(char*)data;
	switch(msg.oper){

		// command response
		case LOG_OK:case LOG_FAIL:break;

		// request add log item
		case LOG_ADD:
			logger_internal_write((struct log_item*)data);
		break;

		// open log file
		case LOG_OPEN:
			if(!open_log_file(ss)){
				ret=LOG_FAIL,retdata=errno;
				break;
			}
			logger_internal_add(ss,LEVEL_DEBUG,&file_logger);
			logger_internal_set(ss,true);
		break;

		// close log file
		case LOG_CLOSE:
			close_log_file(ss);
			logger_internal_set(ss,false);
		break;

		// add new listen
		case LOG_LISTEN:
			loggerd_add_listen((char*)data);
			if(errno!=0)ret=LOG_FAIL,retdata=errno;
		break;

		// klog available now
		case LOG_KLOG:
			if(init_kmesg()!=0)ret=LOG_FAIL,retdata=errno;
		break;

		// clean log buffer
		case LOG_CLEAR:
			clean_log_buffers();
		break;

		// terminate loggerd
		case LOG_QUIT:
			logger_internal_printf(LEVEL_EMERG,"loggerd","receive exit signal");
			e=-4;
		break;

		// unknown
		default:
			logger_internal_printf(
				LEVEL_WARNING,
				"loggerd",
				"operation %s not implemented",
				oper2string(msg.oper)
			);
	}
	logger_internal_send_msg(fd,ret,&retdata,sizeof(int));
	ex:if(data)free(data);
	return e;
}

static void logger_cleanup(int s __attribute__((unused))){
	if(clean)return;
	clean=true;
	close_all_file();
	if(fds)list_free_all(fds,fd_item_remove);
	fds=NULL;
	logger_internal_clean();
}

int loggerd_thread(int fd){
	if(fd<0)ERET(EINVAL);
	int r,max,e=0;
	fd_item_add(fd,NULL,false);
	struct timeval timeout={1,0};
	fd_set fs;
	logger_internal_add("stderr",LEVEL_DEBUG,&file_logger);
	logger_internal_set("stderr",true);
	logger_internal_send_msg(fd,LOG_OK,NULL,0);
	logger_internal_printf(LEVEL_INFO,TAG,"loggerd start with pid %d",getpid());
	signal(SIGINT,logger_cleanup);
	signal(SIGHUP,logger_cleanup);
	signal(SIGTERM,logger_cleanup);
	signal(SIGQUIT,logger_cleanup);
	while(1){
		list*l;
		FD_ZERO(&fs);
		max=-1;
		if((l=list_first(fds)))do{
			struct fd_item*a=LIST_DATA(l,struct fd_item*);
			FD_SET(a->fd,&fs);
			max=a->fd>max?a->fd:max;
		}while((l=l->next));
		if(max<0){
			logger_internal_printf(LEVEL_ERROR,TAG,"no any stream to read, exiting");
			e=-1;
			goto ex;
		}
		r=select(max+1,&fs,NULL,NULL,&timeout);
		if(r==-1){
			logger_internal_printf(LEVEL_ERROR,TAG,"select failed: %m");
			e=-1;
			goto ex;
		}else if(r==0)continue;
		else if((l=list_first(fds)))do{
			struct fd_item*a=LIST_DATA(l,struct fd_item*);
			if(!FD_ISSET(a->fd,&fs))continue;
			if(a->socket)fd_item_add(accept(a->fd,NULL,NULL),NULL,false);
			else{
				int z=loggerd_read(a->fd);
				if(z==EOF){
					if(list_is_alone(l)){
						list_free_item(fds,fd_item_remove);
						fds=NULL;
					}else {
						if(fds==l){
							if(fds->next)fds=fds->next;
							else if(fds->prev)fds=fds->prev;
						}
						list_remove_free(l,fd_item_remove);
					}
					break;
				}else if(z==-4)goto ex;
			}
		}while((l=l->next));
	}
	ex:
	logger_cleanup(0);
	exit(e);
}
