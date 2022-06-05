/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"init_internal.h"
#include"gadget.h"
#include"shell.h"
#include"boot.h"
#ifdef ENABLE_GUI
#include"gui.h"
#endif

extern int register_bootmenu();
extern int register_ttyd();

int(*register_services[])()={
	#ifdef ENABLE_READLINE
	&register_console_shell,
	#endif
	&register_default_boot,
	&register_ttyd,
	&register_gadget_service,
	#ifdef ENABLE_GUI
	&register_guiapp,
	#endif
	&register_bootmenu,
	NULL
};

int init_register_all_service(){
	for(int i=0;register_services[i];i++)
		register_services[i]();
	return 0;
}
