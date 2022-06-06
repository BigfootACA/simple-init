/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * based on kmod
 *
 */

#include<errno.h>
#include<stdio.h>
#include<libkmod.h>
#include<sys/stat.h>
#include"defines.h"
#include"getopt.h"
#include"output.h"

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: rmmod [OPTIONS] MODULENAME ...\n"
		"Remove a module from the Linux Kernel\n\n"
		"Options:\n"
		"\t-f,--force       forces unload a module\n"
		"\t-h,--help        show this help\n"
	);
}

static int check_module_inuse(struct kmod_module*mod){
	struct kmod_list *holders;
	int state=kmod_module_get_initstate(mod),ret;
	if(state==KMOD_MODULE_BUILTIN)return re_printf(
		-ENOENT,
		"rmmod: Module %s is builtin.\n",
		kmod_module_get_name(mod)
	);
	else if(state<0)re_printf(
		-ENOENT,
		"rmmod: Module %s is not currently loaded\n",
		kmod_module_get_name(mod)
	);
	if((holders=kmod_module_get_holders(mod))){
		struct kmod_list *itr;
		fprintf(stderr,_("rmmod: Module %s is in use by:"),kmod_module_get_name(mod));
		kmod_list_foreach(itr,holders){
			struct kmod_module *hm=kmod_module_get_module(itr);
			fprintf(stderr," %s",kmod_module_get_name(hm));
			kmod_module_unref(hm);
		}
		fputc('\n',stderr);
		kmod_module_unref_list(holders);
		ERET(EBUSY);
	}
	if((ret=kmod_module_get_refcnt(mod))>0)return re_printf(
		-EBUSY,
		"rmmod: Module %s is in use\n",
		kmod_module_get_name(mod)
	);
	else if(ret==-ENOENT)fprintf(
		stderr,
		_("rmmod: Module unloading is not supported\n")
	);
	return ret;
}

int rmmod_main(int argc,char**argv){
	struct kmod_ctx *ctx;
	const char *null_config=NULL;
	int flags=0;
	int i,err,r=0,c;
	static const struct option lo[]={
		{"force", no_argument,0,'f'},
		{"help",  no_argument,0,'h'},
		{NULL,0,0,0}
	};
	while((c=b_getlopt(argc,argv,"fh",lo,0))>0)switch(c){
		case 'f':flags |=KMOD_REMOVE_FORCE;break;
		case 'h':return usage(0);
		default:return 1;
	}
	if(b_optind>=argc){
		fprintf(stderr,_("rmmod: missing module name.\n"));
		r=1;
		goto done;
	}
	if(!(ctx=kmod_new(NULL,&null_config))){
		fprintf(stderr,_("error: kmod_new failed!\n"));
		r=1;
		goto done;
	}
	for(i=b_optind;i<argc;i++){
		struct kmod_module*mod;
		const char*arg=argv[i];
		struct stat st;
		err=stat(arg,&st)==0?
			kmod_module_new_from_path(ctx,arg,&mod):
			kmod_module_new_from_name(ctx,arg,&mod);
		if(err<0){
			errno=-err;
			stderr_perror("rmmod: could not use module %s",arg);
			break;
		}
		if(!(flags&KMOD_REMOVE_FORCE)&&check_module_inuse(mod)<0){
			r++;
			goto next;
		}
		if((err=kmod_module_remove_module(mod,flags))<0){
			errno=-err;
			stderr_perror("rmmod: could not remove module %s",arg);
			r++;
		}
		next:
		kmod_module_unref(mod);
	}
	kmod_unref(ctx);
	done:
	return r==0?0:1;
}
