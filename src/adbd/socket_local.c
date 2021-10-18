/*
 *
 * Copyright 2008, The Android Open Source Project
 *
 */

#include<string.h>
#include<unistd.h>
#include<stddef.h>
#include<sys/un.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include"adbd_internal.h"
int socket_make_sockaddr_un(const char*name,int ns,struct sockaddr_un*p_addr,socklen_t*alen){
	memset(p_addr,0,sizeof(*p_addr));
	size_t namelen;
	switch(ns){
		case ANDROID_SOCKET_NAMESPACE_ABSTRACT:
			namelen=strlen(name);
			if((namelen+1)>sizeof(p_addr->sun_path))return -1;
			p_addr->sun_path[0]=0;
			memcpy(p_addr->sun_path+1,name,namelen);
		break;
		case ANDROID_SOCKET_NAMESPACE_RESERVED:
			namelen=strlen(name)+strlen("/run");
			if(namelen>sizeof(*p_addr)-offsetof(struct sockaddr_un,sun_path)-1)return -1;
			strcpy(p_addr->sun_path,"/run");
			strcat(p_addr->sun_path,name);
		break;
		case ANDROID_SOCKET_NAMESPACE_FILESYSTEM:
			namelen=strlen(name);
			if(namelen>sizeof(*p_addr)-offsetof(struct sockaddr_un,sun_path)-1)return -1;
			strcpy(p_addr->sun_path,name);
		break;
		default:return -1;
	}
	p_addr->sun_family=AF_LOCAL;
	*alen=namelen+offsetof(struct sockaddr_un,sun_path)+1;
	return 0;
}
int socket_local_client_connect(int fd,const char*name,int ns,int type __attribute__((unused))){
	struct sockaddr_un addr;
	socklen_t alen;
	if(socket_make_sockaddr_un(name,ns,&addr,&alen)<0)return -1;
	return connect(fd,(struct sockaddr*)&addr,alen)<0?-1:fd;
}
int socket_local_client(const char*name,int ns,int type){
	int s;
	if((s=socket(AF_LOCAL,type,0))<0)return -1;
	if(socket_local_client_connect(s,name,ns,type)<0){
		close(s);
		return -1;
	}
	return s;
}
int socket_local_server_bind(int s,const char *name,int namespaceId){
	struct sockaddr_un addr;
	socklen_t alen;
	int n;
	if(socket_make_sockaddr_un(name,namespaceId,&addr,&alen)<0)return -1;
	unlink(addr.sun_path);
	n=1;
	setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&n,sizeof(n));
	return bind(s,(struct sockaddr*)&addr,alen)<0?-1:s;
}
int socket_local_server(const char*name,int namespace,int type){
	int s;
	if((s=socket(AF_LOCAL,type,0))<0)return -1;
	if(socket_local_server_bind(s,name,namespace)<0){
		close(s);
		return -1;
	}
	if(type!=SOCK_STREAM)return s;
	if(listen(s,LISTEN_BACKLOG)<0){
		close(s);
		return -1;
	}
	return s;
}
int socket_loopback_client(int port,int type){
	struct sockaddr_in addr;
	int s;
	memset(&addr,0,sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
	if((s=socket(AF_INET,type,0))<0)return -1;
	if(connect(s,(struct sockaddr*)&addr,sizeof(addr))<0){
		close(s);
		return -1;
	}
	return s;
}
int socket_loopback_server(int port,int type){
	struct sockaddr_in addr;
	int s,n;
	memset(&addr,0,sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
	if((s=socket(AF_INET,type,0))<0)return -1;
	n=1;
	setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&n,sizeof(n));
	if(bind(s,(struct sockaddr*) &addr,sizeof(addr)) < 0) {
		close(s);
		return -1;
	}
	if(type!=SOCK_STREAM)return s;
	if(listen(s,LISTEN_BACKLOG)<0){
		close(s);
		return -1;
	}
	return s;
}
int socket_inaddr_any_server(int port,int type){
	struct sockaddr_in addr;
	int s,n;
	memset(&addr,0,sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=htonl(INADDR_ANY);
	if((s=socket(AF_INET,type,0))<0)return -1;
	n=1;
	setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&n,sizeof(n));
	if(bind(s,(struct sockaddr*)&addr,sizeof(addr))<0){
		close(s);
		return -1;
	}
	if(type!=SOCK_STREAM)return s;
	if(listen(s,LISTEN_BACKLOG)<0){
		close(s);
		return -1;
	}
	return s;
}
