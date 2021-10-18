/*
 *
 * Copyright 2008, The Android Open Source Project
 *
 */

#define _GNU_SOURCE
#include<utime.h>
#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include<dirent.h>
#include<sys/stat.h>
#include<sys/types.h>
#include"adbd_internal.h"
#include"logger.h"
#define TAG "adbd"
static int mkdirs(char*name){
	int ret;
	char*x=name + 1;
	if(name[0]!='/')return -1;
	for(;;){
		x=strchr(x,'/');
		if(x==0)return 0;
		*x=0;
		ret=mkdir(name,0755);
		if((ret<0)&&(errno!=EEXIST)){
			telog_warn("mkdir %s",name);
			*x='/';
			return ret;
		}
		*x++='/';
	}
}
static int do_stat(int s,const char*path){
	syncmsg msg;
	struct stat st;
	msg.stat.id=ID_STAT;
	if(lstat(path,&st)){
		msg.stat.mode=0;
		msg.stat.size=0;
		msg.stat.time=0;
	}else{
		msg.stat.mode=st.st_mode;
		msg.stat.size=st.st_size;
		msg.stat.time=st.st_mtime;
	}
	return writex(s,&msg.stat,sizeof(msg.stat));
}
static int do_list(int s,const char*path){
	DIR*d;
	struct dirent*de;
	struct stat st;
	syncmsg msg;
	int len;
	char tmp[1024+256+1];
	char*fname;
	len=strlen(path);
	memcpy(tmp,path,len);
	tmp[len]='/';
	fname=tmp+len+ 1;
	msg.dent.id=ID_DENT;
	d=opendir(path);
	if(d==0)goto done;
	while((de=readdir(d))){
		len=strlen(de->d_name);
		if(len>256)continue;
		strcpy(fname,de->d_name);
		if(lstat(tmp,&st)==0){
			msg.dent.mode=st.st_mode;
			msg.dent.size=st.st_size;
			msg.dent.time=st.st_mtime;
			msg.dent.namelen=len;
			if(writex(s,&msg.dent,sizeof(msg.dent))||writex(s,de->d_name,len))return -1;
		}
	}
	closedir(d);
done:
	msg.dent.id=ID_DONE;
	msg.dent.mode=0;
	msg.dent.size=0;
	msg.dent.time=0;
	msg.dent.namelen=0;
	return writex(s,&msg.dent,sizeof(msg.dent));
}
static int fail_message(int s,const char*reason){
	syncmsg msg;
	int len=strlen(reason);
	telog_warn("sync failure: %s",reason);
	msg.data.id=ID_FAIL;
	msg.data.size=len;
	return(writex(s,&msg.data,sizeof(msg.data))||writex(s,reason,len))?-1:0;
}
static int fail_errno(int s){return fail_message(s,strerror(errno));}
static int handle_send_file(int s,char*path,mode_t mode,char*buffer){
	syncmsg msg;
	unsigned int timestamp;
	int fd;
	fd=adb_open_mode(path,O_WRONLY|O_CREAT|O_EXCL,mode);
	if(fd < 0 && errno==ENOENT){
		mkdirs(path);
		fd=adb_open_mode(path,O_WRONLY|O_CREAT|O_EXCL,mode);
	}
	if(fd<0&&errno==EEXIST)fd=adb_open_mode(path,O_WRONLY,mode);
	if(fd<0){
		if(fail_errno(s))return -1;
		fd=-1;
	}
	for(;;){
		unsigned int len;
		if(readx(s,&msg.data,sizeof(msg.data)))goto fail;
		if(msg.data.id !=ID_DATA){
			if(msg.data.id==ID_DONE){
				timestamp=msg.data.size;
				break;
			}
			fail_message(s,"invalid data message");
			goto fail;
		}
		if((len=msg.data.size)>SYNC_DATA_MAX){
			fail_message(s,"oversize data message");
			goto fail;
		}
		if(readx(s,buffer,len))goto fail;
		if(fd<0)continue;
		if(writex(fd,buffer,len)){
			int saved_errno=errno;
			close(fd);
			unlink(path);
			fd=-1;
			errno=saved_errno;
			if(fail_errno(s))return -1;
		}
	}
	if(fd>=0){
		struct utimbuf u;
		close(fd);
		u.actime=timestamp;
		u.modtime=timestamp;
		utime(path,&u);
		msg.status.id=ID_OKAY;
		msg.status.msglen=0;
		if(writex(s,&msg.status,sizeof(msg.status)))return -1;
	}
	return 0;
fail:
	if(fd >=0)close(fd);
	unlink(path);
	return -1;
}
static int handle_send_link(int s,char*path,char*buffer){
	syncmsg msg;
	unsigned int len;
	int ret;
	if(readx(s,&msg.data,sizeof(msg.data)))return -1;
	if(msg.data.id !=ID_DATA){
		fail_message(s,"invalid data message: expected ID_DATA");
		return -1;
	}
	if((len=msg.data.size)>SYNC_DATA_MAX){
		fail_message(s,"oversize data message");
		return -1;
	}
	if(readx(s,buffer,len))return -1;
	if((ret=symlink(buffer,path))&&errno==ENOENT){
		mkdirs(path);
		ret=symlink(buffer,path);
	}
	if(ret){fail_errno(s);return -1;}
	if(readx(s,&msg.data,sizeof(msg.data)))return -1;
	if(msg.data.id==ID_DONE){
		msg.status.id=ID_OKAY;
		msg.status.msglen=0;
		if(writex(s,&msg.status,sizeof(msg.status)))return -1;
	}else{
		fail_message(s,"invalid data message: expected ID_DONE");
		return -1;
	}
	return 0;
}
static int do_send(int s,char*path,char*buffer){
	char*tmp;
	mode_t mode;
	int is_link,ret;
	tmp=strrchr(path,',');
	if(tmp){
		*tmp=0;
		errno=0;
		mode=strtoul(tmp+1,NULL,0);
		is_link=S_ISLNK(mode);
		mode&=0777;
	}
	if(!tmp||errno){mode=0644;is_link=0;}
	unlink(path);
	if(is_link)ret=handle_send_link(s,path,buffer);
	else{
		mode|=((mode>>3)&0070);
		mode|=((mode>>3)&0007);
		ret=handle_send_file(s,path,mode,buffer);
	}
	return ret;
}
static int do_recv(int s,const char*path,char*buffer){
	syncmsg msg;
	int fd,r;
	if((fd=adb_open(path,O_RDONLY))<0){
		if(fail_errno(s))return -1;
		return 0;
	}
	msg.data.id=ID_DATA;
	for(;;){
		if((r=adb_read(fd,buffer,SYNC_DATA_MAX))<=0){
			if(r==0)break;
			if(errno==EINTR)continue;
			r=fail_errno(s);
			close(fd);
			return r;
		}
		msg.data.size=r;
		if(writex(s,&msg.data,sizeof(msg.data))||writex(s,buffer,r)){
			close(fd);
			return -1;
		}
	}
	close(fd);
	msg.data.id=ID_DONE;
	msg.data.size=0;
	if(writex(s,&msg.data,sizeof(msg.data)))return -1;
	return 0;
}
void file_sync_service(int fd,void*cookie __attribute__((unused))){
	syncmsg msg;
	char name[1025];
	unsigned namelen;
	char*buffer=malloc(SYNC_DATA_MAX);
	if(buffer==0)goto fail;
	for(;;){
		if(readx(fd,&msg.req,sizeof(msg.req))){
			fail_message(fd,"command read failure");
			break;
		}
		namelen=msg.req.namelen;
		if(namelen>1024){
			fail_message(fd,"invalid namelen");
			break;
		}
		if(readx(fd,name,namelen)){
			fail_message(fd,"filename read failure");
			break;
		}
		name[namelen]=0;
		msg.req.namelen=0;
		telog_debug("file request %s %s",(char*)&msg.req,name);
		switch(msg.req.id){
			case ID_STAT:if(do_stat(fd,name))goto fail;break;
			case ID_LIST:if(do_list(fd,name))goto fail;break;
			case ID_SEND:if(do_send(fd,name,buffer))goto fail;break;
			case ID_RECV:if(do_recv(fd,name,buffer))goto fail;break;
			case ID_QUIT:goto fail;
			default:fail_message(fd,"unknown command");goto fail;
		}
	}
fail:
	if(buffer !=0)free(buffer);
	telog_info("sync done");
	close(fd);
}
