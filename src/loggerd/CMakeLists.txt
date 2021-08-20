add_library(init_loggerd STATIC
	buffer.c
	client.c
	file_logger.c
	internal.c
	klog.c
	printk_logger.c
	server.c
	syslog.c
	syslog_logger.c
	lib.c
)
