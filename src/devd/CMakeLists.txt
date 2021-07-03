add_library(init_devd STATIC
	devd.c
	devtmpfs.c
	dyndev.c
	firmware.c
	hotplug.c
	internal.c
	modalias.c
	netlink.c
	server.c
	uevent.c
	modules_load.c
)
