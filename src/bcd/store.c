/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_HIVEX
#define _GNU_SOURCE
#include<hivex.h>
#include<string.h>
#include"keyval.h"
#include"bcdstore.h"

bcd_store bcd_store_open(const char*path,int flags){
	if(!path||!path[0])EPRET(EINVAL);
	bcd_store bcd=malloc(sizeof(struct bcd_store));
	if(!bcd)goto fail;
	memset(bcd,0,sizeof(struct bcd_store));
	realpath(path,bcd->path);
	if(!(bcd->reg=hivex_open(bcd->path,flags)))goto fail;
	errno=ENOTSUP;
	if((bcd->root=hivex_root(bcd->reg))<=0)goto fail;
	if((bcd->objs=hivex_node_get_child(
		bcd->reg,bcd->root,"Objects"
	))<=0)goto fail;
	errno=0;
	return bcd;
	fail:
	if(bcd)bcd_store_free(bcd);
	return NULL;
}

const char*bcd_store_get_path(bcd_store store){
	return store&&store->path[0]?store->path:NULL;
}

hive_h*bcd_store_get_hive(bcd_store store){
	return store&&store->reg?store->reg:NULL;
}

void bcd_store_free(bcd_store store){
	if(!store)return;
	if(store->reg)hivex_close(store->reg);
	list*l,*n;
	if((l=list_first(store->objects)))do{
		n=l->next;
		LIST_DATA_DECLARE(v,l,bcd_object);
		bcd_object_free(v);
	}while((l=n));
	list_free_all_def(store->to_free);
	free(store);
}

#endif
