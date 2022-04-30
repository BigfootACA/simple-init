/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<Library/UefiLib.h>
#include<Library/BaseLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/DevicePathLib.h>
#include<Library/MemoryAllocationLib.h>
#include<comp_libfdt.h>
#include<stdint.h>
#include"list.h"
#include"qcom.h"
#include"logger.h"
#include"fdtparser.h"
#include"internal.h"
#define TAG "fdt"

static list*fdts=NULL;

typedef struct fdt_info{
	size_t id;
	fdt address;
	size_t offset;
	size_t size;
	size_t mem_size;
	size_t mem_pages;
	char model[256];
	qcom_chip_info info;
	int64_t vote;
	list*compatibles;
}fdt_info;

static void search_dtb(linux_boot*lb,void*buff,void**pos){
	fdt_info fi;
	int len,off=0;
	char*model,*comps,*comp;
	if(CompareMem(&fdt_magic,(*pos),4)!=0)goto fail;

	ZeroMem(&fi,sizeof(fdt_info));
	fi.size=fdt_totalsize((*pos));
	if(fi.size<=0||fi.size>=MAX_DTB_SIZE)goto fail;
	ZeroMem(buff,MAX_DTB_SIZE);
	CopyMem(buff,(*pos),fi.size);
	fi.offset=(*pos)-lb->dtb.address;
	(*pos)+=fi.size;
	if(fdt_check_header(buff)!=0)return;
	if(fdt_path_offset(buff,"/")!=0)return;
	fi.mem_pages=EFI_SIZE_TO_PAGES(ALIGN_VALUE(fi.size,MEM_ALIGN));
	fi.mem_size=EFI_PAGES_TO_SIZE(fi.mem_pages);
	if(!(fi.address=AllocateAlignedPages(fi.mem_pages,MEM_ALIGN)))return;
	ZeroMem(fi.address,fi.mem_size);
	CopyMem(fi.address,buff,fi.size);
	fi.id=MAX(list_count(fdts),0);

	model=(char*)fdt_getprop(fi.address,0,"model",&len);
	if(!model)model="Linux Device Tree Blob";
	AsciiStrCpyS(fi.model,sizeof(fi.model),model);

	if((comps=(char*)fdt_getprop(fi.address,0,"compatible",&len)))do{
		comp=comps+off;
		if(!*comp)continue;
		list_obj_add_new_strdup(&fi.compatibles,comp);
		off+=strlen(comp);
	}while(++off<len);

	qcom_parse_id(fi.address,&fi.info);
	if(fi.info.soc_id!=0)lb->status.qualcomm=true;
	tlog_verbose(
		"dtb id %zu offset %zu size %zu (%s)",
		fi.id,fi.offset,fi.size,fi.model
	);

	list_obj_add_new_dup(&fdts,&fi,sizeof(fdt_info));
	return;
	fail:(*pos)++;
}

static int free_fdt(void*d){
	fdt_info*f=(fdt_info*)d;
	if(!f)return 0;
	if(f->address)FreePages(f->address,f->mem_pages);
	free(f);
	return 0;
}

static bool sort_fdt(list*f1,list*f2){
	LIST_DATA_DECLARE(d1,f1,fdt_info*);
	LIST_DATA_DECLARE(d2,f2,fdt_info*);
	return d1->vote<d2->vote;
}

static int check_dtbs(linux_boot*lb){
	list*f,*n;
	qcom_chip_info chip_info;
	bool auto_vote=lb->status.qualcomm;
	if(lb->config->dtb_id>=0){
		tlog_debug("use pre selected dtb id from config");
		auto_vote=false;
	}
	if(lb->config->dtb_model){
		tlog_debug("use pre selected dtb model from config");
		auto_vote=false;
	}
	if(lb->config->dtb_compatible){
		tlog_debug("use pre selected dtb compatible from config");
		auto_vote=false;
	}
	if(!auto_vote)tlog_debug("disabled dtb auto vote");
	else if(qcom_get_chip_info(lb,&chip_info)!=0)return -1;
	if((f=list_first(fdts)))do{
		LIST_DATA_DECLARE(fdt,f,fdt_info*);
		if(auto_vote){
			tlog_verbose("voting dtb %zu (%s)",fdt->id,fdt->model);
			qcom_dump_info(&fdt->info);
			fdt->vote=qcom_check_dtb(&fdt->info,&chip_info);
		}
		if(
			lb->config->dtb_id>=0&&
			(size_t)lb->config->dtb_id==fdt->id
		)fdt->vote+=0x10000000;
		if(
			lb->config->dtb_model&&
			list_search_string(lb->config->dtb_model,fdt->model)
		)fdt->vote+=0x100000;
		if(
			lb->config->dtb_compatible&&
			(n=list_first(fdt->compatibles))
		)do{
			LIST_DATA_DECLARE(c,n,char*);
			if(c&&list_search_string(lb->config->dtb_compatible,c))
				fdt->vote+=0x1000;
		}while((n=n->next));
		if(linux_boot_match_kfdt_model(lb,fdt->model))fdt->vote+=0x10000000;
		tlog_debug(
			"dtb id %zu offset %zu size %zu vote %lld (%s)",
			fdt->id,fdt->offset,fdt->size,
			(long long)fdt->vote,fdt->model
		);
	}while((f=f->next));
	list_sort(fdts,sort_fdt);
	return 0;
}

static int select_dtb(linux_boot*lb,fdt_info*info){
	lb->status.dtb_id=(int64_t)info->id;
	tlog_info("select dtb id %zu (%s)",info->id,info->model);
	linux_boot_set_fdt(lb,info->address,info->size);
	return 0;
}

int linux_boot_select_fdt(linux_boot*lb){
	list*f=NULL;
	int r,ret=-1;
	fdt_info*fdt=NULL;
	void*buff,*end,*pos;
	if(!lb->config)return -1;
	if(!lb||!lb->dtb.address)return 0;
	list_free_all(fdts,free_fdt);
	lb->status.qualcomm=false,fdts=NULL;
	r=fdt_check_header(lb->dtb.address);
	if(r!=0){
		tlog_warn("invalid dtb head: %s",fdt_strerror(r));
		linux_file_clean(&lb->dtb);
		return -1;
	}
	if(fdt_totalsize(lb->dtb.address)==lb->dtb.size)return 0;
	if(!(buff=AllocateZeroPool(MAX_DTB_SIZE)))
		EDONE(tlog_error("allocate for fdt buff failed"));

	pos=lb->dtb.address,end=pos+lb->dtb.size;
	while(pos<end-4)search_dtb(lb,buff,&pos);
	FreePool(buff);
	tlog_info("found %d dtbs",fdts?list_count(fdts):0);
	if(!fdts)EDONE(tlog_warn("no dtb found"));
	check_dtbs(lb);
	if(
		!(f=list_first(fdts))||
		!(fdt=LIST_DATA(f,fdt_info*))
	)EDONE(tlog_warn("no dtb found"));
	if(fdt->vote<0)EDONE(tlog_warn(
		"selected dtb %zu (%s) vote too few",
		fdt->id,fdt->model
	));
	ret=select_dtb(lb,fdt);
	done:
	list_free_all(fdts,free_fdt);
	fdts=NULL;
	return ret;
}
