add_library(init_adbd STATIC
	adb.c
	adb_auth.c
	fdevent.c
	file_sync.c
	services.c
	socket_local.c
	sockets.c
	transport.c
	usb.c
	gadget.c
)
