#include<stdbool.h>
#include<unistd.h>
#include<errno.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#define TAG "assets"
#include"assets.h"
#include"defines.h"

int set_assets_file_info(int fd,entry_file*file){
	int e=0;
	if(!file||fd<0)ERET(EINVAL);
	e+=fchown(fd,file->info.owner,file->info.group);
	if(file->info.mode>0)e+=fchmod(fd,file->info.mode);
	return e;
}

int write_assets_file(int fd,entry_file*file,bool pres){
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
	return 0;
}
