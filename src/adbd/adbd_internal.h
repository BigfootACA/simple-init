#ifndef __ADB_H
#define __ADB_H
#include"adbd.h"
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>
#include<pthread.h>
#include<sys/un.h>
#include<netinet/in.h>
#include<netinet/tcp.h>
#include<linux/types.h>
#define ANDROID_SOCKET_NAMESPACE_ABSTRACT 0
#define ANDROID_SOCKET_NAMESPACE_RESERVED 1
#define ANDROID_SOCKET_NAMESPACE_FILESYSTEM 2
#define MAX_PAYLOAD 4096
#define A_SYNC 0x434e5953
#define A_CNXN 0x4e584e43
#define A_OPEN 0x4e45504f
#define A_OKAY 0x59414b4f
#define A_CLSE 0x45534c43
#define A_WRTE 0x45545257
#define A_AUTH 0x48545541
#define A_VERSION 0x01000000
#define ADB_AUTH_TOKEN         1
#define ADB_AUTH_SIGNATURE     2
#define ADB_AUTH_RSAPUBLICKEY  3
#define	FUNCTIONFS_CLEAR_HALT	_IO('g', 3)
#define FDE_READ              0x0001
#define FDE_WRITE             0x0002
#define FDE_ERROR             0x0004
#define FDE_TIMEOUT           0x0008
#define FDE_DONT_CLOSE        0x0080
#define LISTEN_BACKLOG 4
#define MKID(a,b,c,d) ((a)|((b)<<8)|((c)<<16)|((d)<<24))
#define ID_STAT MKID('S','T','A','T')
#define ID_LIST MKID('L','I','S','T')
#define ID_ULNK MKID('U','L','N','K')
#define ID_SEND MKID('S','E','N','D')
#define ID_RECV MKID('R','E','C','V')
#define ID_DENT MKID('D','E','N','T')
#define ID_DONE MKID('D','O','N','E')
#define ID_DATA MKID('D','A','T','A')
#define ID_OKAY MKID('O','K','A','Y')
#define ID_FAIL MKID('F','A','I','L')
#define ID_QUIT MKID('Q','U','I','T')
#define MAX_PACKET_SIZE_FS 64
#define MAX_PACKET_SIZE_HS 512
#define SYNC_DATA_MAX (64*1024)
#define CS_ANY -1
#define CS_OFFLINE 0
#define CS_BOOTLOADER 1
#define CS_DEVICE 2
#define CS_HOST 3
#define CS_RECOVERY 4
#define CS_NOPERM 5
#define CS_SIDELOAD 6
#define CHUNK_SIZE (64*1024)
#define TOKEN_SIZE 20
#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(exp)({typeof(exp)_rc;do{_rc=(exp);}while(_rc==-1&&errno==EINTR);_rc;})
#endif
typedef struct fdevent fdevent;
typedef void (*fd_func)(int fd,unsigned events,void*userdata);
typedef struct amessage amessage;
typedef struct apacket apacket;
typedef struct asocket asocket;
typedef struct alistener alistener;
typedef struct atransport atransport;
typedef struct adisconnect adisconnect;
typedef struct usb_handle usb_handle;
typedef enum{BACKUP,RESTORE}BackupOperation;
typedef enum transport_type{kTransportUsb,kTransportLocal,kTransportAny,kTransportHost,}transport_type;
typedef union{
	unsigned id;
	struct{unsigned id,namelen;}req;
	struct{unsigned id,mode,size,time;}stat;
	struct{unsigned id,mode,size,time,namelen;}dent;
	struct{unsigned id,size;}data;
	struct{unsigned id,msglen;}status;
}syncmsg;
struct logger_entry{
	uint16_t len,__pad;
	int32_t pid,tid,sec,nsec;
	char msg[0];
};
struct listnode{struct listnode*next,*prev;};
struct fdevent{
	fdevent *next,*prev;
	int fd,force_eof;
	unsigned short state,events;
	fd_func func;
	void *arg;
};
enum{
	FUNCTIONFS_DESCRIPTORS_MAGIC=1,
	FUNCTIONFS_STRINGS_MAGIC=2
};
enum usb_functionfs_event_type {
	FUNCTIONFS_BIND,FUNCTIONFS_UNBIND,
	FUNCTIONFS_ENABLE,FUNCTIONFS_DISABLE,
	FUNCTIONFS_SETUP,
	FUNCTIONFS_SUSPEND,FUNCTIONFS_RESUME
};
struct amessage{unsigned command,arg0,arg1,data_length,data_check,magic;};
struct apacket{
	apacket*next;
	unsigned len;
	unsigned char*ptr;
	amessage msg;
	unsigned char data[MAX_PAYLOAD];
};
struct asocket{
	asocket*next,*prev;
	unsigned id;
	int closing,exit_on_close;
	asocket*peer;
	fdevent fde;
	int fd;
	apacket*pkt_first,*pkt_last;
	int (*enqueue)(asocket*s,apacket*pkt);
	void(*ready)(asocket*s),(*close)(asocket*s);
	void*extra;
	atransport*transport;
};
struct adisconnect{
	void(*func)(void*opaque,atransport*t);
	void*opaque;
	adisconnect*next,*prev;
};
struct atransport{
	atransport*next,*prev;
	int (*read_from_remote)(apacket*p,atransport*t),(*write_to_remote)(apacket*p,atransport*t);
	void (*close)(atransport*t),(*kick)(atransport*t);
	int fd,transport_socket;
	fdevent transport_fde;
	int ref_count;
	unsigned sync_token;
	int connection_state,online;
	transport_type type;
	usb_handle*usb;
	int sfd;
	char*serial,*product,*model,*device,*devpath;
	int adb_port;
	int kicked;
	adisconnect disconnects;
	void*key;
	unsigned char token[TOKEN_SIZE];
	fdevent auth_fde;
	unsigned failed_auth_attempts;
};
struct alistener{
	alistener*next,*prev;
	fdevent fde;
	int fd;
	char*local_name,*connect_to;
	atransport*transport;
	adisconnect disconnect;
};
struct usb_endpoint_descriptor_no_audio{
	__u8  bLength,bDescriptorType,bEndpointAddress,bmAttributes;
	__le16 wMaxPacketSize;
	__u8  bInterval;
}__attribute__((packed));
struct usb_functionfs_descs_head{__le32 magic,length,fs_count,hs_count;}__attribute__((packed));
struct usb_functionfs_strings_head{__le32 magic,length,str_count,lang_count;}__attribute__((packed));
extern int shell_exit_notify_fd;
extern pthread_mutex_t socket_list_lock;
extern pthread_mutex_t transport_lock;
extern pthread_mutex_t D_lock;
extern void file_sync_service(int fd,void*cookie);
extern fdevent*fdevent_create(int fd,fd_func func,void*arg);
extern void fdevent_destroy(fdevent*fde);
extern void fdevent_install(fdevent*fde,int fd,fd_func func,void *arg);
extern void fdevent_remove(fdevent*item);
extern void fdevent_set(fdevent*fde,unsigned events);
extern void fdevent_add(fdevent*fde,unsigned events);
extern void fdevent_del(fdevent*fde,unsigned events);
extern void fdevent_loop(void);
extern asocket*find_local_socket(unsigned id);
extern void install_local_socket(asocket*s);
extern void remove_socket(asocket*s);
extern void close_all_sockets(atransport*t);
extern asocket*create_local_socket(int fd);
extern asocket*create_local_service_socket(const char*destination);
extern asocket*create_remote_socket(unsigned id,atransport*t);
extern void connect_to_remote(asocket*s,const char*destination);
extern void connect_to_smartsocket(asocket*s);
extern void handle_packet(apacket*p,atransport*t);
extern void send_packet(apacket*p,atransport*t);
extern void init_transport_registration(void);
extern void update_transports(void);
extern atransport*acquire_one_transport(int state,transport_type ttype,const char*serial,char**error_out);
extern void add_transport_disconnect(atransport*t,adisconnect*dis);
extern void remove_transport_disconnect(atransport*t,adisconnect*dis);
extern void run_transport_disconnects(atransport*t);
extern void kick_transport(atransport*t);
extern int init_socket_transport(atransport*t,int s,int port,int local);
extern void init_usb_transport(atransport*t,usb_handle*usb,int state);
extern void register_socket_transport(int s,const char*serial,int port,int local);
extern void register_usb_transport(usb_handle*h,const char*serial,const char*devpath,unsigned writeable);
extern void unregister_usb_transport(usb_handle*usb);
extern int service_to_fd(const char*name);
extern apacket*get_apacket(void);
extern int check_header(apacket*p);
extern int check_data(apacket*p);
extern void local_init(int port);
extern int local_connect(int port);
extern int local_connect_arbitrary_ports(int console_port,int adb_port);
extern void usb_init(char*);
extern int usb_write(usb_handle*h,const void*data,int len);
extern int usb_read(usb_handle*h,void*data,int len);
extern int usb_close(usb_handle*h);
extern void usb_kick(usb_handle*h);
extern void adb_auth_init(void);
extern void adb_auth_verified(atransport*t);
extern int adb_auth_generate_token(void*token,size_t token_size);
extern int adb_auth_verify(void*token,void*sig,int siglen);
extern void adb_auth_confirm_key(unsigned char*data,size_t len,atransport*t);
extern int readx(int,void*,size_t);
extern int writex(int,const void*,size_t);
extern int sendfailmsg(int fd,const char*reason);
extern int handle_host_request(char*service,transport_type ttype,char*serial,int reply_fd,asocket*s);
extern int socket_loopback_client(int port,int type);
extern int socket_loopback_server(int port,int type);
extern int socket_local_server(const char*name,int namespaceId,int type);
extern int socket_local_server_bind(int s,const char*name,int namespaceId);
extern int socket_local_client_connect(int fd,const char*name,int namespaceId,int type);
extern int socket_local_client(const char*name,int namespaceId,int type);
extern int socket_inaddr_any_server(int port,int type);
extern int socket_make_sockaddr_un(const char*name,int namespaceId,struct sockaddr_un*p_addr,socklen_t*alen);
extern int android_get_control_socket(const char*name);
extern char*adbd_get_shell(void);
extern void adbd_send_ok(void);
static __inline__ int adb_open_mode(const char*pathname,int options,int mode){return TEMP_FAILURE_RETRY(open(pathname,options,mode));}
static __inline__ int adb_open(const char*pathname,int options){
	int fd=TEMP_FAILURE_RETRY(open(pathname,options));
	if(fd<0)return -1;
	fcntl(fd,F_SETFD,FD_CLOEXEC);
	return fd;
}
static __inline__ int adb_read(int fd,void*buf,size_t len){return TEMP_FAILURE_RETRY(read(fd,buf,len));}
static __inline__ int adb_write(int fd,const void*buf,size_t len){return TEMP_FAILURE_RETRY(write(fd,buf,len));}
static __inline__ int adb_socket_accept(int serverfd,struct sockaddr*addr,socklen_t*addrlen){
	int fd;
	fd=TEMP_FAILURE_RETRY(accept(serverfd,addr,addrlen));
	if(fd>=0)fcntl(fd,F_SETFD,FD_CLOEXEC);
	return fd;
}
typedef void*(*adb_thread_func_t)(void*arg);
static __inline__ int adb_thread_create(pthread_t*pthread,adb_thread_func_t start,void*arg){
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	return pthread_create(pthread,&attr,start,arg);
}
static __inline__ void disable_tcp_nagle(int fd){int on=1;setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,(void*)&on,sizeof(on));}
static __inline__ int adb_socketpair(int sv[2]){
	if(socketpair(AF_UNIX,SOCK_STREAM,0,sv)<0)return -1;
	fcntl(sv[0],F_SETFD,FD_CLOEXEC);
	fcntl(sv[1],F_SETFD,FD_CLOEXEC);
	return 0;
}
#endif
