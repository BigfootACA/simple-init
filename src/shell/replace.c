/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_READLINE
#define _GNU_SOURCE
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<sys/utsname.h>
#include"shell_internal.h"
#include"defines.h"
#include"system.h"
#include"str.h"

char*shell_replace(char*dest,char*src,size_t size){
	if(!src||!dest)return NULL;
	memset(dest,0,size);

	// working directory
	char n[PATH_MAX]={0};
	getcwd(n,PATH_MAX-1);
	char*sn=basename(n);

	// hostname
	struct utsname uts;
	uname(&uts);
	char*fh=strrep(strdup(uts.nodename),'.',0);

	// username/uid
	uid_t u=geteuid();
	char user[128],uid[32]={0};
	get_username(u,user,128);
	snprintf(uid,31,"%d",u);

	// groupname/gid
	gid_t g=getegid();
	char group[128],gid[32]={0};
	get_groupname(g,group,128);
	snprintf(gid,31,"%d",g);

	// exit code
	char code[8]={0},codep[8]={0};
	if(exit_code>0){
		snprintf(code,7,"%d",exit_code);
		snprintf(codep,7,"%d:",exit_code);
	}

	keyval*v[]={
		&KV("[",""),&KV("]",""),

		// escape char
		&KV("e","\033"),

		// if the effective UID is 0, a #, otherwise a $
		&KV("$",u==0?"#":"$"),

		// username
		&KV("u",user),

		// uid
		&KV("U",uid),

		// groupname
		&KV("g",group),

		// gid
		&KV("G",gid),

		// the current working directory
		&KV("w",n),

		// the basename of the current working directory
		&KV("W",sn),

		// full hostname
		&KV("H",uts.nodename),

		// the hostname up to the first '.'
		&KV("h",fh),

		// previous exit code (empty if 0)
		&KV("v",code),

		// previous exit code end with ':' (empty if 0)
		&KV("V",codep),

		NULL
	};
	char*r=replace(v,'\\',dest,src,size);
	free(fh);
	return r;
}
#endif
