add_library(init_gui STATIC
	gui_init.c
	guiapp.c
	guipm/guipm.c
	guipm/disk_select.c
	drivers/drm.c
	drivers/fbdev.c
	drivers/ts.c
	drivers/gtk.c
	drivers/sdl2.c
	filesystem.c
	drivers.c
	freetype.c
	guidrv.c
	sysbar.c
)
