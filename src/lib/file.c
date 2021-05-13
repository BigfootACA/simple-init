#include<errno.h>
#include<string.h>
#include<stdlib.h>
#include<stdbool.h>
#include<dirent.h>
#include<libgen.h>
#include<unistd.h>
#include<sys/stat.h>
#include"system.h"
#include"defines.h"
#include"pathnames.h"

bool is_virt_dir(const struct dirent*d){
	return
		strcmp(d->d_name,".")==0||
		strcmp(d->d_name,"..")==0;
}

size_t get_fd_path(int fd,char*buff,size_t size){
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
	if((fd=open(file,O_WRONLY|O_SYNC|O_TRUNC))<0)return -1;
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
