#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<stdbool.h>
#include<signal.h>
#include<string.h>
#include<sys/un.h>
#include<sys/epoll.h>
#include<sys/prctl.h>
#include<sys/socket.h>
#include"defines.h"
#include"proctitle.h"
#include"logger_internal.h"
#define TAG "loggerd"

struct socket_data{
	int fd;
	bool server;
	struct sockaddr_un un;
};

static bool clean=false;
static int efd=-1;
static list*slist=NULL;

static struct socket_data*new_socket_data(int fd,bool server,char*path){
	struct socket_data*sd=malloc(sizeof(struct socket_data));
	if(!sd)return NULL;
	memset(sd,0,sizeof(struct socket_data));
	sd->fd=fd;
	sd->server=server;
	sd->un.sun_family=AF_UNIX;
	if(path)strncpy(
		sd->un.sun_path,
		path,
		sizeof(sd->un.sun_path)-1
	);
	return sd;
}

static struct socket_data*add_fd(int fd,bool server,char*path){
	list*item=NULL;
	struct epoll_event*ev=NULL;
	struct socket_data*data=NULL;
	if(!(ev=malloc(sizeof(struct epoll_event))))goto fail;
	if(!(data=new_socket_data(fd,server,path)))goto fail;
	if(!(item=list_new(data)))goto fail;
	memset(ev,0,sizeof(struct epoll_event));
	ev->events=EPOLLIN,ev->data.ptr=data;
	epoll_ctl(efd,EPOLL_CTL_ADD,fd,ev);
	free(ev);
	if(!slist)slist=item;
	else list_push(slist,item);
	return data;
	fail:
	if(ev)free(ev);
	if(data)free(data);
	return NULL;
}

static int _hand_remove_data(void*d){
	struct socket_data*data=(struct socket_data*)d;
	if(!d)return -1;
	epoll_ctl(efd,EPOLL_CTL_DEL,data->fd,NULL);
	if(data->server)unlink(data->un.sun_path);
	close(data->fd);
	free(d);
	return 0;
}

static void del_fd(struct socket_data*data){
	if(!data)return;
	if(!slist){
		_hand_remove_data(data);
		return;
	}
	list*d=list_first(slist),*next;
	if(d)do{
		next=d->next;
		if(data!=d->data)continue;
		if(list_is_alone(d)){
			list_free_all(d,_hand_remove_data);
			slist=NULL;
			break;
		}else{
			if(d==slist){
				if(d->prev)slist=d->prev;
				if(d->next)slist=d->next;
			}
			list_remove_free(d,_hand_remove_data);
		}
	}while((d=next));
}

static int loggerd_add_listen(char*path){
	if(!path)ERET(EINVAL);
	struct socket_data*sd;
	int fd;

	if((fd=socket(AF_UNIX,SOCK_STREAM,0))<0){
		logger_internal_printf(
			LEVEL_ERROR,
			TAG,
			"cannot create socket: %m"
		);
		return -errno;
	}

	if(access(path,F_OK)==0){
		logger_internal_printf(
			LEVEL_ERROR,
			TAG,
			"socket %s exists",
			path
		);
		ERET(EEXIST);
	}else if(errno!=ENOENT){
		logger_internal_printf(
			LEVEL_ERROR,
			TAG,
			"failed to access %s: %m",
			path
		);
		return -errno;
	}else errno=0;
	if(!(sd=add_fd(fd,true,path)))return -1;

	if(bind(
		sd->fd,
		(struct sockaddr*)&sd->un,
		sizeof(sd->un)
	)<0){
		int er=errno;
		logger_internal_printf(
			LEVEL_ERROR,
			TAG,
			"cannot bind socket: %m"
		);
		del_fd(sd);
		ERET(er);
	}

	if(listen(sd->fd,1)<0){
		int er=errno;
		logger_internal_printf(
			LEVEL_ERROR,
			TAG,
			"cannot listen socket: %m"
		);
		del_fd(sd);
		ERET(er);
	}

	logger_internal_printf(
		LEVEL_ALERT,
		TAG,
		"add new listen %s",
		path
	);
	return 0;
}

static int loggerd_read(int fd){
	if(fd<0)ERET(EINVAL);
	errno=0;
	struct log_msg msg;
	int e=logger_internal_read_msg(fd,&msg);
	if(e<0)return e;
	else if(e==0)return 0;
	enum log_oper ret=LOG_OK;
	int retdata=0;
	switch(msg.oper){

		// command response
		case LOG_OK:case LOG_FAIL:break;

		// request add log item
		case LOG_ADD:
			logger_internal_write(&msg.data.log);
		break;

		// open log file
		case LOG_OPEN:
			if(open_log_file(msg.data.string)<0){
				ret=LOG_FAIL,retdata=errno;
				break;
			}
			logger_internal_add(
				msg.data.string,
				LEVEL_DEBUG,
				&file_logger
			);
			logger_internal_set(
				msg.data.string,
				true
			);
		break;

		// close log file
		case LOG_CLOSE:
			close_log_file(msg.data.string);
			logger_internal_set(msg.data.string,false);
		break;

		// add new listen
		case LOG_LISTEN:
			loggerd_add_listen(msg.data.string);
			if(errno!=0)ret=LOG_FAIL,retdata=errno;
		break;

		// klog available now
		case LOG_KLOG:
			if(init_kmesg()!=0)
				ret=LOG_FAIL,retdata=errno;
		break;

		// start syslog forwarder
		case LOG_SYSLOG:
			if(init_syslog()!=0)
				ret=LOG_FAIL,retdata=errno;
		break;

		// clean log buffer
		case LOG_CLEAR:
			clean_log_buffers();
		break;

		// terminate loggerd
		case LOG_QUIT:
			logger_internal_printf(
				LEVEL_EMERG,
				TAG,
				"receive exit signal"
			);
			e=-4;
		break;

		// unknown
		default:
			logger_internal_printf(
				LEVEL_WARNING,
				TAG,
				"operation %s not implemented",
				oper2string(msg.oper)
			);
	}
	logger_internal_send_code(fd,ret,retdata);
	return e;
}

static void logger_cleanup(int s __attribute__((unused))){
	if(clean)return;
	clean=true;
	if(slist)list_free_all(slist,_hand_remove_data);
	close_all_file();
	logger_internal_clean();
}

int loggerd_thread(int fd){
	static size_t es=sizeof(struct epoll_event);
	if(fd<0)ERET(EINVAL);
	int r,e=0;
	struct epoll_event*evs;
	struct socket_data*sd;
	logger_internal_add("stderr",LEVEL_DEBUG,&file_logger);
	logger_internal_set("stderr",true);
	logger_internal_send_string(fd,LOG_OK,NULL);
	logger_internal_printf(
		LEVEL_INFO,
		TAG,
		"loggerd start with pid %d",
		getpid()
	);
	setproctitle("initloggerd");
	prctl(PR_SET_NAME,"Logger Daemon",0,0,0);
	handle_signals(
		(int[]){SIGINT,SIGHUP,SIGQUIT,SIGTERM},
		4,logger_cleanup
	);
	if((efd=epoll_create(64))<0)
		return terlog_error(-errno,"epoll_create failed");
	if(!(evs=malloc(es*64))){
		telog_error("malloc failed");
		e=-errno;
		goto ex;
	}
	memset(evs,0,es*64);
	add_fd(fd,false,NULL);
	while(1){
		r=epoll_wait(efd,evs,64,-1);
		if(r==-1){
			if(errno==EINTR)continue;
			logger_internal_printf(
				LEVEL_ERROR,
				TAG,
				"epoll failed: %m"
			);
			e=-1;
			goto ex;
		}else if(r==0)continue;
		else for(int i=0;i<r;i++){
			if(!(sd=(struct socket_data*)evs[i].data.ptr))
				continue;
			int f=sd->fd;
			if(sd->server){
				int n=accept(f,NULL,NULL);
				if(n<0){
					del_fd(sd);
					evs[i].data.ptr=sd=NULL;
					continue;
				}
				fcntl(n,F_SETFL,O_RDWR|O_NONBLOCK);
				add_fd(n,false,NULL);
			}else{
				int x=loggerd_read(f);
				if(x==EOF){
					del_fd(sd);
					evs[i].data.ptr=sd=NULL;
				}else if(x==-4)goto ex;
			}
		}
	}
	ex:
	logger_cleanup(0);
	exit(e);
}
