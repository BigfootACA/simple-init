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
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/SimpleFileSystem.h>
#include<Guid/Fdt.h>
#include<comp_libfdt.h>
#include<libufdt.h>
#include<ufdt_overlay.h>
#include<stdint.h>
#include"str.h"
#include"list.h"
#include"qcom.h"
#include"logger.h"
#include"fdtparser.h"
#include"internal.h"
#define TAG "dtbo"

static list*dtbos;
typedef struct dtbo_info{
	size_t id;
	fdt address;
	size_t size;
	size_t offset;
	char model[256];
	qcom_chip_info info;
	int64_t vote;
	dtbo_table_entry en;
	size_t en_offset;
	list*compatibles;
}dtbo_info;

static bool sort_dtbo(list*f1,list*f2){
	LIST_DATA_DECLARE(d1,f1,dtbo_info*);
	LIST_DATA_DECLARE(d2,f2,dtbo_info*);
	return d1->vote<d2->vote;
}

static int select_dtbo(linux_boot*lb,dtbo_info*sel){
	int r=-1;
	char buf[64];
	size_t dtbs,dtbos,fdts;
	struct fdt_header*fdt=NULL;
	tlog_info("select dtbo id %zu (%s)",sel->id,sel->model);
	dtbs=fdt_totalsize(lb->dtb.address);
	dtbos=fdt_totalsize(sel->address);
	tlog_debug(
		"dtb size %zu bytes (%s)",dtbs,
		make_readable_str_buf(buf,sizeof(buf),dtbs,1,0)
	);
	tlog_debug(
		"dtbo size %zu bytes (%s)",dtbos,
		make_readable_str_buf(buf,sizeof(buf),dtbos,1,0)
	);
	tlog_debug("try to apply dtb overlay...");
	if((fdt=ufdt_apply_overlay(lb->dtb.address,dtbs,sel->address,dtbos))){
		fdts=fdt_totalsize(fdt);
		linux_file_clean(&lb->dtb);
		if(linux_file_allocate(&lb->dtb,fdts)){
			CopyMem(lb->dtb.address,fdt,fdts);
			tlog_debug(
				"fdt size %zu bytes (%s)",fdts,
				make_readable_str_buf(buf,sizeof(buf),fdts,1,0)
			);
			r=0;
		}else tlog_warn("allocate for dtb failed");
		dto_free(fdt);
	}else tlog_error("apply overlay failed");
	if(r==0)tlog_debug("applied dtb overlay");
	return r;
}

static void process_dtbo(linux_boot*lb,qcom_chip_info*chip_info,dtbo_info*dtbo,bool auto_vote){
	list*n;
	int r,len=0,off=0;
	char*model,*comps,*comp;
	if((r=fdt_check_header(dtbo->address))!=0){
		tlog_warn(
			"dtbo id %zu invalid header: %s",
			dtbo->id,fdt_strerror(r)
		);
		dtbo->vote=INT64_MIN;
		return;
	}
	if(fdt_totalsize(dtbo->address)!=dtbo->size){
		tlog_warn("dtbo id %zu size mismatch",dtbo->id);
		dtbo->vote=INT64_MIN;
		return;
	}
	if(fdt_path_offset(dtbo->address,"/")!=0){
		tlog_warn("dtbo id %zu invalid root offset",dtbo->id);
		dtbo->vote=INT64_MIN;
		return;
	}
	model=(char*)fdt_getprop(dtbo->address,0,"model",&len);
	if(!model)model="Linux Device Tree Overlay";
	AsciiStrCpyS(dtbo->model,sizeof(dtbo->model),model);

	if((comps=(char*)fdt_getprop(dtbo->address,0,"compatible",&len)))do{
		comp=comps+off;
		if(!*comp)continue;
		list_obj_add_new_strdup(&dtbo->compatibles,comp);
		off+=strlen(comp);
	}while(++off<len);

	if(auto_vote){
		qcom_parse_id(dtbo->address,&dtbo->info);
		tlog_verbose("voting dtbo %zu (%s)",dtbo->id,model);
		qcom_dump_info(&dtbo->info);
		dtbo->vote=qcom_check_dtb(&dtbo->info,chip_info);
	}

	if(
		lb->config->dtbo_id>=0&&
		(size_t)lb->config->dtbo_id==dtbo->id
	)dtbo->vote+=0x10000000;
	if(
		lb->config->dtbo_model&&
		list_search_string(lb->config->dtbo_model,dtbo->model)
	)dtbo->vote+=0x100000;
	if(
		lb->config->dtbo_compatible&&
		(n=list_first(dtbo->compatibles))
	)do{
		LIST_DATA_DECLARE(c,dtbo->compatibles,char*);
		if(c&&list_search_string(lb->config->dtbo_compatible,c))
			dtbo->vote+=0x1000;
	}while((n=n->next));
	if(linux_boot_match_kfdt_model(lb,dtbo->model))dtbo->vote+=0x10000000;

	lb->status.dtbo_id=(int64_t)dtbo->id;
	tlog_debug(
		"dtbo id %zu offset %zu size %zu vote %lld (%s)",
		dtbo->id,dtbo->offset,dtbo->size,
		(long long)dtbo->vote,
		dtbo->model
	);
	list_obj_add_new_dup(&dtbos,dtbo,sizeof(dtbo_info));
}

static int read_qcom_dtbo(linux_boot*lb,linux_file_info*fi){
	int r=-1;
	list*f=NULL;
	dtbo_info dtbo;
	dtbo_info*sel=NULL;
	qcom_chip_info chip_info;
	dtbo_table_header hdr;
	uint32_t ens_cnt,ens_off,en_size,total_size,header_size;
	bool auto_vote=true;
	if(lb->config->dtbo_id>=0){
		tlog_debug("use pre selected dtbo id from config");
		auto_vote=false;
	}
	if(lb->config->dtbo_model){
		tlog_debug("use pre selected dtbo model from config");
		auto_vote=false;
	}
	if(lb->config->dtbo_compatible){
		tlog_debug("use pre selected dtbo compatible from config");
		auto_vote=false;
	}
	if(!auto_vote)tlog_debug("disabled dtbo auto vote");
	CopyMem(&hdr,fi->address,sizeof(dtbo_table_header));
	en_size=fdt32_to_cpu(hdr.dt_entry_size);
	ens_cnt=fdt32_to_cpu(hdr.dt_entry_count);
	ens_off=fdt32_to_cpu(hdr.dt_entry_offset);
	total_size=fdt32_to_cpu(hdr.total_size);
	header_size=fdt32_to_cpu(hdr.header_size);
	if(
		total_size<=0||
		total_size>MAX_DTBO_SIZE||
		total_size>=fi->size||
		header_size!=sizeof(dtbo_table_header)||
		en_size!=sizeof(dtbo_table_entry)
	)EDONE(tlog_error("invalid dtbo header"));
	if(auto_vote&&qcom_get_chip_info(lb,&chip_info)!=0)goto done;
	tlog_info("found %u dtb overlays",ens_cnt);
	for(size_t s=0;s<ens_cnt;s++){
		ZeroMem(&dtbo,sizeof(dtbo_info));
		dtbo.id=s;
		dtbo.en_offset=ens_off+(en_size*s);
		CopyMem(&dtbo.en,fi->address+dtbo.en_offset,en_size);
		dtbo.offset=fdt32_to_cpu(dtbo.en.dt_offset);
		dtbo.address=fi->address+dtbo.offset;
		dtbo.size=fdt32_to_cpu(dtbo.en.dt_size);
		process_dtbo(lb,&chip_info,&dtbo,auto_vote);
	}
	list_sort(dtbos,sort_dtbo);
	if(
		!(f=list_first(dtbos))||
		!(sel=LIST_DATA(f,dtbo_info*))
	)EDONE(tlog_warn("no dtbo found"));
	if(sel->vote<0)EDONE(tlog_warn(
		"selected dtbo %zu (%s) vote too few",
		sel->id,sel->model
	));
	r=select_dtbo(lb,sel);
	done:
	list_free_all_def(dtbos);
	dtbos=NULL;
	return r;
}

static int fi_free(void*data){
	if(!data)return 0;
	linux_file_clean((linux_file_info*)data);
	free(data);
	return 0;
}

int linux_boot_apply_dtbo(linux_boot*lb){
	list*f;
	int r=0;
	size_t i=0;
	static const uint32_t dtbo_magic=MAGIC_DTBO;
	if(!lb->config)return -1;
	if(!lb->dtbo||!lb->dtb.address)return 0;
	if(lb->config->skip_dtbo)
		EDONE(tlog_debug("skip load dtbo"));
	if((f=list_first(lb->dtbo)))do{
		i++;
		LIST_DATA_DECLARE(fi,f,linux_file_info*);
		int xr=-1;
		if(fi->size<=4||fi->size>MAX_DTBO_SIZE){
			tlog_error("invalid dtbo #%zu size",i);
			continue;
		}
		if(CompareMem(&fdt_magic,fi->address,4)==0){
			tlog_debug("found generic dtbo #%zu",i);
			lb->status.dtbo_id=0;
			fdt_set_totalsize(lb->dtb.address,fi->size+lb->dtb.size);
			xr=fdt_overlay_apply(lb->dtb.address,fi->address);
			if(xr!=0)tlog_error("apply overlay failed: %s",fdt_strerror(xr));
		}else if(CompareMem(&dtbo_magic,fi->address,4)==0){
			tlog_debug("found qualcomm dtbo #%zu",i);
			if(fi->size<=sizeof(dtbo_table_header)){
				tlog_error("invalid dtbo #%zu size",i);
				continue;
			}
			xr=read_qcom_dtbo(lb,fi);
		}else{
			tlog_warn("unknown dtbo #%zu, skip",i);
			continue;
		}
		if(xr!=0)r=xr;
	}while((f=f->next));
	done:
	list_free_all(lb->dtbo,fi_free);
	lb->dtbo=NULL;
	if(lb->config->ignore_dtbo_error)r=0;
	return r;
}
