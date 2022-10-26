/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_UEFI
#include"str.h"
#include"boot.h"
#include"list.h"
#include"confd.h"
#include"keyval.h"
#include"locate.h"
#include"logger.h"
#include"filesystem.h"
#define TAG "prober"

static struct boot_config def={
	.mode=BOOT_EFI,
	.save=false,.replace=false,
	.show=true,.enabled=true
};

typedef struct prober_cache{
	char node[255];
	fsh*hand;
	list*sub;
}prober_cache;

static bool prober_cache_cmp(list*l,void*d){
	LIST_DATA_DECLARE(c,l,prober_cache*);
	return c&&d&&strcasecmp(c->node,d)==0;
}

static prober_cache*get_component(prober_cache*p,const char*name,bool dir){
	list*c;
	prober_cache*child;
	static const size_t size=sizeof(prober_cache);
	if(!p||!name)return NULL;
	if(!(c=list_search_one(p->sub,prober_cache_cmp,(void*)name))){
		if(!(child=malloc(size)))return NULL;
		memset(child,0,size);
		fs_open(
			p->hand,&child->hand,name,
			dir?FILE_FLAG_FOLDER:FILE_FLAG_READ
		);
		strncpy(child->node,name,sizeof(child->node)-1);
		if(list_obj_add_new(&p->sub,child)!=0){
			free(child);
			child=NULL;
		}
	}else child=LIST_DATA(c,prober_cache*);
	return child;
}

static int free_cache(void*d){
	prober_cache*cache=d;
	if(!cache)return 0;
	list_free_all(cache->sub,free_cache);
	fs_close(&cache->hand);
	memset(cache,0,sizeof(prober_cache));
	free(cache);
	return 0;
}

static bool add_item(fsh*f,struct efi_path*ep,char*path,int id,int part){
	char*url=NULL;
	struct boot_config use;
	if(!f||!ep||!path)return false;
	if(fs_get_path_alloc(f,&url)!=0||!url)return false;
	telog_debug("found %s at %s from part %d",ep->title,path,part);
	memcpy(&use,&def,sizeof(struct boot_config));
	snprintf(use.desc,sizeof(use.desc)-1,"%s on #%d",ep->title,part);
	snprintf(use.ident,sizeof(use.ident)-1,"prober-%d",id);
	strncpy(use.icon,ep->icon,sizeof(use.icon));
	boot_create_config(&use,NULL);
	confd_set_string_base(use.key,"efi_file",url);
	if(ep->load_opt){
		confd_set_string_base(use.key,"options",ep->load_opt);
		confd_set_boolean_base(use.key,"options_widechar",ep->unicode);
	}
	free(url);
	return true;
}

static bool boot_scan_part(int part,int*id,fsh*root){
	list*l1,*px;
	fsh*nf=NULL;
	bool found=false;
	struct efi_path*ep;
	char path[PATH_MAX];
	prober_cache*c,cr={.node="",.hand=root,.sub=NULL,};
	for(size_t p=0;(ep=&boot_efi_paths[p])&&ep->title;p++){
		if(!ep->enable||!ep->name||!ep->title)continue;
		if(ep->cpu!=CPU_ANY&&ep->cpu!=current_cpu)continue;
		for(size_t d=0;ep->dir[d];d++)for(size_t d=0;ep->name[d];d++){
			nf=NULL,c=&cr;
			memset(path,0,sizeof(path));
			snprintf(
				path,sizeof(path)-1,"%s%s",
				ep->dir[d],ep->name[d]
			);
			if(!(px=path2list(path,false)))continue;
			if((l1=list_first(px)))do{
				LIST_DATA_DECLARE(comp,l1,char*);
				c=get_component(c,comp,l1->next!=NULL);
			}while((l1=l1->next)&&c&&c->hand);
			if(c&&c->hand)nf=c->hand;
			list_free_all_def(px);
			if(!add_item(nf,ep,path,*id,part))continue;
			fs_close(&nf);
			(*id)++,found=true;
		}
	}
	list_free_all(cr.sub,free_cache);
	return found;
}

void boot_scan_efi(){
	fsh*nf;
	int id=0;
	fsvol_info**vols,*v;
	if(!confd_get_boolean("boot.uefi_probe",true))return;
	if(!(vols=fsvol_get_volumes()))return;
	for(size_t i=0;(v=vols[i]);i++){
		nf=NULL;
		if(!fs_has_vol_feature(v->features,FSVOL_FILES))continue;
		if(fsvol_open_volume(vols[i],&nf)!=0)continue;
		boot_scan_part((int)i,&id,nf);
		fs_close(&nf);
	}
	free(vols);
	tlog_debug("%d items found",id);
}
#endif
