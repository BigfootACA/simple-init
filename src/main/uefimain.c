/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_UEFI
#include<Library/PcdLib.h>
#include<Library/DebugLib.h>
#include<Library/UefiBootManagerLib.h>
#include<Library/ReportStatusCodeLib.h>
#ifdef ENABLE_LUA
#include"xlua.h"
#endif
#include"boot.h"
#include"confd.h"
#include"errno.h"
#include"logger.h"
#include"setjmp.h"
#include"language.h"
#include"gui.h"
#include"gui/splash.h"
#define TAG "main"

int main_retval=0;
jmp_buf main_exit;
extern int bootmenu_draw();

static void show_bootmenu(void*d __attribute__((unused))){
	bootmenu_draw();
}

static int post_main(){
	int r;
	logger_set_console(PcdGetBool(PcdLoggerdUseConsole));
	confd_init();
	logger_init();
	tlog_notice("initialize simple-init");
	tlog_debug("simple init main entry point at %p",&UefiMain);
	if((r=gui_pre_init())!=0)return r;
	if((r=gui_screen_init())!=0)return r;
	gui_splash_draw();
	lv_task_handler();

	gui_splash_set_text(true,_("Initializing config store..."));
	confd_dump(LEVEL_VERBOSE);

	gui_splash_set_text(true,_("Loading extra UEFI Drivers..."));
	boot_load_drivers();

	gui_splash_set_text(true,_("Initializing I18N locale..."));
	char*lang=confd_get_string("language",NULL);
	if(lang)lang_set(lang);
	lang_init_locale();

	gui_splash_set_text(true,_("Initializing Boot Manager..."));
	boot_init_configs();

	#ifdef ENABLE_LUA
	gui_splash_set_text(true,_("Initializing LUA Framework..."));
	lua_State*L=xlua_init();
	if(L){
		xlua_run_confd(L,TAG,"lua.on_post_startup");
		lua_close(L);
	}
	#endif

	gui_splash_set_text(true,_("Starting boot menu..."));
	gui_splash_exit(true,show_bootmenu);
	if((r=gui_main())!=0)return r;
	return 0;
}

// simple-init uefi entry point
EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ih,IN EFI_SYSTEM_TABLE*st){
	REPORT_STATUS_CODE(EFI_PROGRESS_CODE,(EFI_SOFTWARE_DXE_BS_DRIVER|EFI_SW_PC_USER_SETUP));

	EfiBootManagerConnectAll();
	EfiBootManagerRefreshAllBootOption();
	DEBUG((EFI_D_INFO,"Initialize SimpleInit GUI...\n"));

	errno=0;
	main_retval=0;
	if(setjmp(main_exit)==0)
		main_retval=post_main();
	return main_retval;
}
#endif
