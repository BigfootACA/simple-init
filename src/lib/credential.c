#include<pwd.h>
#include<grp.h>
#include<stdio.h>
#include<string.h>

char*get_username(uid_t uid,char*buff,size_t size){
	memset(buff,0,size);
	struct passwd*pw=getpwuid(uid);
	if(pw)strncpy(buff,pw->pw_name,size);
	else if(uid==0)strncpy(buff,"root",size);
	else snprintf(buff,size,"%d",uid);
	return buff;
}

char*get_groupname(gid_t gid,char*buff,size_t size){
	memset(buff,0,size);
	struct group*gr=getgrgid(gid);
	if(gr)strncpy(buff,gr->gr_name,size);
	else if(gid==0)strncpy(buff,"root",size);
	else snprintf(buff,size,"%d",gid);
	return buff;
}
