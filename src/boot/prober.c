/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_UEFI
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

static bool boot_scan_part(int part,int*id,fsh*root){
	fsh*nf=NULL;
	list*probes=NULL;
	char path[PATH_MAX];
	bool found=false;
	char*dir,*name,*url;
	struct efi_path*ep;
	struct boot_config*use=NULL;
	if(!(use=malloc(sizeof(struct boot_config))))goto done;
	for(size_t p=0;(ep=&boot_efi_paths[p])&&ep->title;p++){
		if(
			!ep->enable||
			!ep->name||!ep->name[0]||
			!ep->title||!ep->title[0]
		)continue;
		if(ep->cpu!=CPU_ANY&&ep->cpu!=current_cpu)continue;
		for(size_t d=0;(dir=ep->dir[d]);d++)for(size_t d=0;(name=ep->name[d]);d++){
			nf=NULL,url=NULL;
			memset(path,0,sizeof(path));
			snprintf(path,sizeof(path)-1,"%s%s",dir,name);
			if(list_search_case_string(probes,path))continue;
			if(fs_open(root,&nf,path,FILE_FLAG_READ)!=0)continue;
			fs_get_path_alloc(nf,&url);
			fs_close(&nf);
			if(!url)continue;
			tlog_debug("found %s at %s from part %d",ep->title,path,part);
			list_obj_add_new_strdup(&probes,path);
			memcpy(use,&def,sizeof(struct boot_config));
			snprintf(use->desc,sizeof(use->desc)-1,"%s on #%d",ep->title,part);
			snprintf(use->ident,sizeof(use->ident)-1,"prober-%d",*id);
			strncpy(use->icon,ep->icon,sizeof(use->icon));
			boot_create_config(use,NULL);
			confd_set_string_base(use->key,"efi_file",url);
			if(ep->load_opt){
				confd_set_string_base(use->key,"options",ep->load_opt);
				confd_set_boolean_base(use->key,"options_widechar",ep->unicode);
			}
			free(url);
			(*id)++,found=true;
		}
	}
	done:
	if(use)free(use);
	list_free_all_def(probes);
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
