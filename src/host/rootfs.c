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
#include<unistd.h>
#include<string.h>
#include<stdarg.h>
#include<stdlib.h>
#include<stdbool.h>
#include<sys/uio.h>
#include<sys/stat.h>
#include<sys/time.h>
#include<sys/mman.h>

char source[PATH_MAX],binary[PATH_MAX],*folder;
static int dfd,ofd,bfd;

static void print_info(struct stat*st,int depth);
static void print_file(int cfd,char*name,size_t size,int depth);
static void print_folder(int cfd,char*name,int depth);
static void print_entity(int fd,char*name,int depth);

static void on_failure(){
	close(ofd);
	close(dfd);
	close(bfd);
	unlink(source);
	unlink(binary);
	exit(1);
}

static void add_line_indent(int len,const char*line,...){
	char x[BUFSIZ*4]={0},*p=x;
	memset(x,0,sizeof(x));
	if(len>BUFSIZ)abort();
	for(int c=0;c<len;c++)*p++='\t';
	if(line){
		va_list l;
		va_start(l,line);
		p+=vsnprintf(p,sizeof(x)-len,line,l);
		va_end(l);
	}
	p+=0;
	write(ofd,x,strlen(x));
}

#define add_line(line...) add_line_indent(depth,line)
#define add_str_val(name,value) add_line("%s=\"%s\",\n",(name),(value))
#define add_int_val(name,value) add_line("%s=%d,\n",(name),(value))

static void print_info(struct stat*st,int depth){
	add_line(".info.parent=NULL,\n");
	add_int_val(".info.mode",          st->st_mode);
	add_int_val(".info.owner",         st->st_uid);
	add_int_val(".info.group",         st->st_gid);
	add_int_val(".info.atime.tv_sec",  st->st_atim.tv_sec);
	add_int_val(".info.atime.tv_nsec", st->st_atim.tv_nsec);
	add_int_val(".info.mtime.tv_sec",  st->st_mtim.tv_sec);
	add_int_val(".info.mtime.tv_nsec", st->st_mtim.tv_nsec);
}

static void print_folder(int cfd,char*name,int depth){
	int fd=name?openat(cfd,name,O_RDONLY|O_DIRECTORY):cfd;
	if(fd<0){
		perror("open failed");
		on_failure();
		return;
	}
	DIR*d=fdopendir(fd);
	if(!d){
		perror("fdopendir failed");
		on_failure();
		return;
	}
	struct dirent*e;
	add_line(".subdirs=(entry_dir*[]){\n");
	depth++;
	while((e=readdir(d))){
		if(
			e->d_type!=DT_DIR||
			strcmp(e->d_name,".")==0||
			strcmp(e->d_name,"..")==0
		)continue;
		print_entity(fd,e->d_name,depth);
	}
	add_line("NULL\n");
	depth--;
	seekdir(d,0);
	add_line("},\n");
	add_line(".subfiles=(entry_file*[]){\n");
	depth++;
	while((e=readdir(d))){
		if(
			e->d_type==DT_DIR||
			strncmp(e->d_name,".git",4)==0
		)continue;
		print_entity(fd,e->d_name,depth);
	}
	add_line("NULL\n");
	depth--;
	add_line("},\n");
	closedir(d);
}

static void print_file(int cfd,char*name,size_t size,int depth){
	int fd=name?openat(cfd,name,O_RDONLY):cfd;
	if(fd<0){
		perror("open failed");
		on_failure();
		return;
	}
	add_int_val(".length",size);
	add_int_val(".offset",lseek(bfd,0,SEEK_CUR));
	add_line(".content=NULL,\n");
	if(size>0){
		void*v=mmap(NULL,size,PROT_READ,MAP_PRIVATE,fd,0);
		if(!v){
			perror("mmap failed");
			on_failure();
			return;
		}
		ssize_t x=write(bfd,v,size);
		munmap(v,size);
		if((size_t)x!=size){
			fprintf(stderr,"write binary size mismatch %zu != %zu: %m\n",x,size);
			on_failure();
			return;
		}
		write(bfd,(char[]){0,0},1);
	}
	close(fd);
}

static void print_entity(int fd,char*name,int depth){
	struct stat st;
	if(name?fstatat(fd,name,&st,AT_SYMLINK_NOFOLLOW):fstat(fd,&st)<0){
		perror("fstat failed");
		on_failure();
		return;
	}
	if(name){
		add_line("&(entry_%s){\n",S_ISDIR(st.st_mode)?"dir":"file");
		depth++;
		add_str_val(".info.name",name);
	}else depth++;
	print_info(&st,depth);
	switch(st.st_mode&S_IFMT){
		case S_IFLNK:{
			if(!name)break;
			char buf[PATH_MAX]={0};
			if(readlinkat(fd,name,buf,PATH_MAX-1)<0){
				perror("readlink failed");
				on_failure();
				return;
			}
			add_str_val(".content",buf);
		}break;
		case S_IFBLK:
		case S_IFCHR:
		case S_IFIFO:
		case S_IFSOCK:add_int_val(".dev",st.st_rdev);break;
		case S_IFDIR:print_folder(fd,name,depth);break;
		case S_IFREG:print_file(fd,name,st.st_size,depth);break;
	}
	if(name){
		depth--;
		add_line("},\n");
	}
}
int main(int argc,char**argv){
	if(argc!=4){
		fputs("Usage: assets <FOLDER> <SOURCE_DIR> <VARIABLE>\n",stderr);
		return 1;
	}
	folder=argv[1];
	snprintf(source,PATH_MAX-1,"%s/rootfs.c",argv[2]);
	snprintf(binary,PATH_MAX-1,"%s/rootfs.bin",argv[2]);
	if((dfd=open(folder,O_RDONLY|O_DIRECTORY))<0){
		perror("open folder");
		return -1;
	}
	if((ofd=open(source,O_WRONLY|O_TRUNC|O_CREAT,0644))<0){
		perror("open source");
		return -1;
	}
	if((bfd=open(binary,O_WRONLY|O_TRUNC|O_CREAT,0644))<0){
		perror("open binary");
		return -1;
	}
	dprintf(ofd,"#include<stddef.h>\n");
	dprintf(ofd,"#include<sys/stat.h>\n");
	dprintf(ofd,"#include\"assets.h\"\n");
	dprintf(ofd,"entry_dir %s={\n",argv[3]);
	print_entity(dfd,NULL,0);
	dprintf(ofd,"};\n");
	close(ofd);
	return 0;
}
