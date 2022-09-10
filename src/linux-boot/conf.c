/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<Library/PcdLib.h>
#include<Library/BaseLib.h>
#include<Library/PrintLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#include"str.h"
#include"list.h"
#include"confd.h"
#include"logger.h"
#include"internal.h"
#define TAG "linux-config"

static inline void get_from_confd(linux_load_from*from,const char*key,const char*sub){
	char*s;
	if(!(s=confd_get_string_base(key,sub,NULL)))return;
	AsciiStrCpyS(from->locate,sizeof(from->locate)-1,s);
	from->enabled=true;
	from->type=FROM_LOCATE;
	free(s);
}

static inline void get_multi_from_confd(list**from,const char*key,const char*sub){
	char*s=NULL;
	bool multi=false;
	linux_load_from*f=NULL;
	switch((int)confd_get_type_base(key,sub)){
		case -1:return;
		case TYPE_KEY:multi=true;break;
		case TYPE_STRING:multi=false;break;
		default:tlog_warn("invalid config %s.%s",key,sub);return;
	}
	if(multi){
		char**b=confd_ls_base(key,sub);
		if(!b)return;
		for(size_t i=0;b[i];i++){
			if(
				!(s=confd_get_string_dict(key,sub,b[i],NULL))||
				!(f=malloc(sizeof(linux_load_from)))
			){
				if(s)free(s);
				if(f)free(f);
				s=NULL,f=NULL;
				continue;
			}
			ZeroMem(f,sizeof(linux_load_from));
			AsciiStrCpyS(f->locate,sizeof(f->locate)-1,s);
			f->enabled=true;
			f->type=FROM_LOCATE;
			list_obj_add_new(from,f);
			free(s);
		}
		if(b[0])free(b[0]);
		free(b);
	}else{
		if(!(f=malloc(sizeof(linux_load_from)))){
			tlog_warn("allocate load from failed");
			return;
		}
		ZeroMem(f,sizeof(linux_load_from));
		get_from_confd(f,key,sub);
		list_obj_add_new(from,f);
	}
}

static inline void get_multi_str_from_confd(list**from,const char*key,const char*sub){
	switch((int)confd_get_type_base(key,sub)){
		case -1:return;
		case TYPE_STRING:{
			char*x=confd_get_string_base(key,sub,NULL);
			if(x)list_obj_add_new(from,x);
		}break;
		case TYPE_KEY:{
			char**xs=confd_ls_base(key,sub),*x;
			if(!xs)break;
			for(size_t i=0;xs[i];i++)
				if((x=confd_get_string_dict(key,sub,xs[i],NULL)))
					list_obj_add_new(from,x);
			if(xs[0])free(xs[0]);
			free(xs);
		}break;
		default:tlog_warn("invalid config %s.%s",key,sub);
	}
}

static inline void get_region_confd(linux_mem_region*reg,const char*key,const char*sub){
	int64_t base=confd_get_integer_dict(key,sub,"base",0);
	int64_t size=confd_get_integer_dict(key,sub,"size",0);
	int64_t start=confd_get_integer_dict(key,sub,"start",0);
	int64_t end=confd_get_integer_dict(key,sub,"end",0);
	if((base<=0&&start<=0)||(size<=0&&end<=0))return;
	if((base>0&&start>0)||(size>0&&end>0))return;
	reg->start=start>0?(uint64_t)start:(uint64_t)base;
	reg->end=end>0?(uint64_t)end:(reg->start+(uint64_t)size);
}

static inline void get_memory_confd(linux_config*cfg,const char*key){
	char**ks,path[PATH_MAX];
	ZeroMem(path,sizeof(path));
	AsciiSPrint(path,sizeof(path)-1,"%a.memory",key);
	if(!(ks=confd_ls(path)))return;
	for(size_t i=0;ks[i];i++){
		if(i>=ARRAY_SIZE(cfg->memory)){
			tlog_warn("too many memory region items");
			break;
		}
		get_region_confd(&cfg->memory[i],key,ks[i]);
	}
	free(ks[0]);
	free(ks);
}

static inline void get_boot_addresses_pcd(linux_config*cfg){
	cfg->load_custom_address=PcdGetBool(PcdBootCustomLoadAddress);
	cfg->load_address.load.start=PcdGet64(PcdBootLoadAddressStart);
	cfg->load_address.load.end=PcdGet64(PcdBootLoadAddressEnd);
	cfg->load_address.kernel.start=PcdGet64(PcdBootKernelAddressStart);
	cfg->load_address.kernel.end=PcdGet64(PcdBootKernelAddressEnd);
	cfg->load_address.initrd.start=PcdGet64(PcdBootRamdiskAddressStart);
	cfg->load_address.initrd.end=PcdGet64(PcdBootRamdiskAddressEnd);
	cfg->load_address.fdt.start=PcdGet64(PcdBootFdtAddressStart);
	cfg->load_address.fdt.end=PcdGet64(PcdBootFdtAddressEnd);
}

linux_config*linux_config_new(){
	linux_config*cfg=AllocateZeroPool(sizeof(linux_config));
	if(cfg){
		cfg->arch=ARCH_UEFI;
		cfg->use_uefi=true;
		cfg->dtb_id=-1;
		cfg->dtbo_id=-1;
		cfg->screen.update_splash=true;
		get_boot_addresses_pcd(cfg);
	}
	return cfg;
}

#define load_boolean(key,name,var) var=confd_get_boolean_base(key,name,var)

linux_config*linux_config_new_from_confd(const char*key){
	linux_config*cfg=linux_config_new();
	if(!cfg)return NULL;
	confd_get_sstring_base(key,"cmdline",NULL,cfg->cmdline,sizeof(cfg->cmdline));
	get_from_confd(&cfg->abootimg,key,"abootimg");
	get_from_confd(&cfg->kernel,key,"kernel");
	get_multi_from_confd(&cfg->initrd,key,"initrd");
	get_multi_from_confd(&cfg->dtbo,key,"dtbo");
	get_from_confd(&cfg->dtb,key,"dtb");
	get_region_confd(&cfg->screen.splash,key,"splash");
	get_memory_confd(cfg,key);
	load_boolean(key,"use_uefi",cfg->use_uefi);
	load_boolean(key,"skip_dtb",cfg->skip_dtb);
	load_boolean(key,"skip_dtbo",cfg->skip_dtbo);
	load_boolean(key,"skip_initrd",cfg->skip_initrd);
	load_boolean(key,"skip_efi_memory_map",cfg->skip_efi_memory_map);
	load_boolean(key,"skip_kernel_fdt_memory",cfg->skip_kfdt_memory);
	load_boolean(key,"skip_kernel_fdt_cmdline",cfg->skip_kfdt_cmdline);
	load_boolean(key,"skip_abootimg_kernel",cfg->skip_abootimg_kernel);
	load_boolean(key,"skip_abootimg_cmdline",cfg->skip_abootimg_cmdline);
	load_boolean(key,"skip_abootimg_initrd",cfg->skip_abootimg_initrd);
	load_boolean(key,"skip_dtb_after_kernel",cfg->skip_dtb_after_kernel);
	load_boolean(key,"force_dtb_after_kernel",cfg->force_dtb_after_kernel);
	load_boolean(key,"load_custom_address",cfg->load_custom_address);
	load_boolean(key,"match_kernel_fdt_model",cfg->match_kfdt_model);
	load_boolean(key,"ignore_dtbo_error",cfg->ignore_dtbo_error);
	load_boolean(key,"add_uefi_runtime",cfg->add_uefi_runtime);
	load_boolean(key,"pass_kernel_fdt_as_dtb",cfg->pass_kfdt_dtb);
	load_boolean(key,"use_kernel_fdt_ramdisk_kernel",cfg->use_kfdt_ramdisk_kernel);
	load_boolean(key,"use_kernel_fdt_ramdisk_abootimg",cfg->use_kfdt_ramdisk_abootimg);
	load_boolean(key,"add_simplefb",cfg->screen.add_simplefb);
	load_boolean(key,"update_splash",cfg->screen.update_splash);
	get_multi_str_from_confd(&cfg->dtb_model,key,"dtb_model");
	get_multi_str_from_confd(&cfg->dtb_compatible,key,"dtb_compatible");
	get_multi_str_from_confd(&cfg->dtbo_model,key,"dtbo_model");
	get_multi_str_from_confd(&cfg->dtbo_compatible,key,"dtbo_compatible");
	cfg->dtb_id=confd_get_integer_base(key,"dtb_id",-1);
	cfg->dtbo_id=confd_get_integer_base(key,"dtbo_id",-1);
	cfg->info.soc_id=confd_get_integer_base(key,"soc_id",0);
	cfg->info.soc_rev=confd_get_integer_base(key,"soc_rev",0);
	cfg->info.foundry_id=confd_get_integer_base(key,"foundry_id",0);
	cfg->info.variant_major=confd_get_integer_base(key,"variant_major",0);
	cfg->info.variant_minor=confd_get_integer_base(key,"variant_minor",0);
	cfg->info.variant_id=confd_get_integer_base(key,"variant_id",0);
	cfg->info.subtype_id=confd_get_integer_base(key,"subtype_id",0);
	cfg->info.subtype_ddr=confd_get_integer_base(key,"subtype_ddr",0);
	cfg->screen.width=confd_get_integer_base(key,"screen_width",0);
	cfg->screen.height=confd_get_integer_base(key,"screen_height",0);
	cfg->screen.stride=confd_get_integer_base(key,"screen_stride",0);
	if(cfg->load_custom_address){
		get_region_confd(&cfg->load_address.load,key,"address.load");
		get_region_confd(&cfg->load_address.kernel,key,"address.kernel");
		get_region_confd(&cfg->load_address.initrd,key,"address.initrd");
		get_region_confd(&cfg->load_address.fdt,key,"address.dtb");
	}
	return cfg;
}

void linux_config_free(linux_config*cfg){
	if(!cfg)return;
	FreePool(cfg);
}
