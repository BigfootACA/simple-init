/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<pwd.h>
#include<grp.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<stddef.h>
#include<string.h>
#include<unistd.h>
#include<shadow.h>
#include<termios.h>
#include<sys/select.h>
#include<sys/utsname.h>
#include"logger.h"
#include"version.h"
#include"ttyd_internal.h"
#define TAG "login"
#define USER_LENGTH 32

static void print_prompt(){
	struct utsname u;
	char*h=uname(&u)==0?u.nodename:NAME;
	write(STDOUT_FILENO,h,strlen(h));
	write(STDOUT_FILENO," login: ",8);
}

static bool _read_username_char(struct tty_data*data,size_t*s,char*buf){
	char c;
	errno=0;
	ssize_t r=read(STDIN_FILENO,&c,1);
	if(errno==EAGAIN)return false;
	if(r<1)exit(0);
	switch(c){
		case '\r':
		case '\n':
			buf[*s]='\0',data->eol=c;
			write(STDOUT_FILENO,"\r\n",2);
		return false;
		case CTL('H'):
		case 0x7f:
			data->attrs.c_cc[VERASE]=c;
			// fallthrough
		case CTL('U'):
			if(*s>0&&*s<USER_LENGTH+1){
				write(STDOUT_FILENO,"\010 \010",3);
				buf[--(*s)]=0;
			}
		break;
		case CTL('C'):
		case CTL('D'):
			exit(0);
		default:
			if(c<' '||*s>=USER_LENGTH)break;
			write(STDOUT_FILENO,&c,1);
			buf[(*s)++]=c;
		break;
	}
	return true;
}

static void _read_username(struct tty_data*data,char*buf){
	fd_set fs;
	size_t s=0;
	struct timeval timeout;
	print_prompt();
	memset(buf,0,USER_LENGTH+1);
	data->eol=0;
	do{
		FD_ZERO(&fs);
		FD_SET(STDIN_FILENO,&fs);
		timeout.tv_sec=1,timeout.tv_usec=0;
		int r=select(STDIN_FILENO+1,&fs,NULL,NULL,&timeout);
		if(r<0)exit(0);
		if(r==0)continue;
		while(_read_username_char(data,&s,buf));
	}while(data->eol==0);
}

char*tty_read_username(struct tty_data*data){
	static char buf[USER_LENGTH+1];
	usleep(100000);
	tcflush(STDIN_FILENO,TCIFLUSH);
	do{_read_username(data,buf);}while(buf[0]==0);
	return buf;
}

static char*tty_read_pass(char*buf,size_t len){
	size_t off=0;
	ssize_t s;
	char*p;
	struct termios tio,oldtio;
	write(STDOUT_FILENO,"Password: ",10);
	tcflush(STDOUT_FILENO,TCIFLUSH);
	tcgetattr(STDIN_FILENO,&oldtio);
	tio=oldtio,tio.c_lflag&=~(ECHO|ECHOE|ECHOK|ECHONL);
	tcsetattr(STDIN_FILENO,TCSANOW,&tio);
	memset(buf,0,len);
	while(off<len-1){
		p=buf+off;
		s=read(STDIN_FILENO,p,len-off-1);
		if(s<=0)break;
		if(*p=='\r'||*p=='\n'){
			*p=0;
			break;
		}
		off+=s;
	}
	tcsetattr(STDIN_FILENO,TCSANOW,&oldtio);
	write(STDOUT_FILENO,"\n",1);
	return (off==0||off>=len)?NULL:buf;
}

bool tty_ask_pwd(struct tty_data*data,char*username){
	if(!data||!username)return false;
	struct group*g;
	char*enc,pass[BUFSIZ];
	const char*r;
	errno=0;
	struct passwd*pw=getpwnam(username);
	if(!pw)telog_debug("%s lookup username '%s' failed",data->name,username);
	else{
		switch(pw->pw_passwd[0]){
			case '!':case '*':return false;
		}
		r=pw->pw_passwd;
		if(!r||((r[0]=='x'||r[0]=='*')&&!r[1])){
			struct spwd*s=getspnam(pw->pw_name);
			r=s?s->sp_pwdp:"aa";
		}
		if(!r)return false;
		if(!r[0])goto success;
	}
	tty_read_pass(pass,BUFSIZ);
	if(pw){
		enc=crypt(pass,r);
		if(!enc||!*enc){
			tlog_warn("%s login user %s invalid password",data->name,username);
			return false;
		}
		if(strcmp(enc,r)==0)goto success;
	}
	return false;
	success:
	if((g=getgrgid(pw->pw_gid)))strncpy(data->group,g->gr_name,sizeof(data->group)-1);
	strncpy(data->user,username,sizeof(data->user)-1);
	strncpy(data->shell,pw->pw_shell,sizeof(data->shell)-1);
	strncpy(data->home,pw->pw_dir,sizeof(data->home)-1);
	data->uid=pw->pw_uid,data->gid=pw->pw_gid;
	tlog_debug("user %s login from %s",username,data->name);
	return true;
}

bool tty_login(struct tty_data*data){
	if(!data)return false;
	for(int i=0;i<3;i++){
		tcflush(STDIN_FILENO,TCIFLUSH);
		char*user=tty_confd_get_string(data,"username",NULL);
		if(user)printf("autologin with %s",user);
		else user=tty_read_username(data);
		if(user){
			if(tty_confd_get_boolean(data,"no_password",false))return true;
			if(tty_ask_pwd(data,user))return true;
		}
		sleep(5);
		tlog_warn("%s login user %s login incorrect",data->name,user);
		write(STDOUT_FILENO,"Login incorrect\n\n",17);
	}
	return false;
}
