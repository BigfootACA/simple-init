/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/uio.h>
#include"output.h"
#include"defines.h"

int write_main(int argc,char**argv __attribute__((unused))){
	if(argc<3)return re_printf(1,"Usage: write <FD> <DATA>...\n");
	char*value=argv[1],*end;
	int l=(int)strtol(value,&end,10);
	if(*end||value==end||errno!=0)
		return re_printf(2,"invalid file descriptor\n");
	int cnt=(argc-2)*2-1,off;
	size_t len=sizeof(struct iovec)*cnt;
	struct iovec*iov=malloc(len);
	memset(iov,0,len);
	for(int i=2;i<argc;i++){
		off=(i-2)*2;
		iov[off].iov_base=argv[i];
		iov[off].iov_len=strlen(argv[i]);
		if(i!=argc-1){
			off++;
			iov[off].iov_base=" ";
			iov[off].iov_len=1;
		}
	}
	errno=0;
	ssize_t r=writev(l,iov,cnt);
	if(r<0)perror("write failed");
	free(iov);
	printf("%zu\n",r);
	return errno;
}
