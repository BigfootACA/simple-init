/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<libmount/libmount.h>
#include"logger.h"
#define TAG "umount"

int umount_all(){
	struct libmnt_context*cxt=NULL;
	struct libmnt_iter*itr=NULL;
	struct libmnt_fs*fs;
	int mntrc,ignored,rc=0;
	if(!(itr=mnt_new_iter(MNT_ITER_BACKWARD)))
		return trlog_error(-1,"failed to initialize libmount iterator");
	if(!(cxt=mnt_new_context()))goto er;
	while(mnt_context_next_umount(cxt,itr,&fs,&mntrc,&ignored)==0)if(!ignored){
		const char*tgt=mnt_fs_get_target(fs);
		char buf[BUFSIZ]={0};
		int r=mnt_context_get_excode(cxt,mntrc,buf,sizeof(buf));
		if(*buf){
			const char*spec=mnt_context_get_target(cxt);
			if(!spec)spec=mnt_context_get_source(cxt);
			if(!spec)spec="???";
			tlog_error("failed to mount umount %s: %s",spec,buf);
		}
		rc|=r;
		if(r==0)tlog_debug("umount %s",tgt);
	}
	er:
	mnt_free_iter(itr);
	if(cxt)mnt_free_context(cxt);
	return rc;
}
