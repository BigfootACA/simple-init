#ifdef ENABLE_GUI
#ifdef ENABLE_DRM
#define _GNU_SOURCE
#include<errno.h>
#include<stdio.h>
#include<fcntl.h>
#include<dirent.h>
#include<stdlib.h>
#include<unistd.h>
#include<libgen.h>
#include<semaphore.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<xf86drm.h>
#include<xf86drmMode.h>
#include"gui.h"
#include"confd.h"
#include"logger.h"
#include"system.h"
#include"defines.h"
#include"hardware.h"
#include"gui/guidrv.h"
#define TAG "drm"
#define DIV_ROUND_UP(n,d)(((n)+(d)-1)/(d))
struct drm_buffer{
	uint32_t handle,pitch,offset;
	unsigned long int size;
	void*map;
	uint32_t fb_handle;
};
static struct drm_dev{
	int fd,sfd,bnfd;
	bool blank;
	uint32_t
		conn_id,
		enc_id,
		crtc_id,
		width,height,
		mmWidth,mmHeight;
	drmModeModeInfo mode;
	uint32_t blob_id;
	drmModeCrtc*crtc;
	lv_color_t*cbuf;
	struct drm_buffer buf;
	struct display_mode*modes;
	int modes_cnt;
	sem_t flush;
	pthread_t tid;
	lv_disp_draw_buf_t dbuf;
	lv_disp_drv_t drv;
}drm_dev;
static void drm_exit(void){
	if(drm_dev.fd<0)return;
	if(drm_dev.cbuf)free(drm_dev.cbuf);
	drm_dev.cbuf=NULL;
	close(drm_dev.fd);
	drm_dev.fd=-1;
	sem_post(&drm_dev.flush);
	sem_destroy(&drm_dev.flush);
}
static const char*conn_to_str(drmModeConnection conn){
	switch(conn){
		case DRM_MODE_CONNECTED:return "connected";
		case DRM_MODE_DISCONNECTED:return "disconnected";
		case DRM_MODE_UNKNOWNCONNECTION:return "unknown connection";
		default:return "unknown";
	}
}
static const char*conn_type_to_str(uint32_t type){
	switch(type){
		#ifdef DRM_MODE_CONNECTOR_VGA
		case DRM_MODE_CONNECTOR_VGA:return "VGA";
		#endif
		#ifdef DRM_MODE_CONNECTOR_DVII
		case DRM_MODE_CONNECTOR_DVII:return "DVI-I";
		#endif
		#ifdef DRM_MODE_CONNECTOR_DVID
		case DRM_MODE_CONNECTOR_DVID:return "DVI-D";
		#endif
		#ifdef DRM_MODE_CONNECTOR_DVIA
		case DRM_MODE_CONNECTOR_DVIA:return "DVI-A";
		#endif
		#ifdef DRM_MODE_CONNECTOR_Composite
		case DRM_MODE_CONNECTOR_Composite:return "Composite";
		#endif
		#ifdef DRM_MODE_CONNECTOR_SVIDEO
		case DRM_MODE_CONNECTOR_SVIDEO:return "S-VIDEO";
		#endif
		#ifdef DRM_MODE_CONNECTOR_LVDS
		case DRM_MODE_CONNECTOR_LVDS:return "LVDS";
		#endif
		#ifdef DRM_MODE_CONNECTOR_Component
		case DRM_MODE_CONNECTOR_Component:return "Component";
		#endif
		#ifdef DRM_MODE_CONNECTOR_9PinDIN
		case DRM_MODE_CONNECTOR_9PinDIN:return "DIN";
		#endif
		#ifdef DRM_MODE_CONNECTOR_DisplayPort
		case DRM_MODE_CONNECTOR_DisplayPort:return "DP";
		#endif
		#ifdef DRM_MODE_CONNECTOR_HDMIA
		case DRM_MODE_CONNECTOR_HDMIA:return "HDMI";
		#endif
		#ifdef DRM_MODE_CONNECTOR_HDMIB
		case DRM_MODE_CONNECTOR_HDMIB:return "HDMI-B";
		#endif
		#ifdef DRM_MODE_CONNECTOR_TV
		case DRM_MODE_CONNECTOR_TV:return "TV";
		#endif
		#ifdef DRM_MODE_CONNECTOR_eDP
		case DRM_MODE_CONNECTOR_eDP:return "eDP";
		#endif
		#ifdef DRM_MODE_CONNECTOR_VIRTUAL
		case DRM_MODE_CONNECTOR_VIRTUAL:return "Virtual";
		#endif
		#ifdef DRM_MODE_CONNECTOR_DSI
		case DRM_MODE_CONNECTOR_DSI:return "DSI";
		#endif
		#ifdef DRM_MODE_CONNECTOR_DPI
		case DRM_MODE_CONNECTOR_DPI:return "DPI";
		#endif
		#ifdef DRM_MODE_CONNECTOR_WRITEBACK
		case DRM_MODE_CONNECTOR_WRITEBACK:return "WriteBack";
		#endif
		#ifdef DRM_MODE_CONNECTOR_SPI
		case DRM_MODE_CONNECTOR_SPI:return "SPI";
		#endif
		#ifdef DRM_MODE_CONNECTOR_USB
		case DRM_MODE_CONNECTOR_USB:return "USB";
		#endif
		default:return "unknown";
	}
}
static int drm_setup_mode(drmModeConnector*conn){
	int i;
	bool found_mode=false;
	drm_dev.modes_cnt=conn->count_modes;
	size_t size=sizeof(struct display_mode)*(drm_dev.modes_cnt+1);
	if(!(drm_dev.modes=malloc(size)))
		return terlog_error(-1,"allocate for modes failed");
	memset(drm_dev.modes,0,size);
	char*name;
	if(!(name=confd_get_string("gui.mode",NULL))){
		char*n=getenv("GUIMODE");
		if(n)name=strdup(n);
	}
	for(i=0;i<conn->count_modes;i++){
		if(conn->modes[i].name[0])strncpy(
			drm_dev.modes[i].name,
			conn->modes[i].name,
			sizeof(drm_dev.modes[i].name)-1
		);
		else snprintf(
			drm_dev.modes[i].name,
			sizeof(drm_dev.modes[i].name)-1,
			"%dx%d@%dHz",
			conn->modes[i].hdisplay,
			conn->modes[i].vdisplay,
			conn->modes[i].vrefresh
		);
		drm_dev.modes[i].width=conn->modes[i].hdisplay;
		drm_dev.modes[i].height=conn->modes[i].vdisplay;
		if(!found_mode){
			if(name&&strcasecmp(
				name,drm_dev.modes[i].name
			)!=0)continue;
			memcpy(
				&drm_dev.mode,
				&conn->modes[i],
				sizeof(drmModeModeInfo)
			);
			tlog_info("set mode to %s",drm_dev.modes[i].name);
			found_mode=true;
		}
	}
	if(!found_mode){
		tlog_warn("mode %s not found",name);
		memcpy(
			&drm_dev.mode,
			&conn->modes[0],
			sizeof(drmModeModeInfo)
		);
	}
	free(name);
	return 0;
}
static int drm_find_crtc(drmModeRes*res,drmModeConnector*conn){
	drmModeEncoder*enc=NULL;
	int i,j;
	int32_t crtc;
	if(conn->encoder_id)enc=drmModeGetEncoder(
		drm_dev.fd,conn->encoder_id
	);
	if(enc){
		crtc=enc->crtc_id;
		drmModeFreeEncoder(enc);
		if(crtc>0){
			drm_dev.crtc_id=crtc;
			return 0;
		}
	}
	for(i=0;i<conn->count_encoders;i++) {
		if(!(enc=drmModeGetEncoder(
			drm_dev.fd,
			conn->encoders[i]
		)))continue;
		for(j=0;j<res->count_crtcs;j++)if(
			(enc->possible_crtcs&(1<<j))&&
			res->crtcs[j]>0
		){
			drmModeFreeEncoder(enc);
			drm_dev.crtc_id=res->crtcs[j];
			return 0;
		}
		drmModeFreeEncoder(enc);
	}
	return -ENOENT;
}
static int drm_find_connector(void){
	drmModeConnector*conn=NULL;
	drmModeRes*res;
	int i;
	if((res=drmModeGetResources(drm_dev.fd))==NULL)
		return trlog_error(-1,"drmModeGetResources failed");
	if(res->count_crtcs<=0){
		tlog_error("no crtcs");
		goto free_res;
	}
	uint32_t use_conn=0;
	uint32_t sel_conn=confd_get_integer("gui.connector",0);
	for(i=0;i<res->count_connectors;i++){
		bool use=false;
		if(!(conn=drmModeGetConnector(drm_dev.fd,res->connectors[i])))continue;
		tlog_info(
			"connector %d (%s): %s",
			conn->connector_id,
			conn_type_to_str(conn->connector_type),
			conn_to_str(conn->connection)
		);
		if(conn->connection==DRM_MODE_CONNECTED&&conn->count_modes>0){
			if(sel_conn>0&&sel_conn==res->connectors[i])use=true;
			else if(use_conn==0)use=true;
		}
		if(use)use_conn=res->connectors[i];
		drmModeFreeConnector(conn);
		conn=NULL;
	}
	if(use_conn==0){
		tlog_error("suitable connector not found");
		goto free_res;
	}
	if(!(conn=drmModeGetConnector(drm_dev.fd,use_conn))){
		tlog_error("get connector %d failed",use_conn);
		goto free_res;
	}
	if(conn->connection!=DRM_MODE_CONNECTED){
		tlog_error("connector %d not connected",use_conn);
		goto free_res;
	}
	if(conn->count_modes<=0){
		tlog_error("connector %d has no modes",use_conn);
		goto free_res;
	}
	tlog_debug("use connector %d",use_conn);
	drm_dev.conn_id=conn->connector_id;
	if(drm_setup_mode(conn))goto free_res;
	drm_dev.mmWidth=conn->mmWidth;
	drm_dev.mmHeight=conn->mmHeight;
	if(drmModeCreatePropertyBlob(
		drm_dev.fd,
		&drm_dev.mode,
		sizeof(drm_dev.mode),
		&drm_dev.blob_id
	)){
		telog_error("error creating mode blob");
		goto free_res;
	}
	drm_dev.width=drm_dev.mode.hdisplay;
	drm_dev.height=drm_dev.mode.vdisplay;
	if(drm_find_crtc(res,conn)){
		telog_error("no crtc found");
		goto free_res;
	}
	return 0;
	free_res:
	drmModeFreeResources(res);
	free(drm_dev.modes);
	drm_dev.modes=NULL;
	drm_dev.modes_cnt=0;
	return -1;
}
static int drm_find_backlight(int sfd){
	char buff[64];
	int conn,status,ret;
	struct dirent*e;
	DIR*d=fdopendir(sfd);
	if(d){
		while((e=readdir(d))){
			if(
				e->d_type!=DT_DIR||
				strncmp("card",e->d_name,4)!=0
			)continue;
			if((conn=openat(sfd,e->d_name,O_DIR))<0){
				telog_warn("open connector %s folder failed",e->d_name);
				continue;
			}
			if((status=openat(conn,"status",O_RDONLY))<0){
				telog_warn("read connector %s status failed",e->d_name);
				close(conn);
				continue;
			}
			memset(buff,0,64);
			if(read(status,buff,63)<=0){
				telog_warn("read connector %s status contents failed",e->d_name);
				close(status);
				close(conn);
				continue;
			}
			close(status);
			if(strncmp("connected",buff,9)!=0){
				close(conn);
				continue;
			}
			if((ret=led_find_class(conn,NULL))<0){
				close(conn);
				continue;
			}
			close(conn);
			return ret;
		}
		free(d);
	}
	return backlight_find(NULL);
}
static int drm_show(){
	return drmModeSetCrtc(
		drm_dev.fd,
		drm_dev.crtc_id,
		drm_dev.buf.fb_handle,0,0,
		&drm_dev.conn_id,1,
		&drm_dev.mode
	);
}
static int drm_get_brightness(){
	if(drm_dev.bnfd<0)ERET(ENOTSUP);
	return led_get_brightness_percent(drm_dev.bnfd);
}
static void drm_set_brightness(int value){
	if(drm_dev.bnfd<0){
		errno=ENOTSUP;
		return;
	}
	led_set_brightness_percent(drm_dev.bnfd,value);
	telog_debug("set backlight to %d%%",value);
	if(drm_dev.blank){
		errno=0;
		drm_dev.blank=false;
		drm_show();
		sem_post(&drm_dev.flush);
		telog_debug("screen resume");
	}else if(value<=0){
		errno=0;
		drm_dev.blank=true;
		drmModeSetCrtc(drm_dev.fd,drm_dev.crtc_id,0,0,0,NULL,0,NULL);
		telog_debug("screen suspend");
	}
}
static int drm_open(int fd,int sfd){
	uint64_t has_dumb;
	if(drmGetCap(fd,DRM_CAP_DUMB_BUFFER,&has_dumb)<0||has_dumb==0)
		return trlog_error(-1,"drmGetCap DRM_CAP_DUMB_BUFFER failed or no dumb buffer");
	drm_dev.fd=fd,drm_dev.sfd=sfd;
	if(default_backlight>=0)drm_dev.bnfd=default_backlight;
	else if((drm_dev.bnfd=drm_find_backlight(sfd))<0)return trlog_warn(fd,"no backlight device found");
	int b=drm_get_brightness();
	if(b>=0&&b<=100)tlog_info("current backlight: %d%%\n",b);
	return fd;
}
static int drm_setup(int fd,int sfd){
	if(drm_open(fd,sfd)<0)return -1;
	if(drm_find_connector())
		return trlog_error(-1,"available drm devices not found");
	if(!(drm_dev.crtc=drmModeGetCrtc(fd,drm_dev.crtc_id)))
		return trlog_error(-1,"can not get crtc");
	tlog_info(
		"found connector %d, crtc %d",
		drm_dev.conn_id,
		drm_dev.crtc_id
	);
	tlog_info(
		"size %dx%d(%dmmX%dmm)",
		drm_dev.width,drm_dev.height,
		drm_dev.mmWidth,drm_dev.mmHeight
	);
	return 0;
}
static int drm_allocate_dumb(struct drm_buffer*b){
	struct drm_mode_create_dumb creq;
	struct drm_mode_map_dumb mreq;
	memset(&creq,0,sizeof(creq));
	creq.width=drm_dev.width;
	creq.height=drm_dev.height;
	creq.bpp=LV_COLOR_DEPTH;
	if(drmIoctl(
		drm_dev.fd,
		DRM_IOCTL_MODE_CREATE_DUMB,
		&creq
	)<0)return trlog_error(-1,"DRM_IOCTL_MODE_CREATE_DUMB fail");
	b->handle=creq.handle;
	b->pitch=creq.pitch;
	b->size=creq.size;
	if(drmModeAddFB(
		drm_dev.fd,
		drm_dev.width,
		drm_dev.height,
		24,32,
		b->pitch,
		b->handle,
		&b->fb_handle
	))return terlog_error(-1,"drmModeAddFB fail");
	memset(&mreq,0,sizeof(mreq));
	mreq.handle=creq.handle;
	if(drmIoctl(
		drm_dev.fd,
		DRM_IOCTL_MODE_MAP_DUMB,
		&mreq
	))return terlog_error(-1,"DRM_IOCTL_MODE_MAP_DUMB fail");
	b->offset=mreq.offset;
	if((b->map=mmap(
		0,
		creq.size,
		PROT_READ|PROT_WRITE,
		MAP_SHARED,
		drm_dev.fd,
		mreq.offset
	))==MAP_FAILED)return terlog_error(-1,"mmap fail");
	memset(b->map,0,creq.size);
	return 0;
}
static void*flush_thread(void*data __attribute__((unused))){
	drm_show();
	while(drm_dev.fd>=0){
		drmModePageFlip(
			drm_dev.fd,
			drm_dev.crtc_id,
			drm_dev.buf.fb_handle,
			0,NULL
		);
		sem_wait(&drm_dev.flush);
	}
	return data;
}
static void drm_flush(lv_disp_drv_t*disp_drv,const lv_area_t*area,lv_color_t*color_p){
	int i,y;
	lv_coord_t w=(area->x2-area->x1+1);
	for(y=0,i=area->y1;i<=area->y2;++i,++y)memcpy(
		drm_dev.buf.map+(area->x1*4)+(drm_dev.buf.pitch*i),
		(void*)color_p+(w*4*y),w*4
	);
	sem_post(&drm_dev.flush);
	lv_disp_flush_ready(disp_drv);
}
static void drm_get_sizes(lv_coord_t*width,lv_coord_t*height){
	lv_coord_t w=0,h=0;
	switch(gui_rotate){
		case 0:case 180:w=drm_dev.width,h=drm_dev.height;break;
		case 90:case 270:w=drm_dev.height,h=drm_dev.width;break;
	}
	if(width)*width=w;
	if(height)*height=h;
}
static int drm_get_modes(int*cnt,struct display_mode**modes){
	if(!cnt||!modes)ERET(EINVAL);
	*cnt=0,*modes=NULL;
	if(!drm_dev.modes)ERET(ENODEV);
	size_t size=sizeof(struct display_mode)*(drm_dev.modes_cnt+1);
	if(!(*modes=memdup(drm_dev.modes,size)))ERET(ENOMEM);
	*cnt=drm_dev.modes_cnt;
	return 0;
}
static int _drm_register(){
	if(drm_dev.width<=0||drm_dev.height<=0){
		drm_exit();
		return -1;
	}
	size_t s=drm_dev.width*drm_dev.height*sizeof(lv_color_t);
	if(!(drm_dev.cbuf=malloc(s))){
		telog_error("malloc display buffer");
		drm_exit();
		return -1;
	}
	memset(drm_dev.cbuf,0,s);
	lv_disp_draw_buf_init(&drm_dev.dbuf,drm_dev.cbuf,NULL,s);
	lv_disp_drv_init(&drm_dev.drv);
	drm_dev.drv.hor_res=drm_dev.width;
	drm_dev.drv.ver_res=drm_dev.height;
	tlog_notice(
		"screen resolution: %dx%d",
		drm_dev.width,
		drm_dev.height
	);
	drm_dev.drv.draw_buf=&drm_dev.dbuf;
	drm_dev.drv.flush_cb=drm_flush;
	drm_dev.drv.draw_ctx_init=lv_draw_sw_init_ctx;
	drm_dev.drv.draw_ctx_deinit=lv_draw_sw_init_ctx;
	drm_dev.drv.draw_ctx_size=sizeof(lv_draw_sw_ctx_t);
	switch(gui_rotate){
		case 0:break;
		case 90:drm_dev.drv.sw_rotate=1,drm_dev.drv.rotated=LV_DISP_ROT_90;break;
		case 180:drm_dev.drv.sw_rotate=1,drm_dev.drv.rotated=LV_DISP_ROT_180;break;
		case 270:drm_dev.drv.sw_rotate=1,drm_dev.drv.rotated=LV_DISP_ROT_270;break;
	}
	lv_disp_drv_register(&drm_dev.drv);
	pthread_create(&drm_dev.tid,NULL,flush_thread,NULL);
	sem_init(&drm_dev.flush,0,0);
	set_active_console(7);
	return 0;
}

static int _drm_init_fd(int fd,int sfd){
	if(drm_setup(fd,sfd)){
		drm_dev.fd=-1;
		return -1;
	}
	if(drm_allocate_dumb(&drm_dev.buf)){
		tlog_error("buffer allocation failed");
		drm_dev.fd=-1;
		return -1;
	}
	tlog_debug("initialized");
	return 0;
}

static char*_drm_get_driver_name(int fd,char*buff,size_t len){
	struct stat s;
	char b[128]={0},*ret=NULL;
	memset(&s,0,sizeof(s));
	if(
		fstatat(fd,"device/driver",&s,0)<0||
		!S_ISLNK(s.st_mode)
	)return NULL;
	if(readlinkat(fd,"device/driver",b,127)>0)ret=basename(b);
	if(ret)strncpy(buff,ret,len-1);
	return ret;
}

static int drm_scan_init(){
	int sfd,dfd;
	char*dfmt,*sfmt,*driver;
	char drbuff[128]={0},sdev[256]={0},ddev[256]={0};
	bool x=access(_PATH_DEV"/dri",F_OK)==0;
	if(!x&&errno!=ENOENT)
		return terlog_error(-1,"access "_PATH_DEV"/dri");
	dfmt=_PATH_DEV"/dri/card%d";
	sfmt=_PATH_SYS_CLASS"/drm/card%d";
	for(int i=0;i<32;i++){
		memset(sdev,0,256);
		memset(ddev,0,256);
		snprintf(sdev,255,sfmt,i);
		snprintf(ddev,255,dfmt,i);
		if((sfd=open(sdev,O_DIR|O_CLOEXEC))<0){
			if(errno!=ENOENT)telog_warn("open sysfs %s",sdev);
			continue;
		}
		if((dfd=open(ddev,O_RDWR|O_CLOEXEC))<0){
			if(errno!=ENOENT)telog_warn("open device %s",ddev);
			close(sfd);
			continue;
		}
		tlog_debug("found drm card device %s",ddev);
		memset(drbuff,0,127);
		if((driver=_drm_get_driver_name(sfd,drbuff,127))){
			tlog_info("scan drm card device %d use driver %s",i,driver);
			if(!strcmp(driver,"vkms")){
				tlog_debug("device card%d seems to be Virtual KMS, skip",i);
				close(sfd);
				close(dfd);
				continue;
			}
		}
		if(_drm_init_fd(dfd,sfd)<0){
			tlog_error("card%d init failed, skip",i);
			close(sfd);
			close(dfd);
			continue;
		}
		return 0;
	}
	tlog_error("no drm found");
	return -1;
}

static int drm_scan_init_register(){
	if(drm_scan_init()<0)return -1;
	if(_drm_register()<0)return -1;
	return 0;
}

static void drm_get_dpi(int*dpi){
	if(drm_dev.mmWidth<=0||drm_dev.mmHeight<=0)return;
	if(dpi)*dpi=DIV_ROUND_UP(drm_dev.width*25400,drm_dev.mmWidth*1000);
}

struct gui_driver guidrv_drm={
	.name="drm",
	.drv_register=drm_scan_init_register,
	.drv_getsize=drm_get_sizes,
	.drv_getdpi=drm_get_dpi,
	.drv_get_modes=drm_get_modes,
	.drv_exit=drm_exit,
	.drv_getbrightness=drm_get_brightness,
	.drv_setbrightness=drm_set_brightness
};
#endif
#endif
