/*
 *
 * Copyright 2008, The Android Open Source Project
 *
 */

#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include"logger.h"
#include"adbd_internal.h"
#define TAG "adbd"
static void transport_unref(atransport*t);
static atransport transport_list={.next=&transport_list,.prev=&transport_list,};
pthread_mutex_t transport_lock=PTHREAD_MUTEX_INITIALIZER;
void kick_transport(atransport*t){
	if(t&&!t->kicked){
		int kicked;
		pthread_mutex_lock(&transport_lock);
		kicked=t->kicked;
		if(!kicked)t->kicked=1;
		pthread_mutex_unlock(&transport_lock);
		if(!kicked)t->kick(t);
	}
}
void run_transport_disconnects(atransport*t){
	adisconnect*dis=t->disconnects.next;
	while(dis!=&t->disconnects){
		adisconnect*next=dis->next;
		dis->func(dis->opaque,t);
		dis=next;
	}
}
static int read_packet(
	int fd,
	const char*name __attribute__((unused)),
	apacket**ppacket
){
	char*p=(char*)ppacket;
	int r,len=sizeof(*ppacket);
	errno=0;
	while(len>0){
		if((r=adb_read(fd,p,len))>0)len-=r,p+=r;
		else{
			if((r<0)&&(errno==EINTR))continue;
			return -1;
		}
	}
	return 0;
}
static int write_packet(
	int fd,
	const char*name __attribute__((unused)),
	apacket**ppacket
){
	char*p=(char*)ppacket;
	int r,len=sizeof(ppacket);
	errno=0;
	while(len>0){
		if((r=adb_write(fd,p,len))>0)
			len-=r,p+=r;
		else{
			if((r<0)&&(errno==EINTR))continue;
			return -1;
		}
	}
	return 0;
}
static void transport_socket_events(int fd,unsigned events,void*_t){
	atransport*t=_t;
	if(events&FDE_READ){
		apacket*p=0;
		if(!read_packet(fd,t->serial,&p))handle_packet(p,(atransport*)_t);
	}
}
void send_packet(apacket*p,atransport*t){
	if(!t||!p)return;
	unsigned char*x;
	unsigned sum,count;
	p->msg.magic=p->msg.command^0xffffffff;
	count=p->msg.data_length;
	x=(unsigned char*)p->data;
	sum=0;
	while(count-->0)sum+=*x++;
	p->msg.data_check=sum;
	if(write_packet(t->transport_socket,t->serial,&p)){
		telog_error("cannot enqueue packet on transport socket");
		exit(-1);
	}
}
static void*output_thread(void*_t){
	atransport*t=_t;
	apacket*p;
	p=get_apacket();
	p->msg.command=A_SYNC;
	p->msg.arg0=1;
	p->msg.arg1=++(t->sync_token);
	p->msg.magic=A_SYNC ^ 0xffffffff;
	if(write_packet(t->fd,t->serial,&p)){
		free(p);
		goto oops;
	}
	for(;;){
		if(t->read_from_remote((p=get_apacket()),t)==0){
			if(write_packet(t->fd,t->serial,&p)){
				free(p);
				goto oops;
			}
		}else{
			free(p);
			break;
		}
	}
	p=get_apacket();
	p->msg.command=A_SYNC;
	p->msg.arg0=0;
	p->msg.arg1=0;
	p->msg.magic=A_SYNC ^ 0xffffffff;
	if(write_packet(t->fd,t->serial,&p))free(p);
oops:
	kick_transport(t);
	transport_unref(t);
	return 0;
}
static void*input_thread(void*_t){
	atransport*t=_t;
	apacket*p;
	int active=0;
	for(;;){
		if(read_packet(t->fd,t->serial,&p))break;
		if(p->msg.command==A_SYNC){
			if(p->msg.arg0==0){
				free(p);
				break;
			}else if(p->msg.arg1==t->sync_token)active=1;
		}else if(active)t->write_to_remote(p,t);
		free(p);
	}
	close_all_sockets(t);
	kick_transport(t);
	transport_unref(t);
	return 0;
}
static int transport_registration_send=-1,transport_registration_recv=-1;
static fdevent transport_registration_fde;
void  update_transports(void){}
typedef struct tmsg tmsg;
struct tmsg{atransport*transport;int action;};
static int transport_read_action(int fd,struct tmsg*m){
	char*p=(char*)m;
	int len=sizeof(*m),r;
	while(len>0)if((r=adb_read(fd,p,len))>0){
		len-=r;
		p+=r;
	}else{
		if((r<0)&&(errno==EINTR))continue;
		return -1;
	}
	return 0;
}
static int transport_write_action(int fd,struct tmsg*m){
	char*p=(char*)m;
	int len=sizeof(*m),r;
	while(len>0)if((r=adb_write(fd,p,len))>0){
		len-=r;
		p+=r;
	}else{
		if((r<0)&&(errno==EINTR))continue;
		return -1;
	}
	return 0;
}
static void transport_registration_func(int _fd,unsigned ev,void*data){
	(void)data;
	tmsg m;
	pthread_t output_thread_ptr,input_thread_ptr;
	int s[2];
	atransport*t;
	if(!(ev&FDE_READ))return;
	if(transport_read_action(_fd,&m)){
		telog_error("cannot read transport registration socket");
		exit(-1);
	}
	t=m.transport;
	if(m.action==0){
		fdevent_remove(&(t->transport_fde));
		close(t->fd);
		pthread_mutex_lock(&transport_lock);
		t->next->prev=t->prev;
		t->prev->next=t->next;
		pthread_mutex_unlock(&transport_lock);
		run_transport_disconnects(t);
		if(t->product)free(t->product);
		if(t->serial)free(t->serial);
		if(t->model)free(t->model);
		if(t->device)free(t->device);
		if(t->devpath)free(t->devpath);
		memset(t,0xee,sizeof(atransport));
		free(t);
		update_transports();
		return;
	}
	if(t->connection_state!=CS_NOPERM){
		t->ref_count=2;
		if(adb_socketpair(s)){
			telog_error("cannot open transport socketpair");
			exit(-1);
		}
		t->transport_socket=s[0];
		t->fd=s[1];
		fdevent_install(&(t->transport_fde),t->transport_socket,transport_socket_events,t);
		fdevent_set(&(t->transport_fde),FDE_READ);
		if(adb_thread_create(&input_thread_ptr,input_thread,t)){
			telog_error("cannot create input thread");
			exit(-1);
		}
		if(adb_thread_create(&output_thread_ptr,output_thread,t)){
			telog_error("cannot create output thread");
			exit(-1);
		}
	}
	pthread_mutex_lock(&transport_lock);
	t->next=&transport_list;
	t->prev=transport_list.prev;
	t->next->prev=t;
	t->prev->next=t;
	pthread_mutex_unlock(&transport_lock);
	t->disconnects.next=t->disconnects.prev=&t->disconnects;
	update_transports();
}
void init_transport_registration(void){
	int s[2];
	if(adb_socketpair(s)!=0){
		telog_error("cannot open transport registration socketpair");
		exit(-1);
	}
	transport_registration_send=s[0];
	transport_registration_recv=s[1];
	fdevent_install(&transport_registration_fde,transport_registration_recv,transport_registration_func,0);
	fdevent_set(&transport_registration_fde,FDE_READ);
}
static void register_transport(atransport*transport){
	tmsg m;
	m.transport=transport;
	m.action=1;
	if(transport_write_action(transport_registration_send,&m)!=0){
		telog_error("cannot write transport registration socket");
		exit(-1);
	}
}
static void remove_transport(atransport*transport){
	tmsg m;
	m.transport=transport;
	m.action=0;
	if(transport_write_action(transport_registration_send,&m)!=0){
		telog_error("cannot write transport registration socket");
		exit(-1);
	}
}
static void transport_unref_locked(atransport*t){
	t->ref_count--;
	if(t->ref_count==0){
		if(!t->kicked){
			t->kicked=1;
			t->kick(t);
		}
		t->close(t);
		remove_transport(t);
	}
}
static void transport_unref(atransport*t){
	if(!t)return;
	pthread_mutex_lock(&transport_lock);
	transport_unref_locked(t);
	pthread_mutex_unlock(&transport_lock);
}
void add_transport_disconnect(atransport*t,adisconnect*dis){
	pthread_mutex_lock(&transport_lock);
	dis->next=&t->disconnects;
	dis->prev=dis->next->prev;
	dis->prev->next=dis;
	dis->next->prev=dis;
	pthread_mutex_unlock(&transport_lock);
}
void remove_transport_disconnect(atransport*t,adisconnect*dis){
	(void)t;
	dis->prev->next=dis->next;
	dis->next->prev=dis->prev;
	dis->next=dis->prev=dis;
}
static int qual_char_is_invalid(char ch){
	if('A'<=ch&&ch<='Z')return 0;
	if('a'<=ch&&ch<='z')return 0;
	if('0'<=ch&&ch<='9')return 0;
	return 1;
}
static int qual_match(const char*to_test,const char*prefix,const char*qual,int sanitize_qual){
	if(!to_test||!*to_test)return!qual||!*qual;
	if(!qual)return 0;
	if(prefix)while(*prefix)if(*prefix++!=*to_test++)return 0;
	while(*qual){
		char ch=*qual++;
		if(sanitize_qual&&qual_char_is_invalid(ch))ch='_';
		if(ch!=*to_test++)return 0;
	}
	return !*to_test;
}
atransport*acquire_one_transport(int state,transport_type ttype,const char*serial,char**error_out){
	atransport*t,*result=NULL;
	int ambiguous=0;
retry:
	if(error_out)*error_out="device not found";
	pthread_mutex_lock(&transport_lock);
	for(t=transport_list.next;t!=&transport_list;t=t->next){
		if(t->connection_state==CS_NOPERM){
			if(error_out)*error_out="insufficient permissions for device";
			continue;
		}
		if(serial){
			if((
				t->serial&&
				!strcmp(serial,t->serial))||
				(t->devpath&&!strcmp(serial,t->devpath))||
				qual_match(serial,"product:",t->product,0)||
				qual_match(serial,"model:",t->model,1)||
				qual_match(serial,"device:",t->device,0)
			){
				if(result){
					if(error_out)*error_out="more than one device";
					ambiguous=1;
					result=NULL;
					break;
				}
				result=t;
			}
		}else if(ttype==kTransportUsb&&t->type==kTransportUsb){
			if(result){
				if(error_out)*error_out="more than one device";
				ambiguous=1;
				result=NULL;
				break;
			}
			result=t;
		}else if(ttype==kTransportLocal&&t->type==kTransportLocal){
			if(result){
				if(error_out)*error_out="more than one emulator";
				ambiguous=1;
				result=NULL;
				break;
			}
			result=t;
		}else if(ttype==kTransportAny){
			if(result){
				if(error_out)*error_out="more than one device and emulator";
				ambiguous=1;
				result=NULL;
				break;
			}
			result=t;
		}
	}
	pthread_mutex_unlock(&transport_lock);
	if(result){
		if(result->connection_state==CS_OFFLINE){
			if(error_out)*error_out="device offline";
			result=NULL;
		}
		if(result&&state!=CS_ANY&&result->connection_state!=state){
			if(error_out)*error_out="invalid device state";
			result=NULL;
		}
	}
	if(result){
		if(error_out)*error_out=NULL;
	}else if(state!=CS_ANY&&(serial||!ambiguous)){
		usleep(1000000);
		goto retry;
	}
	return result;
}
void register_socket_transport(int s,const char*serial,int port,int local){
	atransport*t=calloc(1,sizeof(atransport));
	char buff[32];
	if(!serial){
		snprintf(buff,sizeof buff,"T-%p",t);
		serial=buff;
	}
	if(init_socket_transport(t,s,port,local)<0){
		close(s);
		free(t);
		return;
	}
	if(serial)t->serial=strdup(serial);
	register_transport(t);
}
void register_usb_transport(usb_handle*usb,const char*serial,const char*devpath,unsigned writeable){
	atransport*t=calloc(1,sizeof(atransport));
	init_usb_transport(t,usb,(writeable?CS_OFFLINE:CS_NOPERM));
	if(serial)t->serial=strdup(serial);
	if(devpath)t->devpath=strdup(devpath);
	register_transport(t);
}
void unregister_usb_transport(usb_handle*usb){
	atransport*t;
	pthread_mutex_lock(&transport_lock);
	for(t=transport_list.next;t!=&transport_list;t=t->next)if(t->usb==usb&&t->connection_state==CS_NOPERM){
		t->next->prev=t->prev;
		t->prev->next=t->next;
		break;
	}
	pthread_mutex_unlock(&transport_lock);
}
int readx(int fd,void*ptr,size_t len){
	char*p=ptr;
	int r;
	while(len>0)if((r=adb_read(fd,p,len))>0){
		len-=r,p+=r;
	}else{
		if(r<0&&errno==EINTR)continue;
		return -1;
	}
	return 0;
}
int writex(int fd,const void*ptr,size_t len){
	char*p=(char*)ptr;
	int r;
	while(len>0)if((r=adb_write(fd,p,len))>0){
		len-=r,p+=r;
	}else{
		if(r<0&&errno==EINTR)continue;
		return -1;
	}
	return 0;
}
int check_header(apacket*p){
	if(p->msg.magic!=(p->msg.command^0xffffffff))return -1;
	if(p->msg.data_length>MAX_PAYLOAD)return -1;
	return 0;
}
int check_data(apacket*p){
	unsigned count,sum;
	unsigned char*x;
	count=p->msg.data_length;
	x=p->data;
	sum=0;
	while(count-->0)sum +=*x++;
	return(sum !=p->msg.data_check)?-1:0;
}
static int local_remote_read(apacket*p,atransport*t){
	if(readx(t->sfd,&p->msg,sizeof(amessage)))return -1;
	if(check_header(p))return -1;
	if(readx(t->sfd,p->data,p->msg.data_length))return -1;
	if(check_data(p))return -1;
	return 0;
}
static int local_remote_write(apacket *p,atransport *t){
	int length=p->msg.data_length;
	if(writex(t->sfd,&p->msg,sizeof(amessage)+length))return -1;
	return 0;
}
int local_connect(int port){return local_connect_arbitrary_ports(port-1,port);}
int local_connect_arbitrary_ports(int console_port,int adb_port){
	char buf[64];
	int fd=-1;
	if(fd<0)fd=socket_loopback_client(adb_port,SOCK_STREAM);
	if(fd>=0){
		fcntl(fd,F_SETFD,FD_CLOEXEC);
		disable_tcp_nagle(fd);
		snprintf(buf,sizeof buf,"%s%d",LOCAL_CLIENT_PREFIX,console_port);
		register_socket_transport(fd,buf,adb_port,1);
		return 0;
	}
	return -1;
}
static _Noreturn void*server_socket_thread(void*arg){
	int serverfd,fd;
	struct sockaddr addr;
	socklen_t alen;
	int port=*(int*)arg;
	serverfd=-1;
	for(;;){
		if(serverfd==-1){
			if((serverfd=socket_inaddr_any_server(port,SOCK_STREAM))<0){
				usleep(1000000);
				continue;
			}
			fcntl(serverfd,F_SETFD,FD_CLOEXEC);
			adbd_send_ok();
		}
		alen=sizeof(addr);
		if((fd=adb_socket_accept(serverfd,&addr,&alen))>=0){
			fcntl(fd,F_SETFD,FD_CLOEXEC);
			disable_tcp_nagle(fd);
			register_socket_transport(fd,"host",port,1);
		}
	}
}
void local_init(int port){
	pthread_t thr;
	if(adb_thread_create(&thr,server_socket_thread,&port)!=0){
		telog_error("cannot create local socket server thread");
		exit(-1);
	}
}
static void local_remote_kick(atransport*t){
	int fd=t->sfd;
	t->sfd=-1;
	shutdown(fd,SHUT_RDWR);
	close(fd);
}
static void local_remote_close(atransport*t){close(t->fd);}
int init_socket_transport(
	atransport*t,
	int s,
	int adb_port __attribute__((unused)),
	int local __attribute__((unused))
){
	int fail=0;
	t->kick=local_remote_kick;
	t->close=local_remote_close;
	t->read_from_remote=local_remote_read;
	t->write_to_remote=local_remote_write;
	t->sfd=s;
	t->sync_token=1;
	t->connection_state=CS_OFFLINE;
	t->type=kTransportLocal;
	t->adb_port=0;
	return fail;
}
static int usb_remote_read(apacket*p,atransport*t){
	if(usb_read(t->usb,&p->msg,sizeof(amessage)))return -1;
	if(check_header(p))return -1;
	if(p->msg.data_length&&usb_read(t->usb,p->data,p->msg.data_length))return -1;
	if(check_data(p))return -1;
	return 0;
}
static int usb_remote_write(apacket*p,atransport*t){
	unsigned size=p->msg.data_length;
	if(usb_write(t->usb,&p->msg,sizeof(amessage)))return -1;
	if(p->msg.data_length==0)return 0;
	if(usb_write(t->usb,&p->data,size))return -1;
	return 0;
}
static void usb_remote_close(atransport*t){usb_close(t->usb);t->usb=0;}
static void usb_remote_kick(atransport*t){usb_kick(t->usb);}
void init_usb_transport(atransport*t,usb_handle*h,int state){
	t->close=usb_remote_close;
	t->kick=usb_remote_kick;
	t->read_from_remote=usb_remote_read;
	t->write_to_remote=usb_remote_write;
	t->sync_token=1;
	t->connection_state=state;
	t->type=kTransportUsb;
	t->usb=h;
}
