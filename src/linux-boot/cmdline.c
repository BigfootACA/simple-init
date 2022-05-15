/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<Library/UefiBootServicesTableLib.h>
#include<comp_libfdt.h>
#include"logger.h"
#include"internal.h"
#include"KernelFdt.h"
#define TAG "fdt"

static void load_kernel_fdt_cmdline(linux_boot*lb){
	int len=0;
	char*cmdline=NULL;
	EFI_STATUS st;
	KERNEL_FDT_PROTOCOL*fdt;
	if(lb->config->skip_kfdt_cmdline)return;
	if(lb->config->pass_kfdt_dtb)return;
	st=gBS->LocateProtocol(
		&gKernelFdtProtocolGuid,
		NULL,
		(VOID**)&fdt
	);
	if(EFI_ERROR(st)||!fdt||!fdt->Fdt)return;
	if(fdt_get_cmdline(fdt->Fdt,&cmdline,&len)!=0)return;
	if(len<=0||!cmdline)return;
	tlog_verbose("add cmdline from kernel fdt: '%s'",cmdline);
	linux_boot_append_cmdline(lb,cmdline);
}

int linux_boot_update_cmdline(linux_boot*lb){
	int r,off,len=0;
	char cmdline[4096];
	if(!lb->cmdline[0])return 0;
	r=fdt_path_offset(lb->dtb.address,"/chosen");
	if(r<0)return trlog_warn(
		-1,
		"get chosen node failed: %s",
		fdt_strerror(r)
	);
	off=r;
	load_kernel_fdt_cmdline(lb);
	ZeroMem(cmdline,sizeof(cmdline));
	const char*fdt_str=fdt_getprop(lb->dtb.address,r,"bootargs",&len);
	if(fdt_str)AsciiStrnCatS(cmdline,sizeof(cmdline),fdt_str,len);
	if(cmdline[0])AsciiStrCatS(cmdline,sizeof(cmdline)," ");
	AsciiStrCatS(cmdline,sizeof(cmdline),lb->cmdline);
	r=fdt_setprop_string(
		lb->dtb.address,
		off,"bootargs",cmdline
	);
	if(r<0)return trlog_warn(
		-1,
		"update bootargs failed: %s",
		fdt_strerror(r)
	);
	tlog_verbose("update fdt cmdline done: '%s'",cmdline);
	return 0;
}

void linux_boot_append_cmdline(linux_boot*lb,const char*cmdline){
	if(!lb||!cmdline||!cmdline[0])return;
	UINTN old=AsciiStrLen(lb->cmdline);
	UINTN avail=sizeof(lb->cmdline)-old;
	if(AsciiStrLen(cmdline)>=avail-1)tlog_warn("cmdline too long, skip");
	else{
		if(old>0){
			tlog_verbose("append cmdline '%s'",cmdline);
			lb->cmdline[old++]=' ',avail--;
		}else tlog_verbose("use cmdline '%s'",cmdline);
		AsciiStrCpyS(
			lb->cmdline+old,
			avail,cmdline
		);
	}
}
