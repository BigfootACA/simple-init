add_library(init_gui STATIC
	gui_init.c
	guiapp.c
	guipm/guipm.c
	guipm/disk_select.c
	drivers/drm.c
	drivers/fbdev.c
	drivers/input.c
	drivers/gtk.c
	drivers/sdl2.c
	decoders/freetype.c
	decoders/png.c
	filesystem.c
	drivers.c
	guidrv.c
	sysbar.c
	activity.c
)
