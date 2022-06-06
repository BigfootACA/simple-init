/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include<stdio.h>
#include<libkmod.h>
#include<string.h>
#include<stdlib.h>
#include"defines.h"
#include"output.h"
#include"getopt.h"

// from kmod tools/insmod.c
static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: insmod [OPTIONS] FILE [ARGS]\n"
		"Insert a module into the Linux Kernel\n"
		"Options:\n"
		"\t-h, --help        show this help\n"
	);
}

static const char*mod_strerror(int err){
	switch(err){
		case ENOEXEC:return _("Invalid module format");
		case ENOENT:return _("Unknown symbol in module");
		case ESRCH:return _("Module has wrong symbol version");
		case EINVAL:return _("Invalid parameters");
		default:return strerror(err);
	}
}

int insmod_main(int argc,char**argv){
	struct kmod_ctx*ctx;
	struct kmod_module*mod;
	const char*filename;
	char*opts=NULL;
	size_t optslen=0;
	int i,err,c;
	unsigned int flags=0;
	struct option lo[]={
		{"help",no_argument,0,'h'},
		{NULL,0,0,0}
	};
	while((c=b_getlopt(argc,argv,"psfVh",lo,0))!=-1)switch(c){
		case 'p':case 's':break;
		case 'f':
			flags|=KMOD_PROBE_FORCE_MODVERSION;
			flags|=KMOD_PROBE_FORCE_VERMAGIC;
		break;
		case 'h':return usage(0);
		default:return 1;
	}
	if(b_optind>=argc)return re_printf(1,"insmod: missing filename.\n");
	filename=argv[b_optind];
	if(strcmp(filename,"-")==0)
		return re_printf(1,"insmod: loading stdin not supported!\n");
	for(i=b_optind+1;i<argc;i++){
		size_t len=strlen(argv[i]);
		void*tmp=realloc(opts,optslen+len+2);
		if(!tmp){
			free(opts);
			return re_printf(1,"insmod: out of memory\n");
		}
		opts=tmp;
		if(optslen>0)opts[optslen]=' ',optslen++;
		memcpy(opts+optslen,argv[i],len);
		optslen+=len;
		opts[optslen]='\0';
	}
	if(!(ctx=kmod_new(NULL,NULL))){
		free(opts);
		return re_printf(1,"insmod: kmod_new failed!\n");
	}
	if((err=kmod_module_new_from_path(ctx,filename,&mod))<0){
		errno=-err;
		stderr_perror("insmod: could not load module %s",filename);
		goto end;
	}
	if((err=kmod_module_insert_module(mod,flags,opts))<0)fprintf(
		stderr,
		_("insmod: could not insert module %s: %s\n"),
		filename,mod_strerror(-err)
	);
	kmod_module_unref(mod);
	end:
	kmod_unref(ctx);
	free(opts);
	return err>=0?0:1;
}
