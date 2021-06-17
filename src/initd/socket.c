#define _GNU_SOURCE
#include<errno.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<sys/un.h>
#include<sys/stat.h>
#include<sys/epoll.h>
#include<sys/socket.h>
#include"init_internal.h"
#include"logger.h"
#include"defines.h"
#define TAG "init"

struct{
	int efd,size;
	struct epoll_event*evs;
}ep={
	.efd=-1,
	.size=64,
	.evs=NULL
};

int listen_init_socket(){
	static struct sockaddr_un un={
		.sun_family=AF_UNIX,
		.sun_path=DEFAULT_INITD
	};
	int er,o=1,sfd;

	// create socket
	if((sfd=socket(AF_UNIX,SOCK_STREAM|SOCK_NONBLOCK,0))<0)
		return terlog_error(-errno,"cannot create socket");

	// check socket exists
	if(access(un.sun_path,F_OK)==0){
		if(connect(sfd,(struct sockaddr*)&un,sizeof(un))!=0||errno!=ECONNREFUSED){
			close(sfd);
			return trlog_error(-EEXIST,"socket %s exists",un.sun_path);
		}else unlink(un.sun_path);
	}else if(errno!=ENOENT){
		close(sfd);
		return terlog_error(-errno,"failed to access %s",un.sun_path);
	}

	// bind socket
	if(bind(sfd,(struct sockaddr*)&un,sizeof(un))<0){
		telog_error("cannot bind socket");
		goto fail;
	}

	// listen socket
	if(listen(sfd,1)<0){
		telog_error("cannot listen socket");
		goto fail;
	}

	// change permission
	chmod(un.sun_path,0600);
	chown(un.sun_path,0,0);

	// require ucred
	setsockopt(sfd,SOL_SOCKET,SO_PASSCRED,&o,sizeof(o));
	return sfd;
	fail:
	er=errno;
	if(er<=0)er=1;
	close(sfd);
	unlink(un.sun_path);
	ERET(-er);
}

static inline int ctl_fd(int action,int fd){
	static struct epoll_event s={.events=EPOLLIN};
	s.data.fd=fd;
	int r=epoll_ctl(
		ep.efd,action,fd,
		action==EPOLL_CTL_ADD?&s:NULL
	);
	if(action==EPOLL_CTL_DEL)close(fd);
	return r;
}

static int clean_epoll(){
	if(ep.efd>=0)close(ep.efd);
	if(ep.evs)free(ep.evs);
	ep.efd=-1,ep.evs=NULL;
	return 0;
}

static int init_epoll(int sfd){
	static size_t es=sizeof(struct epoll_event);
	if((ep.efd=epoll_create(64))<0)
		return terlog_error(-errno,"epoll_create failed");
	if(!(ep.evs=malloc(es*ep.size)))
		return terlog_error(-errno,"malloc failed");
	memset(ep.evs,0,es*ep.size);
	ctl_fd(EPOLL_CTL_ADD,sfd);
	return 0;
}

static int recv_init_socket(int cfd){
	static socklen_t credsize=sizeof(struct ucred);
	struct init_msg msg;
	struct ucred cred;
	int z=init_recv_data(cfd,&msg);
	if(z<0&&errno==EAGAIN)return 0;
	if(
		z<0||
		getsockopt(
			cfd,
			SOL_SOCKET,SO_PEERCRED,
			&cred,&credsize
		)!=0
	){
		ctl_fd(EPOLL_CTL_DEL,cfd);
		return -1;
	}
	return init_process_data(cfd,&cred,&msg);
}

int init_process_socket(int sfd){
	if(sfd<0)return clean_epoll();
	if(ep.efd<0&&init_epoll(sfd)<0)return -1;
	int r=epoll_wait(ep.efd,ep.evs,ep.size,-1);
	if(r==-1){
		if(errno==EINTR)return 0;
		return terlog_error(-1,"epoll failed");
	}else if(r==0)return 0;
	else for(int i=0;i<r;i++){
		int s=ep.evs[i].data.fd;
		if(s==sfd){
			int n=accept(s,NULL,NULL);
			if(n<0){
				if(
					errno==EINTR||
					errno==EAGAIN
				)continue;
				telog_error("accept on %d failed",s);
				close(s);
				return -1;
			}
			fcntl(n,F_SETFL,O_RDWR|O_NONBLOCK);
			ctl_fd(EPOLL_CTL_ADD,n);
		}else recv_init_socket(s);
	}
	return 0;
}
