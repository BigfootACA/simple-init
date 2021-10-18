/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * based on coreutils, busybox
 *
 */

#define _GNU_SOURCE
#include<fcntl.h>
#include<stdio.h>
#include<errno.h>
#include<string.h>
#include<dirent.h>
#include<sys/stat.h>
#include<sys/sysmacros.h>
#include<stdlib.h>
#include"str.h"
#include"getopt.h"
#include"output.h"
#include"defines.h"
#include"system.h"

#define progname program_invocation_short_name
#define TYPEINDEX(mode) (((mode)>>12)&0x0F)
#define COLOR(mode) ("\037\043\043\045\042\045\043\043\000\045\044\045\043\045\045\040"[TYPEINDEX(mode)])
#define ATTR(mode) ("\01\00\01\07\01\07\01\07\00\07\01\07\01\07\07\01"[TYPEINDEX(mode)])
#define APPCHAR(mode) ("\0""|""\0""\0""/""\0""\0""\0""\0""\0""@""\0""=""\0""\0""\0"[TYPEINDEX(mode)])

static struct{
	bool list,all,directory,inode,type;
	bool numeric,size,nogroup,human;
	bool fulltime,atime,ctime,color;
	int perline;
	time_t curtime;
	char whencolor[32];
}opts;

static char fgcolor(mode_t mode){
	return COLOR(
		(S_ISREG(mode)&&(mode&(S_IXUSR|S_IXGRP|S_IXOTH)))?
		0xF000:
		mode
	);
}

static char bold(mode_t mode){
	return ATTR(
		(S_ISREG(mode)&&(mode&(S_IXUSR|S_IXGRP|S_IXOTH)))?
		0xF000:
		mode
	);
}

static char append_char(mode_t mode){
	if(!opts.type)return '\0';
	if(S_ISDIR(mode))return '/';
	if(S_ISREG(mode)&&(mode&(S_IXUSR|S_IXGRP|S_IXOTH)))return '*';
	return APPCHAR(mode);
}

static void display_single(int fd,int dfd,char*op,char*name){
	struct stat st;
	char lpath[PATH_MAX]={0},nb[128]={0};
	if(fstatat(dfd,name,&st,AT_SYMLINK_NOFOLLOW)>=0){
		if(
			S_ISLNK(st.st_mode)&&
			readlinkat(dfd,name,lpath,PATH_MAX-1)<0
		)stderr_perror("%s: cannot readlink %s%s%s",progname,op?op:"",op?"/":"",name);
		if(opts.inode)dprintf(fd,"%7llu ",(long long)st.st_ino);
		if(opts.size)dprintf(fd,"%6llu ",(long long)(st.st_blocks>>1));
		if(opts.list){
			dprintf(fd,"%-10s ",(char*)mode_string(st.st_mode));
			dprintf(fd,"%4lu ",(long)st.st_nlink);
			if(opts.numeric){
				dprintf(fd,"%-8u ",(int)st.st_uid);
				if(!opts.nogroup)dprintf(
					fd,"%-8u ",
					(int)st.st_gid
				);
			}else{
				dprintf(fd,"%-8.8s ",get_username(st.st_uid,nb,128));
				if(!opts.nogroup)dprintf(
					fd,"%-8.8s ",
					get_groupname(st.st_gid,nb,128)
				);
			}
			if(
				S_ISBLK(st.st_mode)||
				S_ISCHR(st.st_mode)
			)dprintf(fd,"%4u,%4u ",major(st.st_rdev),minor(st.st_rdev));
			else if (opts.human)dprintf(fd,"%9s ",make_readable_str(st.st_size,1,0));
			else dprintf(fd,"%9llu ",(unsigned long long)st.st_size);
			time_t stime=st.st_mtime;
			if(opts.atime)stime=st.st_atime;
			if(opts.ctime)stime=st.st_ctime;
			if(opts.fulltime){
				char buf[32]={0};
				strftime(buf,31,"%Y-%m-%d %H:%M:%S %z",localtime(&stime));
				dprintf(fd,"%s ",buf);
			}else{
				char*ft=ctime(&stime);
				time_t age=opts.curtime-stime;
				if(age<3600L*24*365/2&&age>-15*60)dprintf(fd,"%.12s ",ft+4);
				else{
					strchr(ft+20,'\n')[0]=' ';
					dprintf(fd,"%.7s%6s",ft+4,ft+20);
				}
			}
		}
		if(opts.color)dprintf(fd,"\033[%u;%um",bold(st.st_mode),fgcolor(st.st_mode));
		dprintf(fd,"%s",name);
		if(opts.color)dprintf(fd,"\033[m");
		if(opts.list){
			if(lpath[0])dprintf(fd," -> %s",lpath);
			if(opts.type)dprintf(fd,"%c",append_char(st.st_mode));
		}
	}else{
		stderr_perror("%s: cannot stat %s%s%s",progname,op?op:"",op?"/":"",name);
		dprintf(fd,"%s",name);
	}
	fsync(fd);
}

static int do_list(int fd,char*path){
	static char*op;
	op=path;
	if(!path)op=".";
	size_t ss=strlen(op);
	if(ss==0)return EINVAL;
	if(ss-1>0&&op[ss-1]=='/')op[ss-1]=0;
	int dfd=open(op,O_RDONLY|O_DIRECTORY);
	if(dfd<0)return re_err(2,"ls: opening directory %s",op);
	DIR*d=fdopendir(dfd);
	if(!d){
		close(dfd);
		return re_err(2,"ls: reading directory %s",op);
	}
	size_t max=0,p;
	int s=MAX(get_term_width(fd,40),40),c=s,x=0;
	struct dirent*r=NULL;
	while((r=readdir(d)))if(r->d_name[0]!='.'||opts.all){
		max=MAX(max,strlen(r->d_name));
	}
	seekdir(d,0);
	while((r=readdir(d))){
		if(r->d_name[0]=='.'&&!opts.all)continue;
		display_single(fd,dfd,op,r->d_name);
		x++;
		if((x<opts.perline||opts.perline<=0)&&!opts.list){
			p=strlen(r->d_name),c-=p;
			p=max-p+2,c-=p;
			if(c>0)repeat(fd,' ',p);
			else{
				dprintf(fd,"\n");
				c=s,x=0;
			}
		}else dprintf(fd,"\n");
	}
	if(c!=s)dprintf(fd,"\n");
	closedir(d);
	return 0;
}

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: ls [OPTION]... [FILE]...\n"
		"List information about the FILEs (the current directory by default).\n\n"
		"Options:\n"
		"\t-l, --list            use a long listing format\n"
		"\t-a, --all             do not ignore entries starting with .\n"
		"\t-d, --directory       list directories themselves, not their contents\n"
		"\t-i, --inode           print the index number of each file\n"
		"\t-n, --numeric-uid-gid like -l, but list numeric user and group IDs\n"
		"\t-s, --size            print the allocated size of each file, in blocks\n"
		"\t-h, --human-readable  with -l and -s, print sizes like 1K 234M 2G etc.\n"
		"\t-e, --full-time       show full-iso format time\n"
		"\t-c, --ctime           show ctime (time of last modification)\n"
		"\t-u, --atime           show atime (time of last access)\n"
		"\t-g, --no-group        in a long listing, don't print group names\n"
		"\t-1, --oneline         list one file per line.\n"
		"\t-F, --classify        append indicator (one of */=>@|) to entries\n"
		"\t-C, --color[=WHEN]    colorize the output, WHEN can be always, auto, never, force\n"
		"\t-H, --help            display this help and exit\n"
	);
}

int ls_main(int argc,char**argv){
	const char*so="ladinshgeucFC1H";
	const struct option lo[]={
		{"list",           no_argument,      NULL,'l'},
		{"all",            no_argument,      NULL,'a'},
		{"directory",      no_argument,      NULL,'d'},
		{"inode",          no_argument,      NULL,'i'},
		{"numeric-uid-gid",no_argument,      NULL,'n'},
		{"size",           no_argument,      NULL,'s'},
		{"human-readable", no_argument,      NULL,'h'},
		{"full-time",      no_argument,      NULL,'e'},
		{"atime",          no_argument,      NULL,'u'},
		{"ctime",          no_argument,      NULL,'c'},
		{"nogroup",        no_argument,      NULL,'g'},
		{"color",          optional_argument,NULL,'C'},
		{"oneline",        no_argument,      NULL,'1'},
		{"classify",       no_argument,      NULL,'F'},
		{"help",           no_argument,      NULL,'H'},
		{NULL,0,NULL,0}
	};
	int o;
	strcpy(opts.whencolor,"auto");
	if(!isatty(STDOUT_FILENO))opts.perline=1;
	while((o=b_getlopt(argc,argv,so,lo,NULL))!=-1)switch(o){
		case 'l':opts.list=true;break;
		case 'a':opts.all=true;break;
		case 'd':opts.directory=true;break;
		case '1':opts.perline=1;break;
		case 'i':opts.inode=true;break;
		case 'n':opts.numeric=true;break;
		case 's':opts.size=true;break;
		case 'h':opts.human=true;break;
		case 'e':opts.fulltime=true;break;
		case 'u':opts.atime=true;break;
		case 'c':opts.ctime=true;break;
		case 'C':
			strcpy(opts.whencolor,"always");
			if(!b_optarg)break;
			if(*b_optarg=='=')b_optarg++;
			strncpy(opts.whencolor,b_optarg,31);
		break;
		case 'F':opts.type=true;break;
		case 'g':opts.list=opts.nogroup=true;break;
		case 'H':return usage(0);
		default:return 2;
	}
	opts.curtime=time(NULL);
	char*term=getenv("TERM");
	if(strcmp(opts.whencolor,"never")==0)opts.color=false;
	else if(strcmp(opts.whencolor,"force")==0)opts.color=true;
	else if(strcmp(opts.whencolor,"always")==0)opts.color=isatty(STDOUT_FILENO);
	else if(strcmp(opts.whencolor,"auto")==0)if(
		term&&(
		      strcmp(term,"linux")==0||
		      strncmp(term,"xterm",5)==0
		)&&isatty(STDOUT_FILENO)
	)opts.color=true;
	if(b_optind==argc)do_list(STDOUT_FILENO,NULL);
	else for(int i=b_optind;i<argc;i++)if(argv[i]){
		struct stat st;
		if(stat(argv[i],&st)<0)return re_err(2,"ls: stat %s",argv[i]);
		if(S_ISDIR(st.st_mode)){
			if(argc-b_optind>1)printf("%s:\n",argv[i]);
			do_list(STDOUT_FILENO,argv[i]);
			if(i<argc-1)putchar('\n');
		}else{
			display_single(STDOUT_FILENO,AT_FDCWD,NULL,argv[i]);
			putchar('\n');
		}
	}
	return 0;
}
