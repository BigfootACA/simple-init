/*
 *
 * Copyright 2008, The Android Open Source Project
 *
 */

#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<stddef.h>
#include<string.h>
#include<signal.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/utsname.h>
#include"str.h"
#include"list.h"
#include"confd.h"
#include"logger.h"
#include"keyval.h"
#include"version.h"
#include"defines.h"
#include"pathnames.h"
#include"adbd_internal.h"
#define TAG "adbd"
pthread_mutex_t D_lock=PTHREAD_MUTEX_INITIALIZER;
static struct adb_data*data;
char*adbd_get_shell(){
	return data?data->shell:NULL;
}
void adbd_send_ok(){
	if(data&&data->notifyfd>=0){
		write(data->notifyfd,"OK",2);
		close(data->notifyfd);
		data->notifyfd=-1;
	}
}
apacket*get_apacket(void){
	apacket*p=malloc(sizeof(apacket));
	if(!p){
		telog_error("failed to allocate an apacket");
		exit(-1);
		return NULL;
	}
	memset(p,0,sizeof(apacket)-MAX_PAYLOAD);
	return p;
}
void handle_online(atransport*t){
	tlog_info("status change to online");
	t->online=1;
}
void handle_offline(atransport*t){
	tlog_info("status change to offline");
	t->online=0;
	run_transport_disconnects(t);
}
static void send_ready(unsigned local,unsigned remote,atransport*t){
	apacket*p=get_apacket();
	p->msg.command=A_OKAY;
	p->msg.arg0=local,p->msg.arg1=remote;
	send_packet(p,t);
}
static void send_close(unsigned local,unsigned remote,atransport*t){
	apacket*p=get_apacket();
	p->msg.command=A_CLSE;
	p->msg.arg0=local,p->msg.arg1=remote;
	send_packet(p,t);
}
static size_t fill_connect_data(char*buf,size_t bufsize){
	size_t remaining=bufsize,len;
	len=snprintf(buf,remaining,"%s::",data->banner);
	remaining-=len,buf+=len;
	list*next,*cur;
	if((next=list_first(data->prop)))do{
		cur=next;
		if(!cur->data)continue;
		LIST_DATA_DECLARE(s,cur,keyval*);
		if(!s->key)continue;
		len=snprintf(
			buf,remaining,
			"%s=%s;",
			s->key,s->value
		);
		remaining-=len,buf+=len;
		if(remaining<=0)break;
	}while((next=cur->next));
	return bufsize-remaining+1;
}
static void send_connect(atransport*t){
	apacket*cp=get_apacket();
	cp->msg.command=A_CNXN;
	cp->msg.arg0=A_VERSION;
	cp->msg.arg1=MAX_PAYLOAD;
	cp->msg.data_length=fill_connect_data(
		(char*)cp->data,
		sizeof(cp->data)
	);
	send_packet(cp,t);
}
static void send_auth_request(atransport*t){
	apacket*p;
	int ret;
	if((ret=adb_auth_generate_token(
		t->token,
		sizeof(t->token)
	))!=sizeof(t->token)){
		telog_warn("error generating token");
		return;
	}
	p=get_apacket();
	memcpy(p->data,t->token,ret);
	p->msg.command=A_AUTH;
	p->msg.arg0=ADB_AUTH_TOKEN;
	p->msg.data_length=ret;
	send_packet(p,t);
}
static void send_auth_publickey(atransport*t __attribute__((unused))){
	apacket*p=get_apacket();
	free(p);
}
void adb_auth_verified(atransport*t){
	handle_online(t);
	send_connect(t);
}
static char*connection_state_name(atransport*t){
	if(!t)return "unknown";
	switch(t->connection_state){
		case CS_BOOTLOADER:return "bootloader";
		case CS_DEVICE:return "device";
		case CS_OFFLINE:return "offline";
		default:return "unknown";
	}
}
static void qual_overwrite(char**dst,const char*src){
	if(!dst)return;
	free(*dst);
	*dst=NULL;
	if(!src||!*src)return;
	*dst=strdup(src);
}
void parse_banner(char*banner,atransport*t){
	static const char*prop_seps=";",key_val_sep='=';
	char*cp,*type;
	type=banner;
	if((cp=strchr(type,':'))){
		*cp++=0;
		if((cp=strchr(cp,':'))){
			char*save,*key;
			if((key=strtok_r(cp + 1,prop_seps,&save)))do{
				if((cp=strchr(key,key_val_sep))){
					*cp++='\0';
					if(strcmp(key,"ro.product.name")==0)
						qual_overwrite(&t->product,cp);
					else if(strcmp(key,"ro.product.model")==0)
						qual_overwrite(&t->model,cp);
					else if(strcmp(key,"ro.product.device")==0)
						qual_overwrite(&t->device,cp);
				}
			}while((key=strtok_r(NULL,prop_seps,&save)));
		}
	}
	if(strcmp(type,"bootloader")==0){
		tlog_info("setting connection_state to CS_BOOTLOADER");
		t->connection_state=CS_BOOTLOADER;
		update_transports();
		return;
	}
	if(strcmp(type,"device")==0){
		tlog_info("setting connection_state to CS_DEVICE");
		t->connection_state=CS_DEVICE;
		update_transports();
		return;
	}
	if(strcmp(type,"recovery")==0){
		tlog_info("setting connection_state to CS_RECOVERY");
		t->connection_state=CS_RECOVERY;
		update_transports();
		return;
	}
	if(strcmp(type,"sideload")==0){
		tlog_info("setting connection_state to CS_SIDELOAD");
		t->connection_state=CS_SIDELOAD;
		update_transports();
		return;
	}
	t->connection_state=CS_HOST;
}
void handle_packet(apacket*p,atransport*t){
	asocket*s;
	switch(p->msg.command){
		case A_SYNC:
			if(p->msg.arg0){
				send_packet(p,t);
			}else{
				t->connection_state=CS_OFFLINE;
				handle_offline(t);
				send_packet(p,t);
			}
		return;
		case A_CNXN:
			if(t->connection_state!=CS_OFFLINE){
				t->connection_state=CS_OFFLINE;
				handle_offline(t);
			}
			parse_banner((char*)p->data,t);
			if(!data->auth_enabled){
				handle_online(t);
				send_connect(t);
			}else send_auth_request(t);
		break;
		case A_AUTH:
			if(p->msg.arg0==ADB_AUTH_TOKEN){
				t->key=NULL;
				send_auth_publickey(t);
			}else if(p->msg.arg0==ADB_AUTH_SIGNATURE){
				if(adb_auth_verify(t->token,p->data,p->msg.data_length)){
					adb_auth_verified(t);
					t->failed_auth_attempts=0;
				}else{
					if(t->failed_auth_attempts++>10)usleep(1000000);
					send_auth_request(t);
				}
			}else if(p->msg.arg0==ADB_AUTH_RSAPUBLICKEY)
				adb_auth_confirm_key(p->data,p->msg.data_length,t);
		break;
		case A_OPEN:
			if(!t->online)break;
			char*name=(char*)p->data;
			name[p->msg.data_length>0?p->msg.data_length-1:0]=0;
			if((s=create_local_service_socket(name))==0)send_close(0,p->msg.arg0,t);
			else{
				s->peer=create_remote_socket(p->msg.arg0,t);
				s->peer->peer=s;
				send_ready(s->id,s->peer->id,t);
				s->ready(s);
			}
		break;
		case A_OKAY:
			if(!t->online||!(s=find_local_socket(p->msg.arg1)))break;
			if(s->peer==0){
				s->peer=create_remote_socket(p->msg.arg0,t);
				s->peer->peer=s;
			}
			s->ready(s);
		break;
		case A_CLSE:if(t->online&&(s=find_local_socket(p->msg.arg1)))s->close(s);break;
		case A_WRTE:
			if(!t->online||!(s=find_local_socket(p->msg.arg1)))break;
			unsigned rid=p->msg.arg0;
			p->len=p->msg.data_length;
			if(s->enqueue(s,p)==0)send_ready(s->id,rid,t);
		return;
		default:tlog_warn("handle_packet what is %08x?!",p->msg.command);
	}
	free(p);
}
alistener listener_list={
	.next=&listener_list,
	.prev=&listener_list,
};
static void ss_listener_event_func(
	int _fd,
	unsigned ev,
	void*_l __attribute__((unused))
){
	asocket*s;
	if(!(ev&FDE_READ))return;
	struct sockaddr addr;
	socklen_t alen;
	int fd;
	alen=sizeof(addr);
	if((fd=adb_socket_accept(_fd,&addr,&alen))<0)return;
	int opt=CHUNK_SIZE;
	setsockopt(fd,SOL_SOCKET,SO_RCVBUF,&opt,sizeof(opt));
	if((s=create_local_socket(fd))){
		connect_to_smartsocket(s);
		return;
	}
	close(fd);
}
static void listener_event_func(int _fd,unsigned ev,void*_l){
	alistener*l=_l;
	asocket*s;
	if(!(ev&FDE_READ))return;
	struct sockaddr addr;
	socklen_t alen;
	int fd;
	alen=sizeof(addr);
	if((fd=adb_socket_accept(_fd,&addr,&alen))<0)return;
	if((s=create_local_socket(fd))){
		s->transport=l->transport;
		connect_to_remote(s,l->connect_to);
		return;
	}
	close(fd);
}
static void free_listener(alistener*l){
	if(l->next){
		l->next->prev=l->prev;
		l->prev->next=l->next;
		l->next=l->prev=l;
	}
	fdevent_remove(&l->fde);
	if(l->local_name)free((char*)l->local_name);
	if(l->connect_to)free((char*)l->connect_to);
	if(l->transport)remove_transport_disconnect(
		l->transport,
		&l->disconnect
	);
	free(l);
}
static void listener_disconnect(
	void*_l,
	atransport*t __attribute__((unused))
){
	alistener*l=_l;
	free_listener(l);
}
int local_name_to_fd(const char*name){
	int port;
	if(!strncmp("tcp:",name,4)){
		port=parse_int((char*)name+4,0);
		if(port<0)port=0;
		return socket_loopback_server(port,SOCK_STREAM);
	}
	if(strncmp(name,"local:",6)==0)return socket_local_server(
		name+6,
		ANDROID_SOCKET_NAMESPACE_ABSTRACT,
		SOCK_STREAM
	);
	else if(strncmp(name,"localabstract:",14)==0)return socket_local_server(
		name+14,
		ANDROID_SOCKET_NAMESPACE_ABSTRACT,
		SOCK_STREAM
	);
	else if(strncmp(name,"localfilesystem:",16)==0)return socket_local_server(
		name+16,
		ANDROID_SOCKET_NAMESPACE_FILESYSTEM,
		SOCK_STREAM
	);
	return trlog_warn(-1,"unknown local portname '%s'",name);
}
static int format_listener(alistener*l,char*buffer,size_t buffer_len){
	int local_len=strlen(l->local_name);
	int connect_len=strlen(l->connect_to);
	int serial_len=strlen(l->transport->serial);
	if(buffer)snprintf(
		buffer,
		buffer_len,
		"%s %s %s\n",
		l->transport->serial,
		l->local_name,
		l->connect_to
	);
	return local_len+connect_len+serial_len+3;
}
static int format_listeners(char*buf,size_t buflen){
	alistener*l;
	int result=0;
	for(l=listener_list.next;l!=&listener_list;l=l->next){
		if(l->connect_to[0]=='*')continue;
		int len=format_listener(l,buf,buflen);
		result+=len;
		if(buf){
			buf+=len,buflen-=len;
			if(buflen<=0)break;
		}
	}
	return result;
}
static int remove_listener(
	const char*local_name,
	atransport*t __attribute__((unused))
){
	alistener*l;
	for(l=listener_list.next;l!=&listener_list;l=l->next)
		if(!strcmp(local_name,l->local_name)){
			listener_disconnect(l,l->transport);
			return 0;
		}
	return -1;
}
static void remove_all_listeners(void){
	alistener*l,*l_next;
	for(l=listener_list.next;l!=&listener_list;l=l_next){
		l_next=l->next;
		if(l->connect_to[0]=='*')continue;
		listener_disconnect(l,l->transport);
	}
}
typedef enum {
	INSTALL_STATUS_OK=0,
	INSTALL_STATUS_INTERNAL_ERROR=-1,
	INSTALL_STATUS_CANNOT_BIND=-2,
	INSTALL_STATUS_CANNOT_REBIND=-3,
}install_status_t;
static install_status_t install_listener(
	const char*local_name,
	const char*connect_to,
	atransport*transport,
	int no_rebind
){
	alistener*l;
	for(l=listener_list.next;l!=&listener_list; l=l->next){
		if(strcmp(local_name,l->local_name)==0){
			char*cto;
			if(l->connect_to[0]=='*')return INSTALL_STATUS_INTERNAL_ERROR;
			if(no_rebind)return INSTALL_STATUS_CANNOT_REBIND;
			cto=strdup(connect_to);
			if(cto==0)return INSTALL_STATUS_INTERNAL_ERROR;
			free((void*)l->connect_to);
			l->connect_to=cto;
			if(l->transport!=transport){
				remove_transport_disconnect(
					l->transport,
					&l->disconnect
				);
				l->transport=transport;
				add_transport_disconnect(
					l->transport,
					&l->disconnect
				);
			}
			return INSTALL_STATUS_OK;
		}
	}
	if((l=calloc(1,sizeof(alistener)))==0)goto nomem;
	if((l->local_name=strdup(local_name))==0)goto nomem;
	if((l->connect_to=strdup(connect_to))==0)goto nomem;
	if((l->fd=local_name_to_fd(local_name))<0){
		free((void*)l->local_name);
		free((void*)l->connect_to);
		free(l);
		return trlog_warn(-1,"cannot bind '%s'",local_name);
	}
	fcntl(l->fd,F_SETFD,FD_CLOEXEC);
	fdevent_install(
		&l->fde,l->fd,
		strcmp(l->connect_to,"*smartsocket*")==0?
			ss_listener_event_func:
			listener_event_func,
	l);
	fdevent_set(&l->fde,FDE_READ);
	l->next=&listener_list;
	l->prev=listener_list.prev;
	l->next->prev=l;
	l->prev->next=l;
	l->transport=transport;
	if(transport){
		l->disconnect.opaque=l;
		l->disconnect.func=listener_disconnect;
		add_transport_disconnect(transport,&l->disconnect);
	}
	return INSTALL_STATUS_OK;
nomem:
	if(l){
		if(l->local_name)free(l->local_name);
		if(l->connect_to)free(l->connect_to);
		free(l);
	}
	telog_error("cannot allocate listener");
	exit(-1);
	return INSTALL_STATUS_INTERNAL_ERROR;
}
void build_local_name(char*target_str,size_t target_size,int server_port){
	snprintf(target_str,target_size,"tcp:%d",server_port);
}
int init_adb_data(struct adb_data*d){
	char*shell=getenv("SHELL");
	struct utsname u;
	uname(&u);
	memset(d,0,sizeof(struct adb_data));
	if(shell)strcpy(d->shell,shell);
	strcpy(d->ffs,_PATH_DEV"/usb-ffs/adb");
	strcpy(d->banner,"device");
	d->prop=kvlst_set(d->prop,"ro.product.device",NAME);
	d->prop=kvlst_set(d->prop,"ro.product.name",u.sysname);
	d->prop=kvlst_set(d->prop,"ro.product.model",u.version);
	d->local_port=5038;
	d->notifyfd=-1;
	d->port=5555;
	return 0;
}
int free_adb_data(struct adb_data*d){
	if(!d)ERET(EINVAL);
	kvlst_free(d->prop);
	if(d->notifyfd>=0)close(d->notifyfd);
	free(d);
	return 0;
}
static void adbd_exit(){
	tlog_debug("adbd exiting");
	confd_delete("runtime.pid.adbd");
}
static void adbd_signal(int s __attribute__((unused))){
	exit(0);
}
int adbd_init(struct adb_data*d){
	if(!d)ERET(EINVAL);
	data=d;
	umask(000);
	atexit(adbd_exit);
	signal(SIGINT,adbd_signal);
	signal(SIGTERM,adbd_signal);
	signal(SIGQUIT,adbd_signal);
	signal(SIGPIPE,adbd_signal);
	init_transport_registration();
	if(d->auth_enabled)adb_auth_init();
	char local_name[30];
	build_local_name(local_name,sizeof(local_name),data->port);
	if(install_listener(local_name,"*smartsocket*",NULL,0))exit(1);
	switch(data->proto){
		case PROTO_TCP:
			tlog_info("using Local Port");
			local_init(data->local_port);
		break;
		case PROTO_USB:
			if(access(data->ffs,F_OK)!=0)return terlog_error(
				errno,
				"access adb ffs %s",
				data->ffs
			);
			tlog_info("using USB");
			usb_init(data->ffs);
		break;
		default:return trlog_error(-1,"unknown adb protocol");
	}
	confd_set_integer("runtime.pid.adbd",getpid());
	fdevent_loop();
	return 0;
}
int handle_host_request(
	char*service,
	transport_type ttype,
	char*serial,
	int reply_fd,
	asocket*s __attribute__((unused))
){
	atransport*transport=NULL;
	char buf[4096];
	if(strcmp(service,"kill")==0){
		tlog_notice("adb server killed by remote request");
		fflush(stdout);
		adb_write(reply_fd,"OKAY",4);
		exit(0);
	}
	if(!strcmp(service,"list-forward")){
		char header[9];
		int buffer_size=format_listeners(NULL,0);
		char*buffer=malloc(buffer_size+1);
		(void)format_listeners(buffer,buffer_size+1);
		snprintf(header,sizeof(header),"OKAY%04x",buffer_size);
		writex(reply_fd,header,8);
		writex(reply_fd,buffer,buffer_size);
		free(buffer);
		return 0;
	}
	if(!strcmp(service,"killforward-all")){
		remove_all_listeners();
		adb_write(reply_fd,"OKAYOKAY",8);
		return 0;
	}
	if(strncmp(service,"forward:",8)==0||strncmp(service,"killforward:",12)==0){
		char*local,*remote,*err;
		int r;
		atransport*tt;
		int cf=strncmp(service,"kill",4);
		int no_rebind=0;
		local=strchr(service,':')+1;
		if(cf&&strncmp(local,"norebind:",9)==0){
			no_rebind=1;
			local=strchr(local,':')+ 1;
		}
		remote=strchr(local,';');
		if(cf){
			if(remote==0){
				sendfailmsg(reply_fd,"malformed forward spec");
				return 0;
			}
			*remote++=0;
			if((local[0]==0)||(remote[0]==0)||(remote[0]=='*')){
				sendfailmsg(reply_fd,"malformed forward spec");
				return 0;
			}
		}else if(local[0]==0){
			sendfailmsg(reply_fd,"malformed forward spec");
			return 0;
		}
		if(!(tt=acquire_one_transport(CS_ANY,ttype,serial,&err))){
			sendfailmsg(reply_fd,err);
			return 0;
		}
		if((r=cf?
			install_listener(local,remote,tt,no_rebind):
			remove_listener(local,tt)
		)==0){
			writex(reply_fd,"OKAYOKAY",8);
			return 0;
		}
		if(cf)switch(r){
			case INSTALL_STATUS_CANNOT_BIND:
				sendfailmsg(reply_fd,"cannot bind to socket");
			break;
			case INSTALL_STATUS_CANNOT_REBIND:
				sendfailmsg(reply_fd,"cannot rebind existing socket");
			break;
			default:sendfailmsg(reply_fd,"internal error");
		}else sendfailmsg(reply_fd,"cannot remove listener");
		return 0;
	}
	if(strncmp(service,"get-state",strlen("get-state"))==0){
		transport=acquire_one_transport(CS_ANY,ttype,serial,NULL);
		char*state=connection_state_name(transport);
		snprintf(
			buf,sizeof(buf),
			"OKAY%04x%s",
			(unsigned)strlen(state),
			state
		);
		writex(reply_fd,buf,strlen(buf));
		return 0;
	}
	return -1;
}
