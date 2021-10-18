/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * based on util-linux
 *
 */

#define _GNU_SOURCE
#include<string.h>
#include<stdlib.h>
#include<libintl.h>
#include<sys/stat.h>
#include<sys/sysmacros.h>
#include<libmount/libmount.h>
#include"pathnames.h"
#include"defines.h"
#include"output.h"
#include"getopt.h"

struct mountpoint_control {
	char *path;
	dev_t dev;
	struct stat st;
	bool
		dev_devno,
		fs_devno,
		nofollow,
		quiet;
};

static int dir_to_device(struct mountpoint_control *ctl){
	struct libmnt_table*tb=mnt_new_table_from_file(_PATH_PROC_MOUNTINFO);
	struct libmnt_fs*fs;
	struct libmnt_cache*cache;
	int rc=-1;
	if(!tb){
		struct stat pst;
		char buf[PATH_MAX],*cn;
		int len;
		cn=mnt_resolve_path(ctl->path,NULL);
		len=snprintf(buf,sizeof(buf),"%s/..",cn?cn:ctl->path);
		free(cn);
		if(len<0||(size_t)len>=sizeof(buf))return -1;
		if(stat(buf,&pst)!=0)return -1;
		if(ctl->st.st_dev!=pst.st_dev||ctl->st.st_ino==pst.st_ino){
			ctl->dev=ctl->st.st_dev;
			return 0;
		}
		return -1;
	}
	cache=mnt_new_cache();
	mnt_table_set_cache(tb,cache);
	mnt_unref_cache(cache);
	fs=mnt_table_find_target(tb,ctl->path,MNT_ITER_BACKWARD);
	if(fs&&mnt_fs_get_target(fs))ctl->dev=mnt_fs_get_devno(fs),rc=0;
	mnt_unref_table(tb);
	return rc;
}

static int print_devno(const struct mountpoint_control*ctl){
	if(!S_ISBLK(ctl->st.st_mode)) {
		if(!ctl->quiet)printf(_("%s: not a block device"),ctl->path);
		return -1;
	}
	printf("%u:%u\n",major(ctl->st.st_rdev),minor(ctl->st.st_rdev));
	return 0;
}

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage:\n"
		"\tmountpoint [-qd] /path/to/directory\n"
		"\tmountpoint -x /dev/device\n"
		"Check whether a directory or file is a mountpoint.\n\n"
		"Options:\n"
		"\t-q, --quiet        quiet mode - don't print anything\n"
		"\t-n, --nofollow     do not follow symlink\n"
		"\t-d, --fs-devno     print maj:min device number of the filesystem\n"
		"\t-x, --devno        print maj:min device number of the block device\n"
	);
}

int mountpoint_main(int argc,char**argv){
	int c;
	struct mountpoint_control ctl;
	static const struct option lo[]={
		{"quiet",    no_argument,NULL,'q'},
		{"nofollow", no_argument,NULL,'n'},
		{"fs-devno", no_argument,NULL,'d'},
		{"devno",    no_argument,NULL,'x'},
		{"help",     no_argument,NULL,'h'},
		{NULL,0,NULL,0}
	};
	memset(&ctl,0,sizeof(struct mountpoint_control));
	while((c=b_getlopt(argc,argv,"qdxhn",lo,0))>=0)switch(c){
		case 'q':ctl.quiet=true;break;
		case 'n':ctl.nofollow=true;break;
		case 'd':ctl.fs_devno=true;break;
		case 'x':ctl.dev_devno=true;break;
		case 'h':return usage(0);
		default:return 1;
	}
	if(b_optind+1!=argc)return re_printf(1,"mountpoint: bad usage\n");
	if(ctl.nofollow&&ctl.dev_devno)return re_printf(1,"mountpoint: -x and -n are mutually exclusive\n");
	ctl.path=argv[b_optind];
	if(ctl.nofollow?lstat(ctl.path,&ctl.st):stat(ctl.path,&ctl.st)){
		if(!ctl.quiet)return re_err(1,"%s",ctl.path);
		return 1;
	}
	if(ctl.dev_devno)return print_devno(&ctl)?1:0;
	if((ctl.nofollow&&S_ISLNK(ctl.st.st_mode))||dir_to_device(&ctl)){
		if(!ctl.quiet)printf(_("%s is not a mountpoint\n"),ctl.path);
		return 1;
	}
	if(ctl.fs_devno)printf("%u:%u\n",major(ctl.dev),minor(ctl.dev));
	else if(!ctl.quiet)printf(_("%s is a mountpoint\n"),ctl.path);
	return 0;
}
