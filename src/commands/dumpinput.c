/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include<fcntl.h>
#include<stdio.h>
#include<dirent.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<string.h>
#include<sys/epoll.h>
#include<sys/socket.h>
#include<linux/input.h>
#include<linux/netlink.h>
#include"pathnames.h"
#include"defines.h"
#include"system.h"
#include"output.h"
#include"array.h"
#include"list.h"
#include"devd.h"
#include"str.h"

enum code_category{
	CATE_NONE=0x0,
	CATE_TYPE=0x1,
	CATE_CODE=0x2,
};

enum device_type{
	DEV_NONE=0x0,
	DEV_INPUT=0x1,
	DEV_NETLINK=0x2,
};

struct poll_device{
	enum device_type type;
	int fd;
	char path[64];
	char name[40];
	struct epoll_event event;
	char desc[128];
};

struct input_code{
	uint16_t type;
	uint16_t code;
	char name[123];
	enum code_category category:8;
};

static int efd=-1;
static bool run=true;
static list*devices=NULL;

static struct input_code codes[]={
	#define DECL_TYPE(_type,_value){\
		.category=CATE_TYPE,\
		.type=(_value),\
		.code=0,\
		.name=(_type)\
	},
	#define DECL_INPUT(_type,_code,_value){\
		.category=CATE_CODE,\
		.type=(_type),\
		.code=(_value),\
		.name=(_code)\
	},
	#include"input_code.h"
	{.category=0,.type=0,.code=0,.name=""}
};

static bool device_comp(list*l,void*data){
	LIST_DATA_DECLARE(x,l,struct poll_device*);
	return x&&data&&strcmp(x->path,(char*)data)==0;
}

static struct poll_device*add_epoll(struct poll_device*dev){
	int f;
	struct poll_device*d;
	if(!(d=memdup(dev,sizeof(struct poll_device)))){
		perror(_("alloc device failed"));
		close(dev->fd);
		return NULL;
	}
	if((f=fcntl(d->fd, F_GETFL))>=0)
		fcntl(d->fd,F_SETFL,f|O_NONBLOCK);
	if((f=fcntl(d->fd, F_GETFD))>=0)
		fcntl(d->fd,F_SETFD,f|FD_CLOEXEC);
	d->event.events=EPOLLIN;
	d->event.data.ptr=d;
	int r=epoll_ctl(efd,EPOLL_CTL_ADD,d->fd,&d->event);
	if(r<0){
		if(!d->path[0])fprintf(stderr,_("add fd %d to epoll failed"),d->fd);
		else fprintf(stderr,_("add fd %d(%s) to epoll failed"),d->fd,d->path);
		fprintf(stderr,(errno!=0)?": %m\n":"\n");
		close(d->fd);
		free(d);
		return NULL;
	}
	list_obj_add_new(&devices,d);
	return d;
}

static int clean_device(struct poll_device*dev){
	if(!dev)return -1;
	epoll_ctl(efd,EPOLL_CTL_DEL,dev->fd,&dev->event);
	if(dev->fd>=0)close(dev->fd);
	if(!dev->name[0])printf(_(" - Delete fd %d\n"),dev->fd);
	else printf(_(" - Delete device %s fd %d\n"),dev->name,dev->fd);
	memset(dev,0,sizeof(struct poll_device));
	free(dev);
	return 0;
}

static int close_device(struct poll_device*dev){
	list_obj_del_data(&devices,dev,NULL);
	return clean_device(dev);
}

static int free_device(void*dev){
	return clean_device(dev);
}

static int open_device(const char*device){
	char*p=NULL;
	struct poll_device dev;
	memset(&dev,0,sizeof(dev));
	if(!device||!*device||strlen(device)>=sizeof(dev.path)){
		fprintf(stderr,_("invalid device '%s'\n"),device);
		return -1;
	}
	strncpy(dev.path,device,sizeof(dev.path)-1);
	trim(dev.path);
	if(!dev.path[0]){
		fprintf(stderr,_("invalid device '%s'\n"),device);
		return -1;
	}
	if(list_search_one(devices,device_comp,dev.path)){
		fprintf(stderr,_("skip device '%s'\n"),device);
		return -1;
	}
	dev.type=DEV_INPUT;
	strncpy(
		dev.name,
		((p=strrchr(dev.path,'/'))?
		p+1:dev.path),
		sizeof(dev.name)-1
	);
	if((dev.fd=open(dev.path,O_RDONLY))<0){
		fprintf(stderr,_("open %s failed"),device);
		fprintf(stderr,(errno!=0)?": %m\n":"\n");
		return -1;
	}
	ioctl(dev.fd,EVIOCGNAME(sizeof(dev.desc)-1),dev.desc);
	if(!add_epoll(&dev))return -1;
	printf(
		_(" + Add device %s fd %d (%s)\n"),
		dev.name,dev.fd,
		dev.desc[0]?dev.desc:_("Unknown")
	);
	return dev.fd;
}

static int init_netlink(){
	struct poll_device dev;
	struct sockaddr_nl n={
		.nl_family=AF_NETLINK,
		.nl_groups=1
	};
	dev.type=DEV_NETLINK;
	n.nl_pid=getpid();
	if((dev.fd=socket(
		AF_NETLINK,
		SOCK_DGRAM,
		NETLINK_KOBJECT_UEVENT
	))<0){
		perror(_("cannot create netlink socket"));
		return -1;
	}
	if(bind(dev.fd,(struct sockaddr*)&n,sizeof(n))<0){
		perror(_("cannot bind netlink socket"));
		close(dev.fd);
		return -1;
	}
	return add_epoll(&dev)?dev.fd:-1;
}

static int read_device(struct poll_device*dev){
	struct input_event in;
	char*type=NULL,*code=NULL;
	errno=0;
	if(!dev||dev->type!=DEV_INPUT)return -1;
	ssize_t r=read(dev->fd,&in,sizeof(struct input_event));
	if(errno==EINTR)return 0;
	if(errno==EAGAIN)return 0;
	if(r<=0){
		if(r<0){
			fprintf(stderr,_("read %d(%s) failed"),dev->fd,dev->path);
			fprintf(stderr,(errno!=0)?": %m\n":"\n");
		}
		close_device(dev);
		return 0;
	}
	for(size_t i=0;i<ARRLEN(codes);i++)switch(codes[i].category){
		case CATE_TYPE:
			if(codes[i].type==in.type)
				type=codes[i].name;
		break;
		case CATE_CODE:
			if(
				codes[i].type==in.type&&
				codes[i].code==in.code
			)code=codes[i].name;
		break;
		default:;
	}
	if(!type)type=_("Unknown");
	if(!code)code=_("Unknown");
	char buff[16];
	memset(buff,0,sizeof(buff));
	for(size_t i=0;i<10-strlen(dev->name);i++)buff[i]=' ';
	printf(
		_(" ! Device %s:%s "
		"Type: %-6s ( %3d / 0x%03X ), "
		"Code: %-28s ( %4d / 0x%04X ), "
		"Value: %8d / 0x%08X (%s)\n"),
		dev->name,buff,
		type,in.type,in.type,
		code,in.code,in.code,
		in.value,in.value,
		dev->desc[0]?dev->desc:_("Unknown")
	);
	return 0;
}

static int read_netlink(struct poll_device*dev){
	uevent event;
	char buf[8192],path[64],*v;
	if(!dev||dev->type!=DEV_NETLINK)return -1;
	memset(buf,0,sizeof(buf));
	ssize_t s=read(dev->fd,buf,sizeof(buf)-1);
	if(s<=0){
		if(errno==EINTR)return 0;
		if(errno==EAGAIN)return 0;
		if(s<0){
			fprintf(stderr,_("read %d failed"),dev->fd);
			fprintf(stderr,(errno!=0)?": %m\n":"\n");
		}
		close_device(dev);
		return 0;
	}
	for(ssize_t x=0;x<s;x++)if(buf[x]==0)buf[x]='\n';
	if(!(v=strchr(buf,'\n')))return 0;
	v++;
	uevent_parse(v,&event);
	if(!event.subsystem||!event.devname)goto done;
	if(strcmp(event.subsystem,"input")!=0)goto done;
	memset(path,0,sizeof(path));
	snprintf(path,sizeof(path)-1,_PATH_DEV"/%s",event.devname);
	switch(event.action){
		case ACTION_ADD:open_device(path);break;
		case ACTION_REMOVE:{
			list*l=list_search_one(devices,device_comp,path);
			if(!l)break;
			LIST_DATA_DECLARE(d,l,struct poll_device*);
			list_obj_del_data(&devices,d,(runnable_t*)close_device);
		}break;
		default:;
	}
	done:kvarr_free(event.environs);
	return 0;
}

static int process_device(struct poll_device*dev){
	if(!dev)return -1;
	switch(dev->type){
		case DEV_INPUT:return read_device(dev);
		case DEV_NETLINK:return read_netlink(dev);
		default:return -1;
	}
}

static int handle_arg(char*arg){
	if(!arg[0])return 0;
	if(arg[0]=='-'){
		if(
			strcasecmp(arg,"-h")==0||
			strcasecmp(arg,"--help")==0
		){
			printf(_("Usage: dumpinput [INPUT]...\n"));
			return 1;
		}
		fprintf(stderr,_("Unknown option: %s\n"),arg);
		exit(1);
		return 2;
	}
	open_device(arg);
	return 0;
}

static void scan_devices(){
	DIR*d;
	char path[64];
	struct dirent*ent;
	if(!(d=opendir(_PATH_DEV"/input"))){
		perror(_("opendir failed"));
		return;
	}
	while((ent=readdir(d))){
		if(ent->d_type!=DT_CHR)continue;
		memset(path,0,sizeof(path));
		snprintf(
			path,sizeof(path)-1,
			_PATH_DEV"/input/%s",
			ent->d_name
		);
		open_device(path);
	}
	closedir(d);
}

static void signal_handler(int sig){
	fprintf(
		stderr,
		_("Terminated by %s\n"),
		signame(sig)
	);
	run=false;
}

int dumpinput_main(int argc,char**argv){
	int r,e=0;
	struct epoll_event*evs=NULL;
	if((efd=epoll_create(64))<0)
		EDONE(perror(_("epoll create failed")));
	if(!(evs=malloc(sizeof(struct epoll_event)*64)))
		EDONE(perror(_("epoll malloc failed")));
	for(int i=1;i<64;i++)switch(i){
		case SIGKILL:case SIGSTOP:break;
		case SIGCONT:case SIGWINCH:break;
		default:signal(i,signal_handler);
	}
	init_netlink();
	if(argc<=1)scan_devices();
	else for(int i=1;i<argc;i++)
		if((e=handle_arg(argv[i])))
			EDONE(e--);
	if(!devices)EDONE(fprintf(
		stderr,_("No any devices found\n")
	));
	while(run){
		errno=0;
		r=epoll_wait(efd,evs,64,-1);
		if(errno==EAGAIN)continue;
		if(errno==EINTR)continue;
		if(r<0){
			perror(_("epoll wait failed"));
			break;
		}
		for(int i=0;i<r;i++)
			process_device(evs[i].data.ptr);
	}
	printf(_("Cleanup...\n"));
	end:
	if(devices)list_free_all(devices,free_device);
	if(efd>=0)close(efd);
	if(evs)free(evs);
	return e;
	done:e=-1;goto end;
}
