#define _GNU_SOURCE
#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<sys/stat.h>
#define TAG "led"
#include"str.h"
#include"system.h"
#include"logger.h"
#include"pathnames.h"

int led_get_max_brightness(int fd){
	return fd_read_int(fd,"max_brightness");
}

int led_get_brightness(int fd){
	return fd_read_int(fd,"brightness");
}

int led_set_brightness(int fd,int value){
	return fd_write_int(fd,"brightness",value,true);
}

int led_get_brightness_percent(int fd){
	if(fd<0)ERET(EBADF);
	int max,cur;
	if((max=led_get_max_brightness(fd))<=0)return -1;
	if((cur=led_get_brightness(fd))<0)return -1;
	return (int)(((float)cur)/((float)max)*100);
}

int led_set_brightness_percent(int fd,int percent){
	if(percent<0||percent>100)ERET(EINVAL);
	if(fd<0)ERET(EBADF);
	int max=led_get_max_brightness(fd);
	return max>0?led_set_brightness(fd,max/100*percent):-1;
}

int led_set_brightness_percent_by_name(char*name,int percent){
	if(!name)return -1;
	int c,dir,r;
	struct stat s;
	char path[512]={0};

	for(c=0;name[c]!=0;c++)
		if(name[c]=='/'||strncmp(name+c,"..",2)==0)
			return -1;

	snprintf(path,511,_PATH_SYS_CLASS"/leds/%s",name);

	if(stat(path,&s)<0||!S_ISLNK(s.st_mode))
		return terlog_error(-1,"not a valid led device %s",name);

	if((dir=open(path,O_DIR))<0)
		return terlog_error(-1,"failed to open device %s",name);

	r=led_set_brightness_percent(dir,percent);
	close(dir);
	return r;
}
