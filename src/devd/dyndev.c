#include<errno.h>
#include<unistd.h>
#define TAG "devnoded"
#include"devd.h"
#include"uevent.h"
#include"logger.h"

int process_new_node(int devdfd,uevent*event){
	if(
		!event->action||
		!event->devname||
		event->major<0||
		event->minor<0||
		devdfd<0
	)return -1;
	switch(event->action){
		case ACTION_ADD:return init_devtmpfs(_PATH_DEV);
		case ACTION_REMOVE:
			if(faccessat(devdfd,event->devname,F_OK,0)!=0)return -errno;
			if(unlinkat(
				devdfd,
				event->devname,
				0
			)!=0)return return_logger_printf_warning(
					-errno,TAG,
					"unlink %s/%s: %m",
					_PATH_DEV,
					event->devname
				);
			return return_logger_printf_debug(
				0,TAG,
				"remove device %s/%s",
				_PATH_DEV,event->devname
			);
		default:return -2;
	}
}
