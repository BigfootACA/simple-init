/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_UEFI
#include<Library/DebugLib.h>
#include<Library/UefiBootManagerLib.h>
#include<Library/ReportStatusCodeLib.h>
#include"confd.h"
#include"errno.h"
#include"setjmp.h"
#include"language.h"

int main_retval=0;
jmp_buf main_exit;
extern int bootmenu_main(int argc,char**argv);

// simple-init uefi entry point
EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ih,IN EFI_SYSTEM_TABLE*st){
	REPORT_STATUS_CODE(EFI_PROGRESS_CODE,(EFI_SOFTWARE_DXE_BS_DRIVER|EFI_SW_PC_USER_SETUP));

	// init uefi boot manager
	EfiBootManagerConnectAll();
	EfiBootManagerRefreshAllBootOption();
	DEBUG((EFI_D_INFO,"Initialize SimpleInit GUI...\n"));

	confd_load_file(NULL,NULL);
	char*lang=confd_get_string("language",NULL);
	if(lang)lang_set(lang);
	lang_init_locale();

	errno=0;
	main_retval=0;
	if(setjmp(main_exit)==0)
		main_retval=bootmenu_main(1,(char*[]){"bootmenu",NULL});
	return main_retval;
}
#endif
