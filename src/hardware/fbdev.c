#define _GNU_SOURCE
#include<errno.h>
#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<stddef.h>
#include<string.h>
#include<stdbool.h>
#include<pthread.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<sys/ioctl.h>
#include<linux/fb.h>
#include<linux/vt.h>
#include<linux/kd.h>
#include"pathnames.h"
#include"logger.h"
#include"lvgl.h"
#include"hardware.h"
#define TAG "fbdev"
pthread_t fbrt;
static char*fbp=0;
static long int screensize=0;
int fbfd=-1,vtfd=-1;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
static void*fbdev_refresh(void*args __attribute__((unused))){
	while(fbfd>0){
		ioctl(fbfd,FBIOPAN_DISPLAY,&vinfo);
		usleep(40000);
	}
	return NULL;
}
static int fbdev_refresher_start(){
	if(fbrt)return terlog_error(-1,"refresher thread already running");
	if(pthread_create(&fbrt,NULL,fbdev_refresh,(void*)0)!=0)
		return terlog_error(-1,"failed to start refresher thread");
	else pthread_setname_np(fbrt,"FrameBuffer Refresher Thread");
	return 0;
}
static int _fbdev_get_info(){
	if(ioctl(fbfd,FBIOGET_FSCREENINFO,&finfo)==-1)
		return terlog_error(-1,"ioctl FBIOGET_FSCREENINFO");
	if(ioctl(fbfd,FBIOGET_VSCREENINFO,&vinfo)==-1)
		return terlog_error(-1,"ioctl FBIOGET_VSCREENINFO");
	return 0;
}
static int _tty_init(){//from xorg-server
	int fd;
	if((fd=open(_PATH_DEV"/tty0",O_WRONLY,0))<0)
		return terlog_error(-1,"open tty0");
	int vt=-1;
	if(ioctl(fd,VT_OPENQRY,&vt)<0){
		close(fd);
		return terlog_error(-1,"ioctl VT_OPENQRY");
	}
	close(fd);
	if(vt<0||vt>64)return trlog_error(-1,"cannot find a free vt");
	char vtname[16]={0};
	snprintf(vtname,sizeof(vtname),_PATH_DEV"/tty%d",vt);
	if((fd=open(vtname,O_RDWR|O_NDELAY,0))<0)return terlog_error(-1,"open %s",vtname);
	struct vt_stat vts;
	if(ioctl(fd,VT_GETSTATE,&vts)<0){
		close(fd);
		return terlog_error(-1,"ioctl VT_GETSTATE");
	}
	if(ioctl(fd,VT_ACTIVATE,vt)<0){
		close(fd);
		return terlog_error(-1,"ioctl VT_ACTIVATE");
	}
	if(ioctl(fd,VT_WAITACTIVE,vt)<0){
		close(fd);
		return terlog_error(-1,"ioctl VT_WAITACTIVE");
	}
	if(ioctl(fd,KDSETMODE,KD_GRAPHICS)<0){
		close(fd);
		return terlog_error(-1,"ioctl KDSETMODE");
	}
	if(ioctl(fd,KDSKBMODE,K_OFF)<0){
		close(fd);
		return terlog_error(-1,"ioctl KDSKBMODE");
	}
	return (vtfd=fd);
}
static int _fbdev_init_fd(){
	if(_fbdev_get_info()<0)return -1;
	screensize=finfo.smem_len;
	fbp=(char*)mmap(0,screensize,PROT_READ|PROT_WRITE,MAP_SHARED,fbfd,0);
	if((intptr_t)fbp==-1)return terlog_error(-1,"mmap");
	memset(fbp,0,screensize);
	ioctl(fbfd,FBIOPAN_DISPLAY,&vinfo);
	ioctl(fbfd,FBIOBLANK,0);
	set_brightness_percent("lcd-backlight",100);
	return 0;
}
int fbdev_init(char*dev){
	if(!dev)return -1;
	_tty_init();
	if((fbfd=open(dev,O_RDWR))<0)return terlog_error(-1,"open device");
	if(_fbdev_init_fd()<0)return -1;
	tlog_info("init fbdev %s done",dev);
	return 0;
}
void fbdev_exit(void){
	if(fbp){
		memset(fbp,0,screensize);
		ioctl(fbfd,FBIOPAN_DISPLAY,&vinfo);
		munmap(fbp,screensize);
		fbp=NULL;
	}
	if(fbfd>=0){
		close(fbfd);
		fbfd=-1;
	}
}
static int _fbdev_register(){
	if(vinfo.xres<=0||vinfo.yres<=0){
		fbdev_exit();
		return -1;
	}
	static lv_color_t buf[846000];
	static lv_disp_buf_t disp_buf;
	lv_disp_buf_init(&disp_buf,buf,NULL,846000);
	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.hor_res=vinfo.xres;
	disp_drv.ver_res=vinfo.yres;
	tlog_notice("screen resolution: %dx%d",vinfo.xres,vinfo.yres);
	disp_drv.buffer=&disp_buf;
	disp_drv.flush_cb=fbdev_flush;
	lv_disp_drv_register(&disp_drv);
	fbdev_refresher_start();
	return 0;
}
static char*_fbdev_get_driver_name(int fd,char*buff,size_t len){
	struct stat s;
	char buf[128]={0},*ret=NULL;
	memset(&s,0,sizeof(s));
	if(fstatat(fd,"device/driver",&s,0)<0||!S_ISLNK(s.st_mode)){
		memset(&s,0,sizeof(s));
		if(fstatat(fd,"msm_fb_type",&s,0)>0&&S_ISREG(s.st_mode))ret="msmfb";
	}else if(readlinkat(fd,"device/driver",buf,127)>0)ret=basename(buf);
	if(ret)strncpy(buff,ret,len-1);
	return ret;
}
static int _fbdev_scan(){
	int sfd,dfd;
	char*dfmt,*dgfmt,*sfmt,*driver;
	char drbuff[128]={0},sdev[256]={0},ddev[256]={0};
	bool x=access(_PATH_DEV"/graphics",F_OK)==0;
	if(!x&&errno!=ENOENT)return terlog_error(-1,"access "_PATH_DEV"/graphics");
	dgfmt=_PATH_DEV"/graphics/fb%d";
	dfmt=_PATH_DEV"/fb%d";
	sfmt=_PATH_SYS_CLASS"/graphics/fb%d";
	for(int i=0;i<32;i++){
		memset(sdev,0,256);
		memset(ddev,0,256);
		snprintf(sdev,255,sfmt,i);
		snprintf(ddev,255,dfmt,i);
		if((sfd=open(sdev,O_DIRECTORY|O_RDONLY))<0){
			if(errno!=ENOENT)telog_warn("open sysfs class graphics dev fb%d",i);
			continue;
		}
		if((dfd=open(ddev,O_RDWR))<0){
			if(errno!=ENOENT)telog_warn("open device %s",ddev);
			memset(ddev,0,256);
			snprintf(ddev,255,dgfmt,i);
			if((dfd=open(ddev,O_RDWR))<0){
				if(errno!=ENOENT)telog_warn("open device %s",ddev);
				close(sfd);
				continue;
			}
		}
		tlog_debug("found framebuffer device %s",ddev);
		memset(drbuff,0,127);
		if((driver=_fbdev_get_driver_name(sfd,drbuff,127))){
			tlog_info("scan framebuffer device fb%d use driver %s",i,driver);
			if(!strcmp(driver,"vfb")){
				tlog_debug("device fb%d seems to be Virtual FrameBuffer, skip",i);
				close(sfd);
				close(dfd);
				continue;
			}
		}
		close(sfd);
		return dfd;
	}
	tlog_error("no fbdev found.");
	return -1;
}
int fbdev_scan_init(){
	int fd;
	if((fd=_fbdev_scan())<0)return trlog_error(-1,"init scan failed");
	fbfd=fd;
	if(_fbdev_init_fd()<0)return trlog_error(-1,"init failed");
	return 0;
}
int fbdev_scan_init_register(){
	if(fbdev_scan_init()<0)return -1;
	return _fbdev_register();
}
int fbdev_init_register(char*fbdev){
	if(fbdev_init(fbdev)<0)return -1;
	return _fbdev_register();
}
void fbdev_flush(lv_disp_drv_t*drv,const lv_area_t*area,lv_color_t*color_p){
	if(fbp==NULL||area->x2<0||area->y2<0||area->x1>(int32_t)vinfo.xres-1||area->y1>(int32_t)vinfo.yres-1){
		lv_disp_flush_ready(drv);
		return;
	}
	int32_t act_x1=area->x1<0?0:area->x1,act_y1=area->y1<0?0:area->y1;
	int32_t act_x2=area->x2>(int32_t)vinfo.xres-1?(int32_t)vinfo.xres-1:area->x2,act_y2=area->y2>(int32_t)vinfo.yres-1?(int32_t)vinfo.yres-1:area->y2;
	lv_coord_t w=(act_x2-act_x1+1);
	long int location,byte_location;
	unsigned char bit_location;
	if(vinfo.bits_per_pixel==32||vinfo.bits_per_pixel==24){
		uint32_t*fbp32=(uint32_t*)fbp;
		for(int32_t y=act_y1;y<=act_y2;y++){
			location=(act_x1+vinfo.xoffset)+(y+vinfo.yoffset)*finfo.line_length/4;
			memcpy(&fbp32[location],(uint32_t*)color_p,(act_x2-act_x1+1)*4);
			color_p+=w;
		}
	}else if(vinfo.bits_per_pixel==16){
		uint16_t*fbp16=(uint16_t*)fbp;
		for(int32_t y=act_y1;y<=act_y2;y++){
			location=(act_x1+vinfo.xoffset)+(y+vinfo.yoffset)*finfo.line_length/2;
			memcpy(&fbp16[location],(uint32_t*)color_p,(act_x2-act_x1+1)*2);
			color_p+=w;
		}
	}else if(vinfo.bits_per_pixel==8){
		uint8_t*fbp8=(uint8_t*)fbp;
		for(int32_t y=act_y1;y<=act_y2;y++){
			location=(act_x1+vinfo.xoffset)+(y+vinfo.yoffset)*finfo.line_length;
			memcpy(&fbp8[location],(uint32_t*)color_p,(act_x2-act_x1+1));
			color_p+=w;
		}
	}else if(vinfo.bits_per_pixel==1){
		uint8_t*fbp8=(uint8_t*)fbp;
		for(int32_t y=act_y1;y<=act_y2;y++){
			for(int32_t x=act_x1;x<=act_x2;x++){
				location=(x+vinfo.xoffset)+(y+vinfo.yoffset)*vinfo.xres;
				byte_location=location/8;
				bit_location=location%8;
				fbp8[byte_location]&=~(((uint8_t)(1))<<bit_location);
				fbp8[byte_location]|=((uint8_t)(color_p->full))<<bit_location;
				color_p++;
			}
			color_p+=area->x2-act_x2;
		}
	}
	lv_disp_flush_ready(drv);
}
void fbdev_get_sizes(uint32_t*width,uint32_t*height){
	if(width)*width=vinfo.xres;
	if(height)*height=vinfo.yres;
}