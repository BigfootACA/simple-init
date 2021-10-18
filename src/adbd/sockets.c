/*
 *
 * Copyright 2008, The Android Open Source Project
 *
 */

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<ctype.h>
#include"logger.h"
#include"adbd_internal.h"
#define TAG "adbd"
pthread_mutex_t socket_list_lock=PTHREAD_MUTEX_INITIALIZER;
static void local_socket_close_locked(asocket*s);
int sendfailmsg(int fd,const char*reason){
	char buf[9];
	int len;
	len=strlen(reason);
	if(len>0xffff)len=0xffff;
	snprintf(buf,sizeof buf,"FAIL%04x",len);
	if(writex(fd,buf,8))return -1;
	return writex(fd,reason,len);
}
static unsigned local_socket_next_id=1;
static asocket local_socket_list={.next=&local_socket_list,.prev=&local_socket_list,};
static asocket local_socket_closing_list={.next=&local_socket_closing_list,.prev=&local_socket_closing_list,};
asocket*find_local_socket(unsigned id){
	asocket*s;
	asocket*result=NULL;
	pthread_mutex_lock(&socket_list_lock);
	for(s=local_socket_list.next;s!=&local_socket_list;s=s->next)if(s->id==id){result=s;break;}
	pthread_mutex_unlock(&socket_list_lock);
	return result;
}
static void insert_local_socket(asocket*s,asocket*list){
	s->next=list;
	s->prev=s->next->prev;
	s->prev->next=s;
	s->next->prev=s;
}
void install_local_socket(asocket*s){
	pthread_mutex_lock(&socket_list_lock);
	s->id=local_socket_next_id++;
	insert_local_socket(s,&local_socket_list);
	pthread_mutex_unlock(&socket_list_lock);
}
void remove_socket(asocket*s){
	if(!s->prev||!s->next)return;
	s->prev->next=s->next;
	s->next->prev=s->prev;
	s->next=0;
	s->prev=0;
	s->id=0;
}
void close_all_sockets(atransport*t){
	asocket*s;
	pthread_mutex_lock(&socket_list_lock);
restart:
	for(s=local_socket_list.next;s!=&local_socket_list;s=s->next)if(s->transport==t||(s->peer&&s->peer->transport==t)){
		local_socket_close_locked(s);
		goto restart;
	}
	pthread_mutex_unlock(&socket_list_lock);
}
static int local_socket_enqueue(asocket*s,apacket*p){
	p->ptr=p->data;
	if(s->pkt_first)goto enqueue;
	while(p->len>0){
		int r=adb_write(s->fd,p->ptr,p->len);
		if(r>0){
			p->len-=r;
			p->ptr+=r;
			continue;
		}
		if((r==0)||(errno!=EAGAIN)){s->close(s);return 1;}else break;
	}
	if(p->len==0){
		free(p);
		return 0;
	}
enqueue:
	p->next=0;
	if(s->pkt_first)s->pkt_last->next=p;
	else s->pkt_first=p;
	s->pkt_last=p;
	fdevent_add(&s->fde,FDE_WRITE);
	return 1;
}
static void local_socket_ready(asocket*s){fdevent_add(&s->fde,FDE_READ);}
static void local_socket_close(asocket*s){
	pthread_mutex_lock(&socket_list_lock);
	local_socket_close_locked(s);
	pthread_mutex_unlock(&socket_list_lock);
}
static void local_socket_destroy(asocket*s){
	apacket*p,*n;
	int exit_on_close=s->exit_on_close;
	fdevent_remove(&s->fde);
	for(p=s->pkt_first;p;p=n){
		n=p->next;
		free(p);
	}
	remove_socket(s);
	free(s);
	if(exit_on_close)exit(1);
}
static void local_socket_close_locked(asocket*s){
	if(s->peer){
		s->peer->peer=0;
		if(s->peer->close==local_socket_close)local_socket_close_locked(s->peer);
		else s->peer->close(s->peer);
		s->peer=0;
	}
	if(s->closing||s->pkt_first==NULL){
		local_socket_destroy(s);
		return;
	}
	s->closing=1;
	fdevent_del(&s->fde,FDE_READ);
	remove_socket(s);
	insert_local_socket(s,&local_socket_closing_list);
}
static void local_socket_event_func(int fd,unsigned ev,void*_s){
	asocket*s=_s;
	if(ev&FDE_WRITE){
		apacket*p;
		while((p=s->pkt_first)!=0){
			while(p->len>0){
				int r=adb_write(fd,p->ptr,p->len);
				if(r>0){
					p->ptr +=r;
					p->len -=r;
					continue;
				}else if(r<0){
					if(errno==EAGAIN)return;
					if(errno==EINTR)continue;
				}
				s->close(s);
				return;
			}
			if(p->len==0){
				s->pkt_first=p->next;
				if(s->pkt_first==0)s->pkt_last=0;
				free(p);
			}
		}
		if(s->closing){s->close(s);return;}
		fdevent_del(&s->fde,FDE_WRITE);
		s->peer->ready(s->peer);
	}
	if(ev&FDE_READ){
		apacket*p=get_apacket();
		unsigned char*x=p->data;
		size_t avail=MAX_PAYLOAD;
		int r,is_eof=0;
		while(avail>0){
			r=adb_read(fd,x,avail);
			if(r>0){
				avail-=r,x+=r;
				continue;
			}else if(r<0){
				if(errno==EAGAIN)break;
				if(errno==EINTR)continue;
			}
			is_eof=1;
			break;
		}
		if((avail==MAX_PAYLOAD)||(s->peer==0))free(p);
		else{
			p->len=MAX_PAYLOAD-avail;
			r=s->peer->enqueue(s->peer,p);
			if(r<0)return;
			else if(r>0)fdevent_del(&s->fde,FDE_READ);
		}
		if((s->fde.force_eof&&!r)||is_eof)s->close(s);
	}
	if(ev&FDE_ERROR)return;
}
asocket*create_local_socket(int fd){
	asocket*s=calloc(1,sizeof(asocket));
	if(!s){
		telog_error("cannot allocate socket");
		exit(-1);
	}
	s->fd=fd;
	s->enqueue=local_socket_enqueue;
	s->ready=local_socket_ready;
	s->close=local_socket_close;
	install_local_socket(s);
	fdevent_install(&s->fde,fd,local_socket_event_func,s);
	return s;
}
asocket*create_local_service_socket(const char*name){
	asocket*s;
	int fd;
	fd=service_to_fd(name);
	if(fd<0)return 0;
	s=create_local_socket(fd);
	if((!strncmp(name,"root:",5)&&getuid()!=0)||!strncmp(name,"usb:",4)||!strncmp(name,"tcpip:",6))s->exit_on_close=1;
	return s;
}
typedef struct aremotesocket{asocket socket;adisconnect disconnect;}aremotesocket;
static int remote_socket_enqueue(asocket*s,apacket*p){
	p->msg.command=A_WRTE;
	p->msg.arg0=s->peer->id;
	p->msg.arg1=s->id;
	p->msg.data_length=p->len;
	send_packet(p,s->transport);
	return 1;
}
static void remote_socket_ready(asocket*s){
	apacket*p=get_apacket();
	p->msg.command=A_OKAY;
	p->msg.arg0=s->peer->id;
	p->msg.arg1=s->id;
	send_packet(p,s->transport);
}
static void remote_socket_close(asocket*s){
	apacket*p=get_apacket();
	p->msg.command=A_CLSE;
	if(s->peer){p->msg.arg0=s->peer->id;s->peer->peer=0;s->peer->close(s->peer);}
	p->msg.arg1=s->id;
	send_packet(p,s->transport);
	remove_transport_disconnect(s->transport,&((aremotesocket*)s)->disconnect );
	free(s);
}
static void remote_socket_disconnect(void*_s,atransport*t __attribute__((unused))){
	asocket*s=_s,*peer=s->peer;
	if(peer){
		peer->peer=NULL;
		peer->close(peer);
	}
	remove_transport_disconnect(s->transport,&((aremotesocket*)s)->disconnect);
	free(s);
}
asocket*create_remote_socket(unsigned id,atransport*t){
	asocket*s=calloc(1,sizeof(aremotesocket));
	adisconnect*dis=&((aremotesocket*)s)->disconnect;
	if(!s){
		telog_error("cannot allocate socket");
		exit(-1);
	}
	s->id=id;
	s->enqueue=remote_socket_enqueue;
	s->ready=remote_socket_ready;
	s->close=remote_socket_close;
	s->transport=t;
	dis->func=remote_socket_disconnect;
	dis->opaque=s;
	add_transport_disconnect(t,dis );
	return s;
}
void connect_to_remote(asocket*s,const char*destination){
	apacket*p=get_apacket();
	int len=strlen(destination)+ 1;
	if(len>(MAX_PAYLOAD-1)){
		tlog_error("destination oversized");
		exit(-1);
	}
	p->msg.command=A_OPEN;
	p->msg.arg0=s->id;
	p->msg.data_length=len;
	strcpy((char*)p->data,destination);
	send_packet(p,s->transport);
}
static void local_socket_ready_notify(asocket*s){
	s->ready=local_socket_ready;
	s->close=local_socket_close;
	adb_write(s->fd,"OKAY",4);
	s->ready(s);
}
static void local_socket_close_notify(asocket*s){
	s->ready=local_socket_ready;
	s->close=local_socket_close;
	sendfailmsg(s->fd,"closed");
	s->close(s);
}
unsigned unhex(unsigned char*s,int len){
	unsigned n=0,c;
	while(len-->0){
		switch(c=*s++){
			case'0':case'1':case'2':case'3':case'4':case'5':case'6':case'7':case'8':case'9':c-='0';break;
			case'a':case'b':case'c':case'd':case'e':case'f':c=c-'a'+10;break;
			case'A':case'B':case'C':case'D':case'E':case'F':c=c-'A'+10;break;
			default:return 0xffffffff;
		}
		n=(n<<4)|c;
	}
	return n;
}
#define PREFIX(str){str,sizeof(str)-1}
static const struct prefix_struct{const char*str;const size_t len;}prefixes[]={
	PREFIX("usb:"),
	PREFIX("product:"),
	PREFIX("model:"),
	PREFIX("device:"),
};
static const int num_prefixes=(sizeof(prefixes)/sizeof(prefixes[0]));
char*skip_host_serial(char*service){
	char*first_colon,*serial_end;
	int i;
	for(i=0;i<num_prefixes;i++)if(!strncmp(service,prefixes[i].str,prefixes[i].len))return strchr(service+prefixes[i].len,':');
	first_colon=strchr(service,':');
	if(!first_colon)return NULL;
	serial_end=first_colon;
	if(isdigit(serial_end[1])){
		serial_end++;
		while((*serial_end)&&isdigit(*serial_end))serial_end++;
		if((*serial_end)!=':')serial_end=first_colon;
	}
	return serial_end;
}
static int smart_socket_enqueue(asocket*s,apacket*p){
	unsigned len;
	if(s->pkt_first==0){
		s->pkt_first=p;
		s->pkt_last=p;
	}else{
		if((s->pkt_first->len + p->len)>MAX_PAYLOAD){
			free(p);
			goto fail;
		}
		memcpy(s->pkt_first->data + s->pkt_first->len,p->data,p->len);
		s->pkt_first->len +=p->len;
		free(p);
		p=s->pkt_first;
	}
	if(p->len<4)return 0;
	len=unhex(p->data,4);
	if(len<1||len>1024)goto fail;
	if((len+4)>p->len)return 0;
	p->data[len+4]=0;
	if(s->transport==NULL){
		char*error_string="unknown failure";
		if((s->transport=acquire_one_transport(CS_ANY,kTransportAny,NULL,&error_string))==NULL){
			sendfailmsg(s->peer->fd,error_string);
			goto fail;
		}
	}
	if(!(s->transport)||(s->transport->connection_state==CS_OFFLINE)){
		sendfailmsg(s->peer->fd,"device offline(x)");
		goto fail;
	}
	s->peer->ready=local_socket_ready_notify;
	s->peer->close=local_socket_close_notify;
	s->peer->peer=0;
	s->peer->transport=s->transport;
	connect_to_remote(s->peer,(char*)(p->data+4));
	s->peer=0;
	s->close(s);
	return 1;
fail:
	s->close(s);
	return -1;
}
static void smart_socket_ready(asocket*s __attribute__((unused))){}
static void smart_socket_close(asocket*s){
	if(s->pkt_first)free(s->pkt_first);
	if(s->peer){s->peer->peer=0;s->peer->close(s->peer);s->peer=0;}
	free(s);
}
asocket*create_smart_socket(
	void(*action_cb)(asocket*s __attribute__((unused)),
	const char*act __attribute__((unused)))
){
	asocket*s=calloc(1,sizeof(asocket));
	if(!s){
		tlog_error("cannot allocate socket");
		exit(1);
	}
	s->enqueue=smart_socket_enqueue;
	s->ready=smart_socket_ready;
	s->close=smart_socket_close;
	s->extra=action_cb;
	return s;
}
void smart_socket_action(
	asocket*s __attribute__((unused)),
	const char*act __attribute__((unused))
){}
void connect_to_smartsocket(asocket*s){
	asocket*ss=create_smart_socket(smart_socket_action);
	s->peer=ss,ss->peer=s;
	s->ready(s);
}
int android_get_control_socket(const char*name){
	char key[64]="adb_";
	const char*val;
	int fd;
	strncpy(key+4,name,sizeof(key)-4);
	key[sizeof(key)-1]='\0';
	if(!(val=getenv(key)))return -1;
	errno=0;
	fd=strtol(val,NULL,10);
	if(errno)return -1;
	return fd;
}
