/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<netdb.h>
#include<sys/un.h>
#include<sys/socket.h>
#include<sys/ioctl.h>
#include<arpa/inet.h>
#include<netinet/tcp.h>
#include"str.h"
#include"../fs_internal.h"

enum protocol{
	PROTO_NONE = 0x00000000,
	PROTO_IP   = 0x00000001,
	PROTO_UNIX = 0x00000002,
	PROTO_IPV4 = PROTO_IP|0x00000004,
	PROTO_IPV6 = PROTO_IP|0x00000008,
	PROTO_TCP  = 0x00010000,
	PROTO_UDP  = 0x00020000,
	PROTO_TCP4 = PROTO_IPV4|PROTO_TCP,
	PROTO_TCP6 = PROTO_IPV6|PROTO_TCP,
	PROTO_UDP4 = PROTO_IPV4|PROTO_UDP,
	PROTO_UDP6 = PROTO_IPV6|PROTO_UDP,
};

typedef union use_sockaddr{
	struct sockaddr addr;
	struct sockaddr_un un;
	struct sockaddr_in in4;
	struct sockaddr_in6 in6;
}use_sockaddr;

static const struct sock_options{
	enum option_type{
		OPT_TYPE_NONE,
		OPT_TYPE_BOOLEAN,
	}type;
	int level;
	int option;
	char*name;
}opts[]={
	{OPT_TYPE_BOOLEAN, SOL_SOCKET, SO_DEBUG,      "debug"},
	{OPT_TYPE_BOOLEAN, SOL_SOCKET, SO_ACCEPTCONN, "acceptconn"},
	{OPT_TYPE_BOOLEAN, SOL_SOCKET, SO_REUSEADDR,  "reuseaddr"},
	{OPT_TYPE_BOOLEAN, SOL_SOCKET, SO_REUSEPORT,  "reuseport"},
	{OPT_TYPE_BOOLEAN, SOL_SOCKET, SO_DONTROUTE,  "dontroute"},
	{OPT_TYPE_BOOLEAN, SOL_SOCKET, SO_BROADCAST,  "broadcast"},
	{OPT_TYPE_BOOLEAN, SOL_SOCKET, SO_KEEPALIVE,  "keepalive"},
	{OPT_TYPE_BOOLEAN, SOL_SOCKET, SO_OOBINLINE,  "oobinline"},
	{OPT_TYPE_BOOLEAN, SOL_SOCKET, SO_TIMESTAMP,  "timestamp"},
	{OPT_TYPE_BOOLEAN, SOL_TCP,    TCP_FASTOPEN,  "fastopen"},
	{OPT_TYPE_BOOLEAN, SOL_TCP,    TCP_NODELAY,   "timestamp"},
	{OPT_TYPE_NONE,0,0,NULL}
};

static const struct protocol_map{
	enum protocol proto;
	char*name;
}pmaps[]={
	{PROTO_IP|PROTO_TCP,"tcp"},
	{PROTO_IP|PROTO_UDP,"udp"},
	{PROTO_TCP4,"tcp4"},
	{PROTO_UDP4,"udp4"},
	{PROTO_TCP6,"tcp6"},
	{PROTO_UDP6,"udp6"},
	{PROTO_UNIX,"unix"},
	{PROTO_NONE,NULL}
};

static struct sockaddr*init_sockaddr(
	enum protocol protocol,
	url*uri,
	int*domain,
	socklen_t*len,
	use_sockaddr*addr
){
	if(protocol&PROTO_IPV4){
		addr->in4.sin_family=*domain=AF_INET;
		addr->in4.sin_port=htons(uri->port);
		*len=sizeof(addr->in4);
	}else if(protocol&PROTO_IPV6){
		addr->in6.sin6_family=*domain=AF_INET6;
		addr->in6.sin6_port=htons(uri->port);
		*len=sizeof(addr->in6);
	}else if(protocol&PROTO_UNIX){
		addr->un.sun_family=*domain=AF_UNIX;
		if(!uri->path)return NULL;
		strncpy(
			addr->un.sun_path,uri->path,
			sizeof(addr->un.sun_path)-1
		);
		*len=sizeof(addr->un);
	}else return NULL;
	return &addr->addr;
}

static struct sockaddr*try_addr_ip(
	enum protocol protocol,int*domain,url*uri,
	use_sockaddr*addr,socklen_t*len
){
	struct hostent*host;
	if(!uri->host||uri->port<0||!(protocol&PROTO_IP))return NULL;
	if(
		(protocol&PROTO_IPV6)==PROTO_IPV6&&
		inet_pton(AF_INET6,uri->host,&addr->in6.sin6_addr)>0
	)goto done;
	if(
		(protocol&PROTO_IPV4)==PROTO_IPV4&&
		((int)(addr->in4.sin_addr.s_addr=inet_addr(uri->host))!=-1)
	)goto done;
	if(
		(protocol&PROTO_IPV6)==PROTO_IPV6&&
		(host=gethostbyname2(uri->host,AF_INET6))
	){
		memcpy(&addr->in6.sin6_addr,host->h_addr,host->h_length);
		goto done;
	}
	if(
		(protocol&PROTO_IPV4)==PROTO_IPV4&&
		(host=gethostbyname2(uri->host,AF_INET))
	){
		memcpy(&addr->in4.sin_addr,host->h_addr,host->h_length);
		goto done;
	}
	tlog_warn("cannot resolve host %s",uri->host);
	errno=EDESTADDRREQ;
	return NULL;
	done:
	return init_sockaddr(protocol,uri,domain,len,addr);
}

static struct sockaddr*init_addr(
	enum protocol protocol,
	int*domain,int*proto,int*type,
	url*uri,use_sockaddr*addr,socklen_t*len
){
	if(!domain||!proto||!type)return NULL;
	if(!uri||!addr||!len)return NULL;
	memset(addr,0,sizeof(use_sockaddr));
	if(protocol&PROTO_UDP)*proto|=IPPROTO_UDP,*type|=SOCK_DGRAM;
	if(protocol&PROTO_TCP)*proto|=IPPROTO_TCP,*type|=SOCK_STREAM;
	if(protocol&PROTO_IP)return try_addr_ip(protocol,domain,uri,addr,len);
	if(protocol&PROTO_UNIX)return init_sockaddr(protocol,uri,domain,len,addr);
	return NULL;
}

static void sockopt_bool(
	int fd,
	keyval**options,
	int level,
	int opt,
	char*name
){
	char*p;
	int val;
	if(!(p=kvarr_get_value_by_key(
		options,name,NULL
	)))return;
	if(string_is_false(p))val=0;
	else if(string_is_true(p))val=1;
	else return;
	setsockopt(fd,level,opt,&val,sizeof(val));
}

static int fsdrv_socket_open(
	const fsdrv*drv,
	fsh*nf,
	url*uri,
	fs_file_flag flags
){
	errno=0;
	int fd,e;
	socklen_t len=0;
	use_sockaddr addr;
	keyval**os=NULL;
	enum protocol protocol=PROTO_NONE;
	int domain=0,proto=0,type=SOCK_CLOEXEC;
	if(!drv||!nf||!uri)RET(EINVAL);
	if(fs_has_flag(flags,FILE_FLAG_ACCESS))RET(ENOTSUP);
	if(fs_has_flag(flags,FILE_FLAG_FOLDER))RET(ENOTSUP);
	for(size_t i=0;pmaps[i].name;i++){
		if(strcmp(drv->protocol,pmaps[i].name)!=0)continue;
		protocol=pmaps[i].proto;
		break;
	}
	if(protocol==PROTO_NONE)RET(EPROTONOSUPPORT);
	if(uri->query&&!(os=url_parse_query_array(
		uri->query,0
	)))goto done;
	if(fs_has_flag(flags,FILE_FLAG_NON_BLOCK))
		type|=SOCK_NONBLOCK;
	if(!init_addr(
		protocol,&domain,&proto,
		&type,uri,&addr,&len
	))goto done;
	if((nf->fd=fd=socket(domain,type,proto))<0)goto done;
	if(connect(fd,&addr.addr,len)!=0)goto done;
	for(
		size_t i=0;
		opts[i].type!=OPT_TYPE_NONE;
		i++
	)switch(opts[i].type){
		case OPT_TYPE_BOOLEAN:sockopt_bool(
			fd,os,opts[i].level,
			opts[i].option,opts[i].name
		);break;
		default:break;
	}
	if(os)kvarr_free(os);
	RET(0);
	done:
	e=errno;
	if(os)kvarr_free(os);
	fsdrv_posix_close(drv,nf);
	XRET(e,EIO);
}

static int fsdrv_get_size(const fsdrv*drv,fsh*f,size_t*out){
	errno=0;
	if(!f||!drv||f->driver!=drv||!out)RET(EINVAL);
	if(f->fd<=0)RET(EBADF);
	if(ioctl(f->fd,FIONREAD,out)<0)EXRET(EIO);
	RET(0);
}

static fsdrv fsdrv_socket={
	.magic=FS_DRIVER_MAGIC,
	.features=
		FS_FEATURE_READABLE|
		FS_FEATURE_WRITABLE|
		FS_FEATURE_NON_BLOCK|
		FS_FEATURE_HAVE_SIZE,
	.base=&fsdrv_template,
	.open=fsdrv_socket_open,
	.close=fsdrv_posix_close,
	.read=fsdrv_posix_read,
	.write=fsdrv_posix_write,
	.get_size=fsdrv_get_size,
	.wait=fsdrv_posix_wait,
};

void fsdrv_register_socket(bool deinit){
	fsdrv*drv=NULL;
	if(deinit)return;
	for(size_t i=0;pmaps[i].name;i++){
		if(!(drv=malloc(sizeof(fsdrv))))continue;
		memcpy(drv,&fsdrv_socket,sizeof(fsdrv));
		strncpy(
			drv->protocol,
			pmaps[i].name,
			sizeof(drv->protocol)-1
		);
		if(fsdrv_register(drv)==0)continue;
		free(drv);
	}
}
