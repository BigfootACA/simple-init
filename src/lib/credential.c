#include<pwd.h>
#include<grp.h>
#include<stdio.h>
#include<string.h>
#include"defines.h"
#include"pathnames.h"
#include"system.h"

char*get_username(uid_t uid,char*buff,size_t size){
	memset(buff,0,size);
	struct passwd*pw=getpwuid(uid);
	if(pw)strncpy(buff,pw->pw_name,size-1);
	else if(uid==0)strncpy(buff,"root",size-1);
	else snprintf(buff,size-1,"%d",uid);
	return buff;
}

char*get_groupname(gid_t gid,char*buff,size_t size){
	memset(buff,0,size);
	struct group*gr=getgrgid(gid);
	if(gr)strncpy(buff,gr->gr_name,size-1);
	else if(gid==0)strncpy(buff,"root",size-1);
	else snprintf(buff,size-1,"%d",gid);
	return buff;
}

char*get_commname(pid_t pid,char*buff,size_t size,bool with_pid){
	if(pid<=0)return NULL;
	if(read_file(
		buff,BUFSIZ,false,
		_PATH_PROC"/%d/comm",pid
	)<=0)snprintf(buff,size-1,"%d",pid);
	else if(with_pid){
		char p[BUFSIZ]={0};
		snprintf(p,BUFSIZ,"[%d]",pid);
		strncat(buff,p,size-1);
	}
	return buff;
}
