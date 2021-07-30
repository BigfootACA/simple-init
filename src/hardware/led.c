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

int set_brightness_percent(char*name,int percent){
	if(!name||percent<0||percent>100||strcmp(name,"..")==0)return -1;
	int c,dir=-1,dmax=-1,dcur=-1,r=-1;
	long max;
	struct stat s;
	char path[512]={0},buff[64]={0};

	for(c=0;name[c]!=0;c++)
		if(name[c]=='/'||strncmp(name+c,"..",2)==0)
			return -1;

	snprintf(path,511,_PATH_SYS_CLASS"/leds/%s",name);
	if(stat(path,&s)<0||!S_ISLNK(s.st_mode)){
		telog_error("not a valid led device %s",name);
		goto end;
	}

	if((dir=open(path,O_DIRECTORY))<0){
		telog_error("failed to open device %s",name);
		goto end;
	}

	if((dmax=openat(dir,"max_brightness",O_RDONLY))<0||read(dmax,buff,64)<=0){
		telog_error("failed to read max brightness of device %s",name);
		goto end;
	}

	if((max=parse_int(buff,-1))<0){
		telog_error("invalid max brightness of device %s",name);
		goto end;
	}
	memset(buff,0,63);
	snprintf(buff,63,"%ld\n",max/100*percent);
	c=(int)strlen(buff);
	if((dcur=openat(dir,"brightness",O_WRONLY))<0||write(dcur,buff,c)!=c){
		telog_error("failed to write brightness of device %s",name);
		goto end;
	}
	r=0;
	end:
	if(dmax>=0)close(dmax);
	if(dcur>=0)close(dcur);
	if(dir>=0)close(dir);
	return r;
}
