/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<stdio.h>
#include<fcntl.h>
#include<errno.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/sendfile.h>
#define TAG "firmware"
#include"devd.h"
#include"defines.h"
#include"cmdline.h"
#include"uevent.h"
#include"logger.h"

int write_firmware(int cfd,char*path,char*devpath){
	if(cfd<0||!path||!devpath)ERET(EINVAL);
	int ifd,ofd=-1,r;
	char opath[PATH_MAX]={0};
	struct stat st;
	snprintf(opath,PATH_MAX-1,_PATH_SYS"%s/data",devpath);
	if((ifd=open(path,O_RDONLY))<0){
		r=terlog_error(-1,"open firmware %s for read",path);
		goto ex;
	}
	if((ofd=open(opath,O_WRONLY|O_SYNC))<0){
		r=terlog_error(-1,"open %s for send firmware",devpath);
		goto ex;
	}
	if(fstat(ifd,&st)<0){
		r=terlog_error(-1,"stat firmware %s",path);
		goto ex;
	}
	write(cfd,"1\n",2);
	if(st.st_size>0&&sendfile(ofd,ifd,0,st.st_size)!=st.st_size){
		if(errno==ENOSYS){
			void*data=malloc(st.st_size+1);
			if(!data){
				r=terlog_error(-2,"malloc firmware space %s failed",path);
				goto ex;
			}
			if(read(ifd,data,st.st_size)!=st.st_size){
				r=terlog_error(-2,"read firmware %s failed",path);
				free(data);
				goto ex;
			}
			if(write(ofd,data,st.st_size)!=st.st_size){
				r=terlog_error(-2,"write firmware %s failed",path);
				free(data);
				goto ex;
			}
			free(data);
		}else{
			r=terlog_error(-2,"write firmware %s failed(sendfile)",path);
			goto ex;
		}
	}
	r=trlog_info(0,"sent firmware %s (%ld bytes)",path,st.st_size);
	ex:
	if(r==-1)write(cfd,"-1\n",3);
	else write(cfd,"0\n",2);
	if(ifd<0)close(ifd);
	if(ofd<0)close(ofd);
	if(cfd<0)close(cfd);
	return r;
}
char* search_firmware(char*firm,char*buff,size_t len){
	if(!firm||!buff)EPRET(EINVAL);
	for(int i=sizeof(firmware_list)/sizeof(char*)-1;i>=0;i--){
		if(!(firmware_list[i]))continue;
		memset(buff,0,len);
		snprintf(buff,len-1,"%s/%s",firmware_list[i],firm);
		if(access(buff,R_OK)==0){
			tlog_debug("found firmware at %s",buff);
			errno=0;
			return buff;
		}
		if(errno!=ENOENT)telog_warn("failed to access %s",buff);
	}
	telog_warn("firmware %s not found",firm);
	EPRET(ENONET);
}

int process_firmware_load(uevent*event){
	if(
		!event||
		!event->devpath||
		!event->environs||
		!event->subsystem||
		event->action!=ACTION_ADD||
		strcmp(event->subsystem,"firmware")!=0
	)return -1;
	int cfd;
	char*firm=kvarr_get(event->environs,"FIRMWARE",NULL);
	char cpath[PATH_MAX],fpath[PATH_MAX];
	if(!firm)return -1;
	tlog_debug("kernel request firmware %s",firm);
	memset(cpath,0,PATH_MAX);
	snprintf(cpath,PATH_MAX-1,_PATH_SYS"%s/loading",event->devpath);
	if((cfd=open(cpath,O_WRONLY|O_SYNC))<0)return terlog_error(-1,"open %s for control",cpath);
	if(!search_firmware(firm,fpath,PATH_MAX-1)){
		write(cfd,"-1\n",3);
		close(cfd);
		return -1;
	}
	return write_firmware(cfd,fpath,event->devpath);
}
