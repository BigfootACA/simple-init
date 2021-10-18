/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<stdio.h>
#include<fcntl.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<dirent.h>
#include<sys/stat.h>
#include<sys/mount.h>
#include<sys/statfs.h>
#include"init.h"
#include"logger.h"
#include"cmdline.h"
#include"defines.h"
#include"pathnames.h"
#define TAG "switchroot"
#define STATFS_TMPFS_MAGIC 0x01021994
#define STATFS_RAMFS_MAGIC 0x858458f6
#define F_TYPE_EQUAL(a,b) ((a)==(__typeof__(a))(b))

static int recursive_remove(int fd){
	struct stat rb;
	DIR *dir;
	int rc=-1;
	int dfd;
	if(!(dir=fdopendir(fd))){
		telog_error("failed to open directory");
		goto done;
	}
	if((dfd=dirfd(dir))<0||fstat(dfd,&rb)){
		telog_error("stat failed");
		goto done;
	}
	while(1){
		struct dirent*d;
		int isdir=0;
		errno=0;
		if(!(d=readdir(dir))){
			if(errno>0){
				telog_error("failed to read directory");
				goto done;
			}
			break;
		}
		if(!strcmp(d->d_name,".")||!strcmp(d->d_name,".."))continue;
		if(d->d_type==DT_DIR||d->d_type==DT_UNKNOWN){
			struct stat sb;
			if(fstatat(dfd,d->d_name,&sb,AT_SYMLINK_NOFOLLOW)){
				telog_error("stat of %s failed",d->d_name);
				continue;
			}
			if(sb.st_dev!=rb.st_dev)continue;
			if(S_ISDIR(sb.st_mode)){
				int cfd;
				if((cfd=openat(dfd,d->d_name,O_RDONLY))>=0){
					recursive_remove(cfd);
					close(cfd);
				}
				isdir=1;
			}
		}
		if(unlinkat(dfd,d->d_name,isdir?AT_REMOVEDIR:0))telog_error("failed to delete %s",d->d_name);
	}
	rc=0;
	done:
	if(dir)closedir(dir);
	return rc;
}

static int switchroot(const char*newroot){
	const char *umounts[]={_PATH_DEV,_PATH_PROC,_PATH_SYS,NULL};
	int i,cfd;
	pid_t pid;
	struct stat newroot_stat,sb;
	if(stat(newroot,&newroot_stat)!=0)return terlog_error(-1,"stat of %s failed",newroot);
	for(i=0;umounts[i]!=NULL;i++){
		char newmount[PATH_MAX];
		snprintf(newmount,sizeof(newmount),"%s%s",newroot,umounts[i]);
		if((stat(newmount,&sb)!=0)||(sb.st_dev!=newroot_stat.st_dev)){
			umount2(umounts[i],MNT_DETACH);
			continue;
		}
		if(mount(umounts[i],newmount,NULL,MS_MOVE,NULL)<0){
			telog_error("failed to mount moving %s to %s",umounts[i],newmount);
			tlog_error("forcing unmount of %s",umounts[i]);
			umount2(umounts[i],MNT_FORCE);
		}
	}
	if(chdir(newroot))return terlog_error(-1,"failed to change directory to %s",newroot);
	if((cfd=open(_PATH_ROOT,O_RDONLY))<0)return terlog_error(-1,"cannot open %s",_PATH_ROOT);
	if(mount(newroot,_PATH_ROOT,NULL,MS_MOVE,NULL)<0){
		close(cfd);
		return terlog_error(-1,"failed to mount moving %s to %s",newroot,_PATH_ROOT);
	}
	if(chroot(".")){
		close(cfd);
		return terlog_error(-1,"failed to change root");
	}
	if((pid=fork())<=0){
		struct statfs stfs;
		if(
			fstatfs(cfd,&stfs)==0&&(
				F_TYPE_EQUAL(stfs.f_type,STATFS_RAMFS_MAGIC)||
				F_TYPE_EQUAL(stfs.f_type,STATFS_TMPFS_MAGIC)
			)
		)recursive_remove(cfd);
		else telog_error("old root filesystem is not an initramfs");
		if(pid==0)exit(0);
	}
	close(cfd);
	return 0;
}

static int run_init(const char*init){
	if(access(init,X_OK)!=0)return terlog_emerg(-EINVAL,"failed to accsee init '%s'",init);
	if(execl(init,init,NULL)!=0)return terlog_emerg(-EINVAL,"failed to execute init '%s'",init);
	return 0;
}

int run_switch_root(char*root,char*init){
	int rc=0;
	if(!root||!init)ERET(EINVAL);
	struct stat buf;
	if(stat(root,&buf)!=0)return terlog_error(-EINVAL,"cannot stat rootfs");
	if(!S_ISDIR(buf.st_mode))return trlog_error(-EINVAL,"rootfs not a folder");
	tlog_alert("switch to new root %s and execute new init %s",root,init);
	init_do_exit();
	kill_all();
	if(switchroot(root)!=0)return -1;
	if(run_init(init)!=0){
		tlog_emerg("failed to found working init");
		rc=-1;
	}
	exit(rc);
}

bool check_init(bool force,char*root,char*init){
	static char path[PATH_MAX];
	struct stat st;
	if(!root||!init)return false;

	// init must start with /
	if(init[0]!='/'){
		tlog_error("invalid init %s",init);
		errno=EINVAL;
		return false;
	}

	// generate init realpath
	memset(path,0,PATH_MAX);
	snprintf(path,PATH_MAX-1,"%s%s",root,init);

	// stat init
	if(stat(path,&st)<0){
		if(errno!=ENOENT||force)telog_warn("stat init %s",path);
		return false;
	}

	// init is a normal file
	if(!S_ISREG(st.st_mode)){
		tlog_error("init %s is not a file",path);
		errno=ENOEXEC;
		return false;
	}

	// init has execute permission
	if(!(st.st_mode&S_IXUSR)){
		tlog_error("init %s is not an executable file",path);
		errno=EACCES;
		return false;
	}

	errno=0;
	return true;
}

char*search_init(char*init,char*root){
	if(!root)EPRET(EINVAL);

	// init specified in kernel cmdline
	if(init)return check_init(true,root,init)?init:NULL;

	// search init in list
	for(int i=0;(init=init_list[i]);i++){
		if(!check_init(false,root,init))continue;
		return init;
	}

	// no any init found
	tlog_error("no init found in %s",root);
	EPRET(ENOENT);
}
