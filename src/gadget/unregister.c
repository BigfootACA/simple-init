/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<unistd.h>
#include<fcntl.h>
#include<stdlib.h>
#include<string.h>
#include<sys/mount.h>
#include"system.h"
#include"gadget.h"

int gadget_unregister_all(){
	int o=open_usb_gadget();
	if(o<0)return o;
	int r=gadget_unregister_all_fd(o);
	close(o);
	return r;
}

int gadget_unregister(char*name){
	int o=open_usb_gadget();
	if(o<0)return o;
	int r=gadget_unregister_fd(o,name);
	close(o);
	return r;
}

int gadget_unregister_fd(int dir,char*name){
	int d=-1,cs=-1,fs=-1,ss=-1;
	if(!(
		dir>=0&&name&&
		(d=openat(dir,name,O_DIR|O_CLOEXEC))>=0&&
		(cs=openat(d,"configs",O_DIR|O_CLOEXEC))>=0&&
		(fs=openat(d,"functions",O_DIR|O_CLOEXEC))>=0&&
		(ss=openat(d,"strings",O_DIR|O_CLOEXEC))>=0
	))goto er;
	gadget_stop_fd(d);
	DIR*subdir,*cd;
	struct dirent*rent,*ent;
	if(!(subdir=fdopendir(cs)))goto er;
	while((rent=readdir(subdir)))if(!is_virt_dir(rent)&&rent->d_type==DT_DIR){
		int cfg,str;
		if((cfg=openat(cs,rent->d_name,O_DIR|O_CLOEXEC))<0)continue;
		if((cd=fdopendir(cfg))){
			while((ent=readdir(cd)))if(ent->d_type==DT_LNK)unlinkat(cfg,ent->d_name,0);
			free(cd);
		}
		if((str=openat(cfg,"strings",O_DIR|O_CLOEXEC))<0)continue;
		if(remove_folders(str,AT_REMOVEDIR)<0){
			close(str);
			close(cfg);
			continue;
		}
		close(str);
		close(cfg);
		unlinkat(cs,rent->d_name,AT_REMOVEDIR);
	}
	closedir(subdir);
	struct mount_item**ms=read_proc_mounts();
	if((cd=fdopendir(fs))){
		while((ent=readdir(cd)))if(!is_virt_dir(ent)&&ent->d_type==DT_DIR){
			char*e=ent->d_name,*x=strchr(e,'.');
			if(ms&&x&&strncmp(e,"ffs",x-e)==0)for(size_t i=0;ms[i];i++)
				if(strcmp(ms[i]->type,"functionfs")==0&&strcmp(ms[i]->source,x+1)==0)
					umount(ms[i]->target);
			unlinkat(fs,ent->d_name,AT_REMOVEDIR);
		}
		free(cd);
	}
	if(ms)free_mounts(ms);
	if(remove_folders(ss,AT_REMOVEDIR)<0)goto er;
	close(d);
	return unlinkat(dir,name,AT_REMOVEDIR);
	er:
	if(cs>=0)close(cs);
	if(fs>=0)close(fs);
	if(ss>=0)close(ss);
	if(d>=0)close(d);
	return -1;
}

int gadget_unregister_all_fd(int dir){
	DIR*f=NULL;
	if(!(dir>=0&&(f=fdopendir(dir))))return -1;
	struct dirent*x;
	while((x=readdir(f)))
		if(!is_virt_dir(x)&&x->d_type==DT_DIR)
			gadget_unregister_fd(dir,x->d_name);
	free(f);
	return 0;
}
