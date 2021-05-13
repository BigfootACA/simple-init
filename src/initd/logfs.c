#include<errno.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<blkid/blkid.h>
#include"str.h"
#include"logger.h"
#include"system.h"
#include"defines.h"
#include"cmdline.h"
#define TAG "logfs"
#define DEFAULT_LOGFS_FILE "simple-init.%N.log"
#define DEFAULT_LOGFS_BLOCK "PARTLABEL=logfs"
int setup_logfs(){
	int e=0;
	char*logfs=boot_options.logfs_block,*logfile=boot_options.logfs_file;

	if(logfile[0]==0)logfile=DEFAULT_LOGFS_FILE;
	if(logfs[0]==0)logfs=DEFAULT_LOGFS_BLOCK;
	if(logfs[0]!='/')logfs=blkid_evaluate_tag(logfs,NULL,NULL);
	if(!logfs)return tlog_warn("logfs found not found");

	char*type;
	blkid_cache cache=NULL;
	blkid_get_cache(&cache,NULL);
	if(!(type=blkid_get_tag_value(cache,"TYPE",logfs))){
		telog_warn("cannot determine fstype in logfs %s",logfs);
		type="vfat";
		blkid_put_cache(cache);
		cache=NULL;
	}

	char mod[64]={0};
	snprintf(mod,63,"fs-%s",type);
	insmod(mod,false);

	char point[256];
	if(!auto_mountpoint(point,256)){
		e=terlog_error(-errno,"cannot get new mountpoint");
		goto ex;
	}

	if(xmount(false,logfs,point,type,"rw,noatime",true)!=0){
		e=-errno;
		goto ex;
	}

	char cnum[8],path[PATH_MAX];
	keyval*v[]={
		&KV("N",cnum),
		NULL
	};
	size_t s=strlen(point);
	strcpy(path,point);
	path[s++]='/';
	for(int i=0;i<256;i++){
		memset(cnum,0,8);
		memset(path+s,0,PATH_MAX-s);
		snprintf(cnum,7,"%d",i);
		if(!replace(v,'%',path+s,logfile,PATH_MAX-s-1)){
			tlog_error("failed to parse logfile %s",logfile);
			goto ex;
		}
		if(access(path,F_OK)==0)continue;
		if(errno==ENOENT)break;
		telog_error("access %s: %m",path);
		goto ex;
	}
	e=logger_open(path);
	ex:
	if(cache){
		if(type)free(type);
		blkid_put_cache(cache);
	}
	return e;
}