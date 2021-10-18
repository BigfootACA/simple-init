/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<stdlib.h>
#include"gui/guidrv.h"
extern struct gui_driver guidrv_gtk;
extern struct gui_driver guidrv_sdl2;
extern struct gui_driver guidrv_drm;
extern struct gui_driver guidrv_vnc;
extern struct gui_driver guidrv_fbdev;
extern struct gui_driver guidrv_uefigop;
struct gui_driver*gui_drvs[]={
	#ifdef ENABLE_UEFI
	&guidrv_uefigop,
	#else
	#ifdef ENABLE_GTK
	&guidrv_gtk,
	#endif
	#ifdef ENABLE_SDL2
	&guidrv_sdl2,
	#endif
	#ifdef ENABLE_VNCSERVER
	&guidrv_vnc,
	#endif
	#ifdef ENABLE_DRM
	&guidrv_drm,
	#endif
	&guidrv_fbdev,
	#endif
	NULL
};
#endif
