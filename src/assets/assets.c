/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdbool.h>
#include<unistd.h>
#include<errno.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#define TAG "assets"
#include"str.h"
#include"assets.h"
#include"defines.h"

static bool rootfs_inited=false;
extern char _binary_rootfs_bin_start;

static void fill_assets_info(entry_dir*dir){
	entry_dir*d=NULL;
	entry_file*f=NULL;
	if(!dir){
		if(rootfs_inited)return;
		fill_assets_info(&assets_rootfs);
		rootfs_inited=true;
		return;
	}
	if(dir->subdirs)for(size_t s=0;(d=dir->subdirs[s]);s++){
		d->info.parent=dir;
		fill_assets_info(d);
	}
	if(dir->subfiles)for(size_t s=0;(f=dir->subfiles[s]);s++){
		f->info.parent=dir;
		if(!f->content&&f->length>0)
			f->content=&_binary_rootfs_bin_start+f->offset;
	}

}

#ifndef ENABLE_UEFI
int set_assets_file_info(int fd,entry_file*file){
	int e=0;
	if(!file||fd<0)ERET(EINVAL);
	e+=fchown(fd,file->info.owner,file->info.group);
	if(file->info.mode>0)e+=fchmod(fd,file->info.mode);
	return e;
}

int write_assets_file(int fd,entry_file*file,bool pres){
	fill_assets_info(NULL);
	if(file->content){
		if(file->length==0)file->length=strlen(file->content);
		if(write(fd,file->content,file->length)<0)return -errno;
		fsync(fd);
	}
	return pres?set_assets_file_info(fd,file):0;
}

static bool is_zero_time(struct timespec*t){return !(t&&t->tv_nsec!=0&&t->tv_sec!=0);}

int create_assets_file(int dfd,entry_file*file,bool pres,bool override){
	if(dfd<0||!file||!file->info.name[0])ERET(EINVAL);
	int fd,r;
	if(faccessat(dfd,file->info.name,F_OK,0)==0&&!override)return 0;
	mode_t mode=file->info.mode,type=mode&S_IFMT;
	switch(type){
		case S_IFBLK:
		case S_IFCHR:
		case S_IFIFO:
		case S_IFSOCK:
			r=mknodat(dfd,file->info.name,mode,file->dev);
		break;
		case S_IFLNK:
			if(!file->content)ERET(EINVAL);
			r=symlinkat(file->content,dfd,file->info.name);
		break;
		case S_IFREG:
			if((fd=openat(
				dfd,
				file->info.name,
				O_WRONLY|O_CREAT,
				file->info.mode
			))<0)return -errno;
			r=write_assets_file(fd,file,pres);
			close(fd);
		break;
		default:r=ENUM(ENOTSUP);
	}
	if(r<0)return r;
	struct timespec t[2]={file->info.atime,file->info.mtime};
	if(!is_zero_time(&t[0])||!is_zero_time(&t[1])){
		if(is_zero_time(&t[0]))t[0]=t[1];
		if(is_zero_time(&t[1]))t[1]=t[0];
		utimensat(dfd,file->info.name,t,0);
	}
	return 0;
}

int create_assets_dir(int dfd,entry_dir*dir,bool override){
	if(!dir||dfd<0)ERET(EINVAL);
	int fd=dfd,r=0;
	if(dir->info.name[0]){
		if(mkdirat(dfd,dir->info.name,dir->info.mode)<0&&errno!=EEXIST)return -errno;
		if((fd=openat(dfd,dir->info.name,O_RDONLY|O_DIRECTORY))<0)return -errno;
		struct timespec t[2]={dir->info.atime,dir->info.mtime};
		if(!is_zero_time(&t[0])||!is_zero_time(&t[1])){
			if(is_zero_time(&t[0]))t[0]=t[1];
			if(is_zero_time(&t[1]))t[1]=t[0];
			utimensat(dfd,dir->info.name,t,0);
		}
	}

	if(dir->subdirs)for(size_t s=0;dir->subdirs[s];s++)
		r+=create_assets_dir(fd,dir->subdirs[s],override);

	if(dir->subfiles)for(size_t s=0;dir->subfiles[s];s++)
		r+=create_assets_file(fd,dir->subfiles[s],true,override);

	fchown(fd,dir->info.owner,dir->info.group);
	if(dir->info.mode>0)fchmod(fd,dir->info.mode);
	close(fd);
	return r;
}
#endif

static entry_file*_get_assets_subfile(entry_dir*dir,const char*name){
	if(!dir->subfiles)EPRET(ENOENT);
	entry_file*f;
	for(size_t s=0;(f=dir->subfiles[s]);s++)
		if(strcmp(f->info.name,name)==0)return f;
	EPRET(ENOENT);
}

static entry_dir*_get_assets_subdir(entry_dir*dir,const char*name){
	if(!dir->subdirs)EPRET(ENOENT);
	entry_dir *d;
	for(size_t s=0;(d=dir->subdirs[s]);s++)
		if(strcmp(d->info.name,name)==0)return d;
	EPRET(ENOENT);
}

entry_dir*get_assets_dir(entry_dir*dir,const char*path){
	int cnt=0;
	entry_file*f=NULL;
	entry_dir*d=dir,*x;
	if(!dir||!path)return NULL;
	char*p=strdup(path),*xp=p,*n=NULL;
	if(!p)return NULL;
	fill_assets_info(NULL);
	for(size_t i=0;p[i];i++)if(p[i]=='\\')p[i]='/';
	if(path[0]=='/')
		while(d->info.parent)
			d=d->info.parent;
	do{
		if(n)xp=n+1;
		if((n=strchr(xp,'/')))*n=0;
		if(!*xp)continue;
		if(strcmp(xp,".")==0)continue;
		if(strcmp(xp,"..")==0){
			x=d->info.parent;
			if(x)d=x;
			continue;
		}
		if((x=_get_assets_subdir(d,xp))){d=x;continue;}
		if(!(f=_get_assets_subfile(d,xp))){d=NULL,errno=ENOENT;break;}
		if(!S_ISLNK(f->info.mode)){d=NULL,errno=ENOTDIR;break;}
		if(cnt++>=40){d=NULL,errno=ELOOP;break;}
		if(!f->content){d=NULL,errno=ENOENT;break;}
		if(f->content[0]=='/')while(d->info.parent)d=d->info.parent;
		if(!(d=get_assets_dir(d,f->content)))break;
	}while(n);
	free(p);
	if(d)errno=0;
	return d;
}

entry_file*get_assets_file(entry_dir*dir,const char*path){
	int cnt=0;
	entry_file*f=NULL;
	if(!dir||!path)return NULL;
	char*p=strdup(path),*xp=p,*n=NULL;
	if(!p)return NULL;
	fill_assets_info(NULL);
	for(size_t i=0;p[i];i++)if(p[i]=='\\')p[i]='/';
	if((n=strrchr(xp,'/'))){
		*n=0,dir=get_assets_dir(dir,xp);
		if(dir)xp=n+1;
	}
	if(!*xp)errno=ENOTDIR;
	else if(dir){
		f=_get_assets_subfile(dir,xp);
		while(f&&S_ISLNK(f->info.mode)){
			if(cnt++>=40){f=NULL,errno=ELOOP;break;}
			if(!f->content){f=NULL,errno=ENOENT;break;}
			if(f->content[0]=='/')
				while(dir->info.parent)
					dir=dir->info.parent;
			f=get_assets_file(dir,f->content);
		}
	}else errno=ENOENT;
	free(p);
	if(f)errno=0;
	return f;
}

bool asset_dir_check_out_bound(entry_dir*bound,entry_dir*target){
	bool out_of_bound=true;
	if(!bound||!target)return true;
	for(entry_dir*p=target;p;p=p->info.parent)
		if(p==bound)out_of_bound=false;
	return out_of_bound;
}

bool asset_file_check_out_bound(entry_dir*bound,entry_file*target){
	return target?asset_dir_check_out_bound(bound,target->info.parent):true;
}

entry_dir*asset_dir_get_root(entry_dir*target){
	if(!target)return NULL;
	while(target->info.parent)
		target=target->info.parent;
	return target;
}

entry_dir*asset_file_get_root(entry_file*target){
	return target?asset_dir_get_root(target->info.parent):NULL;
}

list*asset_dir_get_path_list(entry_dir*target,entry_dir*end){
	list*l=NULL;
	if(!target)return NULL;
	while(target->info.parent&&target->info.parent!=end){
		list_obj_add_new_strdup(&l,target->info.name);
		target=target->info.parent;
	}
	list_reverse(l);
	return list_first(l);
}

list*asset_file_get_path_list(entry_file*target,entry_dir*end){
	if(!target)return NULL;
	list*l=asset_dir_get_path_list(target->info.parent,end);
	list_obj_add_new_strdup(&l,target->info.name);
	return list_first(l);
}

char*asset_dir_get_path(entry_dir*target,entry_dir*end,char*buffer,size_t len){
	list*l=NULL;
	if(!target||!buffer||len<=0)return NULL;
	memset(buffer,0,len);
	if(!(l=asset_dir_get_path_list(target,end)))return NULL;
	char*r=list_string_append(l,buffer,len,"/");
	size_t s=strlen(buffer);
	if(buffer[s-1]!='/')strlcat(buffer,"/",len);
	return r;
}

char*asset_file_get_path(entry_file*target,entry_dir*end,char*buffer,size_t len){
	list*l=NULL;
	if(!target||!buffer||len<=0)return NULL;
	memset(buffer,0,len);
	if(!(l=asset_file_get_path_list(target,end)))return NULL;
	return list_string_append(l,buffer,len,"/");
}
