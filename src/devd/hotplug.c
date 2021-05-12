#include<errno.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include"defines.h"
#include"logger.h"
#define TAG "hotplug"

int set_hotplug(char*vlink){
	int fd;
	size_t s;
	char*hotplug,*exe,buff[PATH_MAX]={0};
	hotplug=_PATH_PROC_SYS"/kernel/hotplug";
	exe=_PATH_PROC_SELF"/exe";
	if(access(exe,R_OK)!=0)return terlog_error(-errno,"cannot read %s",exe);
	if(access(hotplug,W_OK)!=0)return errno==ENOENT?
		trlog_warn(-errno,"kernel seems disabled hotplug"):
		terlog_error(-errno,"cannot write %s: %m",hotplug);
	if(readlink(exe,buff,sizeof(buff))<0)return terlog_error(-errno,"readlink failed");
	if((s=strlen(buff))<=0)return -1;
	if((fd=open(hotplug,O_WRONLY))<0)return terlog_error(-errno,"open hotplug failed");
	if(vlink){
		if(symlink(buff,vlink)<0)return terlog_error(-errno,"create symlink failed");
		memset(buff,0,PATH_MAX);
		strncpy(buff,vlink,PATH_MAX-1);
		s=strlen(buff);
	}
	if(write(fd,buff,s)<0)return terlog_error(-errno,"write hotplug failed");
	close(fd);
	return trlog_info(0,"set hotplug helper to %s",buff);
}