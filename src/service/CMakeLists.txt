add_library(init_service STATIC
	struct.c
	execute.c
	service.c
	scheduler.c
	data.c
	sigchld.c
	start.c
	stop.c
	queue.c
	default.c
	string.c
	dump.c
	conf.c
)
