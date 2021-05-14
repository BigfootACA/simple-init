#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include"logger.h"
#include"uevent.h"
#include"system.h"
#include"defines.h"
#include"devd.h"
#include"init.h"
#define TAG "devd"

static int devdfd=-1;

int process_module(uevent*event){
	if(!event->devpath)return -1;
	char*mod=strchr(event->devpath+1,'/')+1;
	switch(event->action){
		case ACTION_ADD:tlog_debug("loaded module '%s'",mod);break;
		case ACTION_REMOVE:tlog_debug("unloaded module '%s'",mod);break;
		default:break;
	}
	return 0;
}

int process_uevent(){
	uevent event;
	uevent_parse_x(environ,&event);
	if(strcmp(event.subsystem,"firmware")==0)process_firmware_load(&event);
	if(strcmp(event.subsystem,"module")==0)process_module(&event);
	if(event.major>=0&&event.minor>=0)process_new_node(devdfd,&event);
	#ifdef ENABLE_KMOD
	if(event.modalias)insmod(event.modalias,false);
	#endif
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
