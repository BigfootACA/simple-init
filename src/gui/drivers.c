#include"hardware.h"
extern struct gui_driver guidrv_gtk;
extern struct gui_driver guidrv_sdl2;
extern struct gui_driver guidrv_drm;
extern struct gui_driver guidrv_fbdev;
struct gui_driver*gui_drvs[]={
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
	NULL
};
