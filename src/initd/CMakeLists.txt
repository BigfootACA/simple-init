add_library(init_initd STATIC
	environ.c
	init.c
	logfs.c
	preinit.c
	reboot.c
	run.c
	signals.c
	umount.c
	protocol.c
	socket.c
)
