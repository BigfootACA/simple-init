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
#include<comp_libfdt.h>
#include<errno.h>
#include<stdint.h>
#include"qcom.h"
#include"logger.h"
#include"internal.h"
#include"chipinfo.h"
#include"platforminfo.h"
#define TAG "qcom"

int qcom_parse_id(void*dtb,qcom_chip_info*info){
	int off,msm_len=0,board_len=0;
	if(!dtb||!info||fdt_check_header(dtb)!=0)return -1;
	if((off=fdt_path_offset(dtb,"/"))<0)return -1;
	qcom_msm_id*msm_id=(qcom_msm_id*)fdt_getprop(dtb,off,"qcom,msm-id",&msm_len);
	qcom_board_id*board_id=(qcom_board_id*)fdt_getprop(dtb,off,"qcom,board-id",&board_len);
	if(msm_id&&(msm_len%sizeof(qcom_msm_id))==0){
		uint32_t pid=fdt32_to_cpu(msm_id->platform_id);
		info->soc_rev=fdt32_to_cpu(msm_id->soc_rev);
		info->soc_id=pid&SOC_MASK;
		info->foundry_id=pid&FOUNDRY_ID_MASK;
	}
	if(board_id&&(board_len%sizeof(qcom_board_id))==0){
		uint32_t vid=fdt32_to_cpu(board_id->variant_id);
		uint32_t sub=fdt32_to_cpu(board_id->platform_subtype);
		info->variant_major=vid&VARIANT_MAJOR_MASK;
		info->variant_minor=vid&VARIANT_MINOR_MASK;
		info->variant_id=vid&VARIANT_MASK;
		info->subtype_id=sub&PLATFORM_SUBTYPE_MASK;
		info->subtype_ddr=sub&DDR_MASK;
	}
	return 0;
}

int qcom_dump_info(qcom_chip_info*info){
	tlog_verbose(
		"msm soc id: 0x%03llx rev: 0x%05llx foundry id: 0x%02llx",
		(unsigned long long)info->soc_id,
		(unsigned long long)info->soc_rev,
		(unsigned long long)info->foundry_id
	);
	tlog_verbose(
		"board variant id: 0x%02llx major: 0x%02llx minor: 0x%02llx",
		(unsigned long long)info->variant_id,
		(unsigned long long)info->variant_major,
		(unsigned long long)info->variant_minor
	);
	tlog_verbose(
		"board subtype id: 0x%02llx ddr: 0x%02llx",
		(unsigned long long)info->subtype_id,
		(unsigned long long)info->subtype_ddr
	);
	return 0;
}

static int locate_info(qcom_chip_info*info){
	EFI_STATUS st;
	ChipInfoIdType id;
	ChipInfoVersionType ver;
	ChipInfoFoundryIdType foundry;
	PLATFORM_INFO_TYPE plat;
	CHIPINFO_PROTOCOL*cp=NULL;
	PLATFORM_INFO_PROTOCOL*pi=NULL;
	st=gBS->LocateProtocol(&gEfiChipInfoProtocolGuid,NULL,(VOID**)&cp);
	if(EFI_ERROR(st))return trlog_warn(-1,
		"locate chip info protocol failed: %s"
		" (not qualcomm platform or forget ChipInfoDxe?)",
		efi_status_to_string(st)
	);
	tlog_verbose("handle chip info protocol %p",cp);
	st=gBS->LocateProtocol(&gEfiPlatformInfoProtocolGuid,NULL,(VOID**)&pi);
	if(EFI_ERROR(st))return trlog_warn(-1,
		"locate platform info protocol failed: %s"
		" (not qualcomm platform or forget PlatformInfoDxe?)",
		efi_status_to_string(st)
	);
	tlog_verbose("handle platform info protocol %p",pi);
	st=cp->GetChipId(cp,&id);
	if(EFI_ERROR(st))return trlog_warn(-1,
		"get chip id failed: %s",
		efi_status_to_string(st)
	);
	st=cp->GetChipVersion(cp,&ver);
	if(EFI_ERROR(st))return trlog_warn(-1,
		"get chip version failed: %s",
		efi_status_to_string(st)
	);
	st=cp->GetFoundryId(cp,&foundry);
	if(EFI_ERROR(st))return trlog_warn(-1,
		"get foundry id failed: %s",
		efi_status_to_string(st)
	);
	st=pi->GetPlatformInfo(pi,&plat);
	if(EFI_ERROR(st))return trlog_warn(-1,
		"get foundry id failed: %s",
		efi_status_to_string(st)
	);
	info->soc_id=id&SOC_MASK;
	info->soc_rev=ver;
	info->foundry_id=foundry<<PLATFORM_FOUNDRY_SHIFT;
	info->variant_major=plat.platform&VARIANT_MAJOR_MASK;
	info->variant_minor=plat.platform&VARIANT_MINOR_MASK;
	info->variant_id=plat.platform&VARIANT_MASK;
	info->subtype_id=plat.subtype&PLATFORM_SUBTYPE_MASK;
	info->subtype_ddr=plat.subtype&DDR_MASK;
	return 0;
}

int qcom_get_chip_info(linux_boot*lb,qcom_chip_info*info){
	static qcom_chip_info local;
	static bool locate=false;
	ZeroMem(info,sizeof(qcom_chip_info));
	if(lb->config&&lb->config->info.soc_id>0){
		tlog_debug("use chipinfo from config");
		qcom_dump_info(&lb->config->info);
		CopyMem(info,&lb->config->info,sizeof(qcom_chip_info));
		return 0;
	}
	if(!locate){
		if(locate_info(&local)!=0)return -1;
		qcom_dump_info(&local);
		locate=true;
	}
	CopyMem(info,&local,sizeof(qcom_chip_info));
	return 0;
}

int64_t qcom_check_dtb(qcom_chip_info*dtb,qcom_chip_info*chip){
	int64_t vote=0;
	// soc id not match
	if(dtb->soc_id!=chip->soc_id){
		tlog_verbose(
			"chip id not match dtb %llx chip %llx",
			(unsigned long long)dtb->soc_id,
			(unsigned long long)chip->soc_id
		);
		vote-=chip->soc_id;
	}

	// newer than this soc
	if(dtb->soc_rev>chip->soc_rev){
		tlog_verbose(
			"soc rev not match dtb %llx chip %llx",
			(unsigned long long)dtb->soc_rev,
			(unsigned long long)chip->soc_rev
		);
		vote-=chip->soc_rev;
	}

	// greater than this foundry
	if(dtb->foundry_id>chip->foundry_id){
		tlog_verbose(
			"foundry id mismatch dtb %llx chip %llx",
			(unsigned long long)dtb->foundry_id,
			(unsigned long long)chip->foundry_id
		);
		vote-=chip->foundry_id;
	}

	// greater than this variant major
	if(dtb->variant_major>chip->variant_major){
		tlog_verbose(
			"variant major mismatch dtb %llx chip %llx",
			(unsigned long long)dtb->variant_major,
			(unsigned long long)chip->variant_major
		);
		vote-=chip->variant_major;
	}

	// greater than this variant major
	if(dtb->variant_minor>chip->variant_minor){
		tlog_verbose(
			"variant minor mismatch dtb %llx chip %llx",
			(unsigned long long)dtb->variant_minor,
			(unsigned long long)chip->variant_minor
		);
		vote-=chip->variant_minor;
	}

	// board variant id not match
	if(
		dtb->variant_id!=0&&
		dtb->variant_id!=chip->variant_id
	){
		tlog_verbose(
			"variant id not match dtb %llx chip %llx",
			(unsigned long long)dtb->variant_id,
			(unsigned long long)chip->variant_id
		);
		vote-=chip->variant_id;
	}

	if(vote<0){
		tlog_verbose(
			"too few votes (%lld), return",
			(long long)vote
		);
		return vote;
	}

	vote+=chip->soc_rev;
	vote+=chip->foundry_id;
	vote+=chip->variant_major;
	vote+=chip->variant_minor;
	vote+=chip->subtype_ddr;
	if(dtb->soc_rev!=chip->soc_rev){
		tlog_verbose(
			"soc rev is not expect match dtb %llx chip %llx",
			(unsigned long long)dtb->soc_rev,
			(unsigned long long)chip->soc_rev
		);
		vote-=dtb->soc_rev;
	}
	if(dtb->foundry_id!=chip->foundry_id){
		tlog_verbose(
			"foundry id is not expect match dtb %llx chip %llx",
			(unsigned long long)dtb->foundry_id,
			(unsigned long long)chip->foundry_id
		);
		vote-=dtb->foundry_id;
	}
	if(dtb->variant_major!=chip->variant_major){
		tlog_verbose(
			"variant major is not expect match dtb %llx chip %llx",
			(unsigned long long)dtb->variant_major,
			(unsigned long long)chip->variant_major
		);
		vote-=dtb->variant_major;
	}
	if(dtb->variant_minor!=chip->variant_minor){
		tlog_verbose(
			"variant minor is not expect match dtb %llx chip %llx",
			(unsigned long long)dtb->variant_minor,
			(unsigned long long)chip->variant_minor
		);
		vote-=dtb->variant_minor;
	}
	if(dtb->subtype_ddr!=chip->subtype_ddr){
		tlog_verbose(
			"subtype ddr is not expect match dtb %llx chip %llx",
			(unsigned long long)dtb->subtype_ddr,
			(unsigned long long)chip->subtype_ddr
		);
		vote-=dtb->subtype_ddr;
	}
	tlog_verbose(
		"final votes: %lld",
		(long long)vote
	);
	return vote;
}
