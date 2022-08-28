/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<string.h>
#include<stdlib.h>
#include<stdbool.h>
#include<dirent.h>
#include<libgen.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/ioctl.h>
#include<sys/sysmacros.h>
#include<linux/loop.h>
#include<blkid/blkid.h>
#include"str.h"
#include"logger.h"
#include"system.h"
#include"defines.h"
#include"pathnames.h"

bool is_virt_dir(const struct dirent*d){
	return
		strcmp(d->d_name,".")==0||
		strcmp(d->d_name,"..")==0;
}

ssize_t get_fd_path(int fd,char*buff,size_t size){
	if(fd<0||!buff)return -1;
	memset(buff,0,size);
	char path[64]={0};
	snprintf(path,63,_PATH_PROC_SELF"/fd/%d",fd);
	return readlink(path,buff,size);
}

int write_file(int dir,char*file,const char*str,size_t len,mode_t mode,bool lf,bool create,bool trunc){
	if(!file||!str)return -1;
	int fd,i,e,opt;
	errno=0;
	opt=O_WRONLY;
	if(create)opt|=O_CREAT;
	if(trunc)opt|=O_TRUNC;
	fd=openat(dir<0?AT_FDCWD:dir,file,opt,mode);
	if(fd<0)return fd;
	errno=0;
	i=write(fd,str,len>0?len:strlen(str));
	e=errno;
	if(lf)write(fd,"\n",1);
	close(fd);
	errno=e;
	return i;
}

int simple_file_write(char*file,char*content){
	int fd,i;
	if((fd=open(file,O_CREAT|O_WRONLY|O_SYNC|O_TRUNC,0644))<0)return -1;
	errno=0;
	i=write(fd,content,strlen(content));
	close(fd);
	return i;
}

int simple_file_append(char*file,char*content){
	int fd,i;
	if((fd=open(file,O_WRONLY|O_SYNC|O_APPEND))<0)return -1;
	errno=0;
	i=write(fd,content,strlen(content));
	close(fd);
	return i;
}

static bool is_fail(char*buff,size_t idx){
	buff[idx]=0;
	bool p=true;
	if(idx<=0)p=false;
	else for(size_t k=0;k<idx;k++)if(buff[k]!='.')p=false;
	return p;
}

bool is_invalid_path(char*path){
	if(!path)return false;
	char*buff=NULL,*pb=NULL;
	size_t x=0,l=strlen(path),idx=0,ks=16;
	if(l<=0)return false;
	if(!(buff=malloc(ks)))return false;
	else memset(buff,0,ks);
	bool valid=true;
	while(x<l){
		if(idx>=ks-1){
			ks*=2;
			if((pb=realloc(buff,ks)))buff=pb;
			else{
				free(buff);
				return false;
			}
		}
		switch(path[x]){
			case '\n':case '\r':case '/':case '\\':
				if(is_fail(buff,idx))valid=false;
				idx=-1;
			break;
			default:buff[idx]=path[x];
		}
		x++;idx++;
	}
	if(is_fail(buff,idx))valid=false;
	free(buff);
	return valid;
}

int wait_exists(char*path,long time,long step){
	long cur=0;
	do{
		if(access(path,F_OK)==0)return 0;
		if(errno!=ENOENT)return -1;
		cur+=step;
		usleep(step*1000);
	}while(cur<time);
	errno=ETIME;
	return -2;
}

int mkdir_res(char*path){
	if(!path)return -1;
	if(strcmp(path,"/")==0)return 0;
	bool slash=false;
	for(size_t x=0;x<=strlen(path);x++)if(path[x]=='/')slash=true;
	char s[PATH_MAX]={0},*x;
	strcpy(s,path);
	if(!(x=dirname(s)))return -1;
	mkdir(x,0755);
	if(access(x,F_OK)==0)slash=false;
	return slash?mkdir_res(x):0;
}

int add_link(char*path,char*name,char*dest){
	char p[PATH_MAX]={0};
	snprintf(p,PATH_MAX-1,"%s/%s",path,name);
	return symlink(dest,p);
}

int remove_folders(int dfd,int flags){
	struct dirent*ent;
	DIR*dir;
	if(!(dir=fdopendir(dfd)))return -1;
	while((ent=readdir(dir)))
		if(!is_virt_dir(ent)&&ent->d_type==DT_DIR)
			unlinkat(dfd,ent->d_name,flags);
	free(dir);
	return 0;
}

int has_block(char*block){
	char*p=block;
	if(
		block[0]!='/'&&
		!(p=blkid_evaluate_tag(block,NULL,NULL))
	)return false;
	struct stat st;
	if(stat(p,&st)<0)return errno==ENOENT?false:-1;
	if(!S_ISBLK(st.st_mode)){
		errno=ENOTBLK;
		return -1;
	}
	free(p);
	return true;
}

int wait_block(char*block,long time,char*tag){
	bool msg=false;
	long t=0;
	while(time==0||t++<time)switch(has_block(block)){
		case false:
			if(tag&&!msg){
				char x[128]={0};
				if(time!=0)snprintf(x,127," %ld seconds",time);
				log_notice(tag,"wait for block %s%s",tag,x);
				msg=true;
			}
			sleep(1);
		continue;
		case true:return 0;
		case -1:return -errno;
	}
	if(tag)log_error(tag,"wait for block %s timed out",tag);
	ERET(ETIMEDOUT);
}

static ssize_t _fd_read_file(int at,char*buff,size_t len,bool lf,char*path,va_list va){
	int fd;
	char rpath[PATH_MAX]={0};
	if(!path||!buff||len<=0)ERET(EINVAL);
	vsnprintf(rpath,PATH_MAX-1,path,va);
	memset(buff,0,len);
	if((fd=openat(at,rpath,O_RDONLY))<0)return -1;
	ssize_t s=read(fd,buff,len-1);
	if(s>0&&!lf){
		if(buff[s-1]=='\n')buff[--s]=0;
		if(buff[s-1]=='\r')buff[--s]=0;
	}
	close(fd);
	return s;
}

ssize_t fd_read_file(int at,char*buff,size_t len,bool lf,char*path,...){
	va_list va;
	va_start(va,path);
	int r=_fd_read_file(at,buff,len,lf,path,va);
	va_end(va);
	return r;
}

ssize_t read_file(char*buff,size_t len,bool lf,char*path,...){
	va_list va;
	va_start(va,path);
	int r=_fd_read_file(AT_FDCWD,buff,len,lf,path,va);
	va_end(va);
	return r;
}

bool fd_is_type(int fd,int err,mode_t type,const char*path,...){
	char rpath[PATH_MAX]={0};
	va_list va;
	va_start(va,path);
	vsnprintf(rpath,PATH_MAX-1,path,va);
	va_end(va);
	struct stat st;
	errno=EIO;
	if(fstatat(fd,rpath,&st,AT_SYMLINK_NOFOLLOW)<0)return false;
	errno=0;
	bool ret=(((st.st_mode)&S_IFMT)==type);
	if(errno==0&&err!=0&&!ret)errno=err;
	return ret;
}

int fd_read_int(int fd,char*name){
	errno=0;
	if(!name)ERET(EINVAL);
	if(fd<0)ERET(EBADF);
	int o=openat(fd,name,O_RDONLY);
	if(o<0)return -1;
	char buff[64]={0};
	if(read(o,buff,63)<0){
		close(o);
		return -1;
	}
	close(o);
	return parse_int(buff,-1);
}

int fd_write_int(int fd,char*name,int value,bool lf){
	errno=0;
	if(!name)ERET(EINVAL);
	if(fd<0)ERET(EBADF);
	int o=openat(fd,name,O_WRONLY);
	if(o<0)return -1;
	if(dprintf(o,"%d%s",value,lf?"\n":"")<0){
		close(o);
		return -1;
	}
	close(o);
	return 0;
}

int loop_get_free_num(void){
	int fd,avail=-1;
	if((fd=open(_PATH_DEV"/loop-control",O_RDWR|O_CLOEXEC))>=0){
		avail=ioctl(fd,LOOP_CTL_GET_FREE);
		close(fd);
	}
	return avail;
}

int loop_get_free(char*buf,size_t len){
	struct stat st;
	int avail;
	if(!buf||len<=0)ERET(EINVAL);
	if((avail=loop_get_free_num())<0)return -1;
	memset(buf,0,len);
	snprintf(buf,len-1,_PATH_DEV"/loop%d",avail);
	mknod(buf,S_IFBLK|S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP,makedev(7,avail));
	if(stat(buf,&st)!=0)goto fail;
	if(!S_ISBLK(st.st_mode)){
		errno=-ENOTBLK;
		goto fail;
	}
	return 0;
	fail:
	memset(buf,0,len);
	return -1;
}
