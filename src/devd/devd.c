#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include"logger.h"
#include"uevent.h"
#include"system.h"
#include"defines.h"
#include"devd.h"
#define TAG "devd"

static int devdfd=-1;

int process_uevent(){
	uevent event;
	uevent_parse_x(environ,&event);
	if(strcmp(event.subsystem,"firmware")==0)process_firmware_load(&event);
	if(event.major>=0&&event.minor>=0)process_new_node(devdfd,&event);
	if(event.modalias)insmod(event.modalias,false);
	return 0;
}

int initdevd_main(int argc __attribute__((unused)),char**argv __attribute__((unused))){
	open_socket_logfd_default();
	devdfd=open(_PATH_DEV,O_RDONLY|O_DIRECTORY);
	if(devdfd<0)logger_printf_warning(TAG,"open %s: %m",_PATH_DEV);
	if(getenv("ACTION"))process_uevent();
	close(devdfd);
	return 0;
}
