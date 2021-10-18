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
#include<linux/usb/ch9.h>
#include<sys/ioctl.h>
#include<sys/types.h>
#include<errno.h>
#include"logger.h"
#include"defines.h"
#include"adbd_internal.h"
#define TAG "adbd"
#define ADB_CLASS 0xff
#define ADB_SUBCLASS 0x42
#define ADB_PROTOCOL 0x01
struct usb_handle{
	char*path;
	pthread_cond_t notify;
	pthread_mutex_t lock;
	int (*write)(usb_handle*h,const void*data,int len);
	int (*read)(usb_handle*h,void*data,int len);
	void (*kick)(usb_handle*h);
	int fd,control,bulk_out,bulk_in;
};
static const struct{
	struct usb_functionfs_descs_head header;
	struct{
		struct usb_interface_descriptor intf;
		struct usb_endpoint_descriptor_no_audio source,sink;
	}__attribute__((packed))fs_descs,hs_descs;
}__attribute__((packed)) descriptors={
	.header={
		.magic=FUNCTIONFS_DESCRIPTORS_MAGIC,
		.length=sizeof(descriptors),
		.fs_count=3,
		.hs_count=3,
	},
	.fs_descs={
		.intf={
			.bLength=sizeof(descriptors.fs_descs.intf),
			.bDescriptorType=USB_DT_INTERFACE,
			.bInterfaceNumber=0,
			.bNumEndpoints=2,
			.bInterfaceClass=ADB_CLASS,
			.bInterfaceSubClass=ADB_SUBCLASS,
			.bInterfaceProtocol=ADB_PROTOCOL,
			.iInterface=1,
		},
		.source={
			.bLength=sizeof(descriptors.fs_descs.source),
			.bDescriptorType=USB_DT_ENDPOINT,
			.bEndpointAddress=1|USB_DIR_OUT,
			.bmAttributes=USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize=MAX_PACKET_SIZE_FS,
		},
		.sink={
			.bLength=sizeof(descriptors.fs_descs.sink),
			.bDescriptorType=USB_DT_ENDPOINT,
			.bEndpointAddress=2|USB_DIR_IN,
			.bmAttributes=USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize=MAX_PACKET_SIZE_FS,
		},
	},
	.hs_descs={
		.intf={
			.bLength=sizeof(descriptors.hs_descs.intf),
			.bDescriptorType=USB_DT_INTERFACE,
			.bInterfaceNumber=0,
			.bNumEndpoints=2,
			.bInterfaceClass=ADB_CLASS,
			.bInterfaceSubClass=ADB_SUBCLASS,
			.bInterfaceProtocol=ADB_PROTOCOL,
			.iInterface=1,
		},
		.source={
			.bLength=sizeof(descriptors.hs_descs.source),
			.bDescriptorType=USB_DT_ENDPOINT,
			.bEndpointAddress=1|USB_DIR_OUT,
			.bmAttributes=USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize=MAX_PACKET_SIZE_HS,
		},
		.sink={
			.bLength=sizeof(descriptors.hs_descs.sink),
			.bDescriptorType=USB_DT_ENDPOINT,
			.bEndpointAddress=2|USB_DIR_IN,
			.bmAttributes=USB_ENDPOINT_XFER_BULK,
			.wMaxPacketSize=MAX_PACKET_SIZE_HS,
		},
	},
};
#define STR_INTERFACE_ "ADB Interface"
static const struct{
	struct usb_functionfs_strings_head header;
	struct{
		__le16 code;
		const char str1[sizeof(STR_INTERFACE_)];
	}__attribute__((packed))lang0;
}__attribute__((packed))strings={
	.header={
		.magic=FUNCTIONFS_STRINGS_MAGIC,
		.length=sizeof(strings),
		.str_count=1,
		.lang_count=1,
	},
	.lang0={
		0x0409,
		STR_INTERFACE_,
	},
};
static void init_functionfs(struct usb_handle*h){
	char ep0[PATH_MAX]={0},out[PATH_MAX]={0},in[PATH_MAX]={0};
	snprintf(ep0,sizeof(ep0)-1,"%s/ep0",h->path);
	snprintf(out,sizeof(out)-1,"%s/ep1",h->path);
	snprintf(in,sizeof(in)-1,"%s/ep2",h->path);
	if((h->control=adb_open(ep0,O_RDWR))<0) {
		telog_error(" %s cannot open control endpoint",h->path);
		goto err;
	}
	if(adb_write(h->control,&descriptors,sizeof(descriptors))<0){
		telog_error("%s write descriptors failed",h->path);
		goto err;
	}
	if(adb_write(h->control,&strings,sizeof(strings))<0){
		telog_error("%s writing strings failed",h->path);
		goto err;
	}
	if((h->bulk_out=adb_open(out,O_RDWR))<0){
		telog_error("%s cannot open bulk-out ep",h->path);
		goto err;
	}
	if((h->bulk_in=adb_open(in,O_RDWR))<0){
		telog_error("%s cannot open bulk-in ep",h->path);
		goto err;
	}
	return;
err:
	if(h->bulk_in>0)close(h->bulk_in);
	if(h->bulk_out>0)close(h->bulk_out);
	if(h->control>0)close(h->control);
	h->bulk_in=-1,h->bulk_out=-1,h->control=-1;
}
static _Noreturn void*usb_ffs_open_thread(void*x){
	struct usb_handle*usb=(struct usb_handle*)x;
	for(;;){
		pthread_mutex_lock(&usb->lock);
		while(usb->control!=-1)pthread_cond_wait(&usb->notify,&usb->lock);
		pthread_mutex_unlock(&usb->lock);
		for(;;){
			init_functionfs(usb);
			adbd_send_ok();
			if(usb->control>=0)break;
			usleep(1000000);
		}
		register_usb_transport(usb,0,0,1);
	}
}
static int bulk_write(int bulk_in,const char*buf,size_t length){
	size_t count=0;
	int ret;
	do{
		if((ret=adb_write(bulk_in,buf+count,length-count))>=0)count+=ret;
		else if(errno!=EINTR)return terlog_warn(
			-1,"bulk write failed fd %d length %ld count %ld",
			bulk_in,length,count
		);
	}while(count<length);
	return count;
}
static int usb_ffs_write(usb_handle*h,const void*data,int len){
	int n;
	if((n=bulk_write(h->bulk_in,data,len))==len)return 0;
	telog_warn("usb ffs fd %d write %d",h->bulk_out,n);
	return -1;
}
static int bulk_read(int bulk_out,char*buf,size_t length){
	size_t count=0;
	int ret;
	do{
		if((ret=adb_read(bulk_out,buf+count,length-count))>=0)count+=ret;
		else if(errno!=EINTR)return terlog_warn(
			-1,"bulk read failed fd %d length %ld count %ld",
			bulk_out,length,count
		);
	}while(count<length);
	return count;
}
static int usb_ffs_read(usb_handle*h,void*data,int len){
	int n;
	if((n=bulk_read(h->bulk_out,data,len))==len)return 0;
	telog_warn("usb ffs fd %d read %d",h->bulk_out,n);
	return -1;
}
static void usb_ffs_kick(usb_handle*h){
	if(ioctl(h->bulk_in,FUNCTIONFS_CLEAR_HALT)<0)
		telog_warn("usb ffs kick source fd %d clear halt failed",h->bulk_in);
	if(ioctl(h->bulk_out,FUNCTIONFS_CLEAR_HALT)<0)
		telog_warn("usb ffs kick sink fd %d clear halt failed",h->bulk_out);
	pthread_mutex_lock(&h->lock);
	close(h->control);
	close(h->bulk_out);
	close(h->bulk_in);
	h->control=h->bulk_out=h->bulk_in=-1;
	pthread_cond_signal(&h->notify);
	pthread_mutex_unlock(&h->lock);
}
void usb_init(char*path){
	usb_handle*h;
	pthread_t tid;
	if(!(h=calloc(1,sizeof(usb_handle)))){
		telog_error("cannot allocate usb handle");
		exit(-1);
	}
	h->path=path;
	h->write=usb_ffs_write;
	h->read=usb_ffs_read;
	h->kick=usb_ffs_kick;
	h->control =-1;
	h->bulk_out=-1;
	h->bulk_out=-1;
	pthread_cond_init(&h->notify,0);
	pthread_mutex_init(&h->lock,0);
	if(adb_thread_create(&tid,usb_ffs_open_thread,h)!=0){
		free(h);
		telog_error("cannot create usb thread");
		exit(-1);
	}
}
int usb_write(usb_handle*h,const void*data,int len){return h->write(h,data,len);}
int usb_read(usb_handle*h,void*data,int len){return h->read(h,data,len);}
int usb_close(usb_handle*h __attribute__((unused))){return 0;}
void usb_kick(usb_handle*h){h->kick(h);}
