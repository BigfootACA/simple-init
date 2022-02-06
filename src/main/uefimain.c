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

extern int guiapp_main(int argc,char**argv);
extern INTN EFIAPI ShellAppMain(UINTN argc,CHAR16**argv);

// main for uefi shell app
int main(int argc,char**argv){
	return guiapp_main(argc,argv);
}

// simple-init uefi entry point
EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ih,IN EFI_SYSTEM_TABLE*st){
	REPORT_STATUS_CODE(EFI_PROGRESS_CODE,(EFI_SOFTWARE_DXE_BS_DRIVER|EFI_SW_PC_USER_SETUP));

	// init uefi boot manager
	EfiBootManagerConnectAll();
	EfiBootManagerRefreshAllBootOption();
	DEBUG((EFI_D_INFO,"Initialize SimpleInit GUI...\n"));

	// call uefi shell app main to init edk2-libc
	static UINTN argc=1;
	static CHAR16*argv[]={L"guiapp",NULL};
	return ShellAppMain(argc,argv);
}
#endif
