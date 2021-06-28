#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include"logger.h"
#include"gadget.h"
#define TAG "gadget"

int gadget_register(gadget*g){
	int o=open_usb_gadget();
	if(o<0)return o;
	int r=gadget_register_fd(o,g);
	return r;
}

int gadget_register_fd(int dir_fd,gadget*g){
	int main=-1;
	if(!(g&&g->name))return -1;
	if(mkdirat(dir_fd,g->name,0755)<0)goto er;
	if((main=openat(dir_fd,g->name,O_RDONLY|O_DIRECTORY))<0)goto er;
	g->dir_fd=main;
	if(gadget_write_info(g)<0)goto er;
	tlog_info("registered gadget device '%s'",g->name);
	return 0;
	er:
	telog_error("register gadget device '%s' failed",g->name);
	return -1;
}

int gadget_register_path(char*path,gadget*g){
	int d=open(path,O_RDONLY|O_DIRECTORY);
	return (d<0)?-1:gadget_register_fd(d,g);
}
