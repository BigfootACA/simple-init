add_library(init_filesystem STATIC
	oper.c
	file.c
	utils.c
	locked.c
	string.c
	volume.c
	drivers.c
	internal.c
	layer/template.c
	layer/overlay.c
	layer/assets.c
	layer/zip.c
)
