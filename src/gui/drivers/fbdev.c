#ifdef ENABLE_GUI
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
#include<semaphore.h>
#include"gui.h"
#include"confd.h"
#include"logger.h"
#include"system.h"
#include"defines.h"
#include"hardware.h"
#include"pathnames.h"
#include"gui/guidrv.h"
#define TAG "fbdev"
static pthread_t fbrt;
static bool blank=false,swap_abgr=false;
static char*fbp=0;
static long int screensize=0;
static int fbfd=-1;
static sem_t flush;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
static lv_color_t buf[846000];
static lv_disp_draw_buf_t disp_buf;
static lv_disp_drv_t disp_drv;
static void*fbdev_refresh(void*args __attribute__((unused))){
	for(;;){
		sem_wait(&flush);
		ioctl(fbfd,FBIOPAN_DISPLAY,&vinfo);
	}
	return NULL;
}
static int fbdev_refresher_start(){
	if(fbrt)return terlog_error(-1,"refresher thread already running");
	sem_init(&flush,0,0);
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
static int _fbdev_init_fd(){
	if(_fbdev_get_info()<0)return -1;
	screensize=finfo.smem_len;
	fbp=(char*)mmap(0,screensize,PROT_READ|PROT_WRITE,MAP_SHARED,fbfd,0);
	if((intptr_t)fbp==-1)return terlog_error(-1,"mmap");
	memset(fbp,0,screensize);
	ioctl(fbfd,FBIOPAN_DISPLAY,&vinfo);
	ioctl(fbfd,FBIOBLANK,0);
	return 0;
}
static int vtconsole_all_bind(int value){
	int f=open(_PATH_SYS_CLASS"/vtconsole",O_DIR);
	if(f<0)return -1;
	DIR*d=fdopendir(f);
	if(!d){
		close(f);
		return -1;
	}
	struct dirent*e;
	while((e=readdir(d))){
		if(e->d_type!=DT_LNK)continue;
		if(strncmp(e->d_name,"vtcon",5)!=0)continue;
		int x=openat(f,e->d_name,O_DIR);
		if(x<=0)continue;
		fd_write_int(x,"bind",value,true);
		close(x);
	}
	closedir(d);
	return 0;
}
static void fbdev_exit(void){
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
	vtconsole_all_bind(1);
}
static inline uint32_t swap(uint32_t x){
	return swap_abgr?
		((x&0xff000000))|
		((x&0x00ff0000)>>16)|
		((x&0x0000ff00))|
		((x&0x000000ff)<<16)
		:x;
}
static inline void copy_swapped(uint32_t*a,const uint32_t*b,size_t l){
	while(l--)*(uint32_t*)a++=swap(*(uint32_t*)b++);
}
static void fbdev_flush(lv_disp_drv_t*drv,const lv_area_t*area,lv_color_t*color_p){
	if(fbp==NULL||area->x2<0||area->y2<0||area->x1>(int32_t)vinfo.xres-1||area->y1>(int32_t)vinfo.yres-1){
		lv_disp_flush_ready(drv);
		return;
	}
	int32_t act_x1=area->x1<0?0:area->x1,act_y1=area->y1<0?0:area->y1;
	int32_t act_x2=area->x2>(int32_t)vinfo.xres-1?(int32_t)vinfo.xres-1:area->x2;
	int32_t act_y2=area->y2>(int32_t)vinfo.yres-1?(int32_t)vinfo.yres-1:area->y2;
	lv_coord_t w=(act_x2-act_x1+1);
	long int location,byte_location;
	unsigned char bit_location;
	if(vinfo.bits_per_pixel==32||vinfo.bits_per_pixel==24){
		uint32_t*fbp32=(uint32_t*)fbp;
		for(int32_t y=act_y1;y<=act_y2;y++){
			location=(act_x1+vinfo.xoffset)+(y+vinfo.yoffset)*finfo.line_length/4;
			copy_swapped(&fbp32[location],(uint32_t *)color_p,act_x2-act_x1+1);
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
	if(fbrt)sem_post(&flush);
	lv_disp_flush_ready(drv);
}
static int _fbdev_register(){
	if(vinfo.xres<=0||vinfo.yres<=0){
		fbdev_exit();
		return -1;
	}
	lv_disp_draw_buf_init(&disp_buf,buf,NULL,sizeof(buf));
	lv_disp_drv_init(&disp_drv);
	disp_drv.hor_res=vinfo.xres;
	disp_drv.ver_res=vinfo.yres;
	switch(gui_rotate){
		case 0:break;
		case 90:disp_drv.sw_rotate=1,disp_drv.rotated=LV_DISP_ROT_90;break;
		case 180:disp_drv.sw_rotate=1,disp_drv.rotated=LV_DISP_ROT_180;break;
		case 270:disp_drv.sw_rotate=1,disp_drv.rotated=LV_DISP_ROT_270;break;
	}
	tlog_notice("screen resolution: %dx%d",vinfo.xres,vinfo.yres);
	disp_drv.draw_buf=&disp_buf;
	disp_drv.flush_cb=fbdev_flush;
	disp_drv.draw_ctx_init=lv_draw_sw_init_ctx;
	disp_drv.draw_ctx_deinit=lv_draw_sw_init_ctx;
	disp_drv.draw_ctx_size=sizeof(lv_draw_sw_ctx_t);
	set_active_console(7);
	vtconsole_all_bind(0);
	lv_disp_drv_register(&disp_drv);
	fbdev_refresher_start();
	return 0;
}
static char*_fbdev_get_driver_name(int fd,char*buff,size_t len){
	char str[128],*ret=NULL;
	memset(str,0,sizeof(str));
	if(!fd_is_link(fd,"device/driver")){
		if(fd_is_file(fd,"msm_fb_type"))ret="msmfb";
	}else if(readlinkat(fd,"device/driver",str,127)>0)ret=basename(str);
	if(ret)strncpy(buff,ret,len-1);
	return ret;
}
static int _fbdev_scan(){
	int sfd,dfd;
	char*dfmt,*dgfmt,*sfmt,*driver;
	char drbuff[128],sdev[256],ddev[256];
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
			}else if(!strcmp(driver,"msmfb")){
				tlog_debug("detect device fb%d msm framebuffer, enable ABGR swap",i);
				swap_abgr=true;
			}
		}
		swap_abgr=confd_get_boolean("runtime.cmdline.abgr",swap_abgr);
		close(sfd);
		return dfd;
	}
	tlog_error("no fbdev found.");
	return -1;
}
static int fbdev_scan_init(){
	int fd;
	if((fd=_fbdev_scan())<0)return trlog_error(-1,"init scan failed");
	fbfd=fd;
	if(_fbdev_init_fd()<0)return trlog_error(-1,"init failed");
	return 0;
}
static int fbdev_scan_init_register(){
	if(fbdev_scan_init()<0)return -1;
	if(_fbdev_register()<0)return -1;
	return 0;
}
static void fbdev_get_sizes(lv_coord_t*width,lv_coord_t*height){
	lv_coord_t w=0,h=0;
	switch(gui_rotate){
		case 0:case 180:w=vinfo.xres,h=vinfo.yres;break;
		case 90:case 270:w=vinfo.yres,h=vinfo.xres;break;
	}
	if(width)*width=w;
	if(height)*height=h;
}
static int fbdev_get_brightness(){
	if(default_backlight<0)ERET(ENOTSUP);
	return led_get_brightness_percent(default_backlight);
}
static void fbdev_set_brightness(int value){
	errno=0;
	if(blank){
		if(value>0){
			ioctl(fbfd,FBIOBLANK,0);
			telog_debug("screen resume");
			blank=!blank;
		}
	}else{
		if(value<=0){
			ioctl(fbfd,FBIOBLANK,1);
			telog_debug("screen suspend");
			blank=!blank;
		}
	}
	if(default_backlight<0){
		errno=ENOTSUP;
		return;
	}
	led_set_brightness_percent(default_backlight,value);
	telog_debug("set backlight to %d%%",value);
}
struct gui_driver guidrv_fbdev={
	.name="fbdev",
	.drv_register=fbdev_scan_init_register,
	.drv_getsize=fbdev_get_sizes,
	.drv_exit=fbdev_exit,
	.drv_getbrightness=fbdev_get_brightness,
	.drv_setbrightness=fbdev_set_brightness,
};
#endif
