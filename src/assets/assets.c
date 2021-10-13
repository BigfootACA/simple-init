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

extern char _binary_rootfs_bin_start;

#ifndef ENABLE_UEFI
int set_assets_file_info(int fd,entry_file*file){
	int e=0;
	if(!file||fd<0)ERET(EINVAL);
	e+=fchown(fd,file->info.owner,file->info.group);
	if(file->info.mode>0)e+=fchmod(fd,file->info.mode);
	return e;
}

int write_assets_file(int fd,entry_file*file,bool pres){
	if(!file->content&&file->length>0)file->content=&_binary_rootfs_bin_start+file->offset;
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
			if((fd=openat(dfd,file->info.name,O_WRONLY|O_CREAT))<0)return -errno;
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

static list*resolve_relative_path(list*root,list*dir,char*path){
	if(!path||!dir)EPRET(EINVAL);
	if(!path[0])EPRET(ENOENT);
	list*o,*z;
	if(!(z=o=path2list(path,false)))EPRET(ENOENT);
	if(dir->prev&&path[0]!='/'){
		if(!(z=list_duplicate_chars(root,dir))){
			int e=errno;
			list_free_all_def(o);
			EPRET(e);
		}
		if(!list_merge(z,o)){
			int e=errno;
			list_free_all_def(o);
			list_free_all_def(z);
			EPRET(e);
		}
	}
	if(!(o=path_simplify(z,true))){
		int e=errno;
		list_free_all_def(z);
		EPRET(e);
	}
	return o;
}

static entry_dir*get_assets_dir_component(entry_dir*dir,list*paths,int*cnt){
	if(!dir||!cnt)EPRET(EINVAL);
	list*p=list_first(paths);
	entry_dir*cd=dir;
	do{
		LIST_DATA_DECLARE(n,p,char*);
		entry_dir*c=_get_assets_subdir(cd,n);
		if(!c){
			entry_file*l=_get_assets_subfile(cd,n);
			if(!l)EPRET(ENOENT);
			if(S_ISLNK(l->info.mode)){
				if((*cnt)++>=40)EPRET(ELOOP);
				list*o=resolve_relative_path(paths,p,l->content);
				if(!o)return NULL;
				c=get_assets_dir_component(dir,o,cnt);
				int e=errno;
				list_free_all_def(o);
				if(!c)EPRET(e);
			}else EPRET(ENOTDIR);
		}
		cd=c;
	}while((p=p->next));
	return cd;
}

static entry_file*get_assets_file_component(entry_dir*dir,list*paths,int*cnt){
	list*f=list_first(paths),*l=list_last(paths),*x;
	if(!cnt||!dir||!f||!l)EPRET(EINVAL);
	entry_file*r;
	entry_dir*d=dir;
	if(f->next){
		if(!(x=l->prev))EPRET(EFAULT);
		x->next=NULL;
		d=get_assets_dir_component(dir,f,cnt);
		int e=errno;
		x->next=l;
		if(!d)EPRET(e);
	}else if(f!=l||f->prev)EPRET(EFAULT);
	if(!(r=_get_assets_subfile(d,(char*)l->data)))return NULL;
	if(S_ISLNK(r->info.mode)){
		if((*cnt)++>=40)EPRET(ELOOP);
		list*o=resolve_relative_path(paths,l,r->content);
		if(!o)return NULL;
		r=get_assets_file_component(dir,o,cnt);
		int e=errno;
		list_free_all_def(o);
		if(!r)EPRET(e);
	}
	if(!r->content&&r->length>0)r->content=&_binary_rootfs_bin_start+r->offset;
	return r;
}

entry_dir*get_assets_dir(entry_dir*dir,char*path){
	list*paths=path2list(path,true);
	if(!paths)return NULL;
	int c=0;
	entry_dir*f=get_assets_dir_component(dir,paths,&c);
	int e=errno;
	list_free_all_def(paths);
	errno=e;
	return f;
}

entry_file*get_assets_file(entry_dir*dir,char*path){
	list*paths=path2list(path,true);
	if(!paths)return NULL;
	int c=0;
	entry_file*f=get_assets_file_component(dir,paths,&c);
	int e=errno;
	list_free_all_def(paths);
	errno=e;
	return f;
}
