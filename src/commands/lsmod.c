/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<stdio.h>
#include<errno.h>
#include<libkmod.h>
#include"defines.h"
#include"output.h"

// from kmod tools/lsmod.c
int lsmod_main(int argc,char**argv __attribute__((unused))){
	struct kmod_ctx*ctx;
	struct kmod_list*list,*itr;
	int err;
	if(argc!=1)return re_printf(1,"Usage: lsmod\n");
	if(!(ctx=kmod_new(NULL,NULL)))return re_printf(1,"error: kmod_new failed!\n");
	if((err=kmod_module_new_from_loaded(ctx,&list))<0){
		errno=-err;
		perror(_("error: could not get list of modules"));
		kmod_unref(ctx);
		return 1;
	}
	puts(_("Module                  Size  Used by"));
	kmod_list_foreach(itr,list) {
		struct kmod_module*mod=kmod_module_get_module(itr);
		const char*name=kmod_module_get_name(mod);
		int use_count=kmod_module_get_refcnt(mod);
		long size=kmod_module_get_size(mod);
		struct kmod_list*holders,*hitr;
		int first=1;
		printf("%-19s %8ld  %d",name,size,use_count);
		holders=kmod_module_get_holders(mod);
		kmod_list_foreach(hitr,holders) {
			struct kmod_module*hm=kmod_module_get_module(hitr);
			putchar(first?' ':',');
			first=0;
			fputs(kmod_module_get_name(hm),stdout);
			kmod_module_unref(hm);
		}
		putchar('\n');
		kmod_module_unref_list(holders);
		kmod_module_unref(mod);
	}
	kmod_module_unref_list(list);
	kmod_unref(ctx);
	return 0;
}
