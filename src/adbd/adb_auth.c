/*
 *
 * Copyright 2008, The Android Open Source Project
 *
 */

#include<stdio.h>
#include<string.h>
#include"adbd_internal.h"
#include"pathnames.h"
#include"logger.h"
#define TAG "adbd"
static fdevent listener_fde;
static int framework_fd=-1;
int adb_auth_generate_token(void *token,size_t token_size){
	FILE *f;
	int ret;
	if(!(f=fopen(_PATH_DEV_URANDOM,"r")))return 0;
	ret=fread(token,token_size,1,f);
	fclose(f);
	return ret*token_size;
}
int adb_auth_verify(
	void *token __attribute__((unused)),
	void *sig __attribute__((unused)),
	int siglen __attribute__((unused))
){
	return 0;
}
static void adb_auth_event(int fd,unsigned events,void *data){
	atransport *t=data;
	char response[2];
	int ret;
	if(!(events&FDE_READ))return;
	if((ret=adb_read(fd,response,sizeof(response)))<0){
		tlog_info("auth: disconnect");
		fdevent_remove(&t->auth_fde);
		framework_fd=-1;
	}else if(ret==2&&response[0]=='O'&&response[1]=='K'){
		adb_auth_verified(t);
	}
}
void adb_auth_confirm_key(unsigned char *key,size_t len,atransport *t){
	char msg[MAX_PAYLOAD];
	int ret;
	if(framework_fd<0){
		tlog_warn("auth: client not connected");
		return;
	}
	if(key[len-1]!='\0'){
		tlog_warn("auth: key must be a null-terminated string");
		return;
	}
	if((ret=snprintf(msg,sizeof(msg),"PK%s",key))>=(signed)sizeof(msg)){
		tlog_warn("auth: key too long: %d",ret);
		return;
	}
	tlog_debug("auth: sending '%s'\n",msg);
	if(adb_write(framework_fd,msg,ret)<0) {
		telog_warn("auth: failed to write PK");
		return;
	}
	fdevent_install(&t->auth_fde,framework_fd,adb_auth_event,t);
	fdevent_add(&t->auth_fde,FDE_READ);
}
static void adb_auth_listener(
	int fd __attribute__((unused)),
	unsigned events __attribute__((unused)),
	void *data __attribute__((unused))
){
	struct sockaddr addr;
	socklen_t alen;
	int s;
	alen=sizeof(addr);
	if((s=adb_socket_accept(fd,&addr,&alen))<0) {
		telog_warn("auth: failed to accept");
		return;
	}
	framework_fd=s;
}
void adb_auth_init(void){
	int fd;
	if((fd=android_get_control_socket("adbd"))<0){
		telog_warn("failed to get adbd socket");
		return;
	}
	if(listen(fd,4)<0){
		telog_warn("failed to listen on '%d'",fd);
		return;
	}
	fdevent_install(&listener_fde,fd,adb_auth_listener,NULL);
	fdevent_add(&listener_fde,FDE_READ);
}
