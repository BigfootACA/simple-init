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
extern struct gui_driver guidrv_dummy;
extern struct gui_driver guidrv_gtk;
extern struct gui_driver guidrv_sdl2;
extern struct gui_driver guidrv_drm;
extern struct gui_driver guidrv_vnc;
extern struct gui_driver guidrv_http;
extern struct gui_driver guidrv_fbdev;
extern struct gui_driver guidrv_uefigop;
extern struct gui_driver guidrv_uefiuga;
extern struct input_driver indrv_uefi_kbd;
extern struct input_driver indrv_uefi_touch;
extern struct input_driver indrv_uefi_pointer;
extern struct input_driver indrv_sdl2_kbd;
extern struct input_driver indrv_sdl2_mse;
extern struct input_driver indrv_sdl2_whl;
extern struct input_driver indrv_gtk_kbd;
extern struct input_driver indrv_gtk_mse;
extern struct input_driver indrv_vnc;
extern struct input_driver indrv_http;
extern struct input_driver indrv_stdin;
extern struct input_driver indrv_event;
struct gui_driver*gui_drvs[]={
	#ifdef ENABLE_UEFI
	&guidrv_uefigop,
	&guidrv_uefiuga,
	#else
	#ifdef ENABLE_GTK
	&guidrv_gtk,
	#endif
	#ifdef ENABLE_SDL2
	&guidrv_sdl2,
	#endif
	#ifdef ENABLE_DRM
	&guidrv_drm,
	#endif
	&guidrv_fbdev,
	#ifdef ENABLE_VNCSERVER
	&guidrv_vnc,
	#endif
	#ifdef ENABLE_MICROHTTPD
	&guidrv_http,
	#endif
	#endif
	&guidrv_dummy,
	NULL
};
struct input_driver*input_drvs[]={
	#ifdef ENABLE_UEFI
	&indrv_uefi_kbd,
	&indrv_uefi_touch,
	&indrv_uefi_pointer,
	#else
	#ifdef ENABLE_GTK
	&indrv_gtk_kbd,
	&indrv_gtk_mse,
	#endif
	#ifdef ENABLE_SDL2
	&indrv_sdl2_kbd,
	&indrv_sdl2_mse,
	&indrv_sdl2_whl,
	#endif
	#ifdef ENABLE_VNCSERVER
	&indrv_vnc,
	#endif
	#ifdef ENABLE_MICROHTTPD
	&indrv_http,
	#endif
	&indrv_stdin,
	&indrv_event,
	#endif
	NULL
};
#endif
