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
#include<sys/mman.h>
#include<sys/stat.h>
#include<xf86drm.h>
#include<xf86drmMode.h>
#include<drm_fourcc.h>
#include"defines.h"
#include"hardware.h"
#include"lvgl.h"
#include"gui.h"
#include"logger.h"
#define TAG "drm"
#define DIV_ROUND_UP(n,d)(((n)+(d)-1)/(d))
static lv_color_t*buf1=NULL,*buf2=NULL;
struct drm_buffer{
	uint32_t handle,pitch,offset;
	unsigned long int size;
	void*map;
	uint32_t fb_handle;
};
static struct drm_dev{
	int fd,sfd,bnfd;
	uint32_t
		conn_id,
		enc_id,
		crtc_id,
		plane_id,
		crtc_idx,
		width,height,
		mmWidth,mmHeight,
		fourcc;
	drmModeModeInfo mode;
	uint32_t blob_id;
	drmModeCrtc*saved_crtc;
	drmModeAtomicReq*req;
	drmModePlane*plane;
	drmModeCrtc*crtc;
	drmModeConnector*conn;
	uint32_t
		count_plane_props,
		count_crtc_props,
		count_conn_props;
	drmModePropertyPtr
		plane_props[128],
		crtc_props[128],
		conn_props[128];
	struct drm_buffer drm_bufs[2];
	struct drm_buffer*cur_bufs[2];
}drm_dev;
static void drm_exit(void){
	if(drm_dev.fd<0)return;
	if(buf1)free(buf1);
	if(buf2)free(buf2);
	buf1=NULL;
	buf2=NULL;
	close(drm_dev.fd);
	drm_dev.fd=-1;
}
static uint32_t get_plane_property_id(const char*name){
	uint32_t i;
	for(i=0;i<drm_dev.count_plane_props;++i)
		if(!strcmp(drm_dev.plane_props[i]->name,name))
			return drm_dev.plane_props[i]->prop_id;
	return 0;
}
static uint32_t get_crtc_property_id(const char*name){
	uint32_t i;
	for(i=0;i<drm_dev.count_crtc_props;++i)
		if(!strcmp(drm_dev.crtc_props[i]->name,name))
			return drm_dev.crtc_props[i]->prop_id;
	return 0;
}
static uint32_t get_conn_property_id(const char*name){
	uint32_t i;
	for(i=0;i<drm_dev.count_conn_props;++i)
		if(!strcmp(drm_dev.conn_props[i]->name,name))
			return drm_dev.conn_props[i]->prop_id;
	return 0;
}
static int drm_get_plane_props(void){
	uint32_t i;
	drmModeObjectPropertiesPtr props=drmModeObjectGetProperties(
		drm_dev.fd,
		drm_dev.plane_id,
		DRM_MODE_OBJECT_PLANE
	);
	if(!props)return trlog_error(-1,"drmModeObjectGetProperties failed");
	drm_dev.count_plane_props=props->count_props;
	for(i=0;i<props->count_props;i++)
		drm_dev.plane_props[i]=drmModeGetProperty(
			drm_dev.fd,
			props->props[i]
		);
	drmModeFreeObjectProperties(props);
	return 0;
}
static int drm_get_crtc_props(void){
	uint32_t i;
	drmModeObjectPropertiesPtr props=drmModeObjectGetProperties(
		drm_dev.fd,
		drm_dev.crtc_id,
		DRM_MODE_OBJECT_CRTC
	);
	if(!props)return trlog_error(-1,"drmModeObjectGetProperties failed");
	drm_dev.count_crtc_props=props->count_props;
	for(i=0;i<props->count_props;i++)
		drm_dev.crtc_props[i]=drmModeGetProperty(
			drm_dev.fd,
			props->props[i]
		);
	drmModeFreeObjectProperties(props);
	return 0;
}
static int drm_get_conn_props(void){
	uint32_t i;
	drmModeObjectPropertiesPtr props=drmModeObjectGetProperties(
		drm_dev.fd,
		drm_dev.conn_id,
		DRM_MODE_OBJECT_CONNECTOR
	);
	if(!props)return trlog_error(-1,"drmModeObjectGetProperties failed");
	drm_dev.count_conn_props=props->count_props;
	for(i=0;i<props->count_props;i++)
		drm_dev.conn_props[i]=drmModeGetProperty(
			drm_dev.fd,
			props->props[i]
		);
	drmModeFreeObjectProperties(props);
	return 0;
}

static int drm_add_plane_property(const char*name,uint64_t value){
	int ret;
	uint32_t prop_id=get_plane_property_id(name);
	if(!prop_id)return trlog_error(-1,"could not find plane prop %s",name);
	if((ret=drmModeAtomicAddProperty(
		drm_dev.req,
		drm_dev.plane_id,
		get_plane_property_id(name),
		value
	))<0)return trlog_error(
		ret,"drmModeAtomicAddProperty(%s:%lu) failed: %d",
		name,value,ret
	);
	return 0;
}
static int drm_add_crtc_property(const char*name,uint64_t value){
	int ret;
	uint32_t prop_id=get_crtc_property_id(name);
	if(!prop_id)return trlog_error(-1,"could not find crtc prop %s",name);
	if((ret=drmModeAtomicAddProperty(
		drm_dev.req,
		drm_dev.crtc_id,
		get_crtc_property_id(name),
		value
	))<0)return trlog_error(
		ret,"drmModeAtomicAddProperty(%s:%lu) failed: %d",
		name,value,ret
	);
	return 0;
}
static int drm_add_conn_property(const char*name,uint64_t value){
	int ret;
	uint32_t prop_id=get_conn_property_id(name);
	if(!prop_id)return trlog_error(-1,"could not find conn prop %s",name);
	if((ret=drmModeAtomicAddProperty(
		drm_dev.req,
		drm_dev.conn_id,
		get_conn_property_id(name),
		value
	))<0)return trlog_error(
		ret,"drmModeAtomicAddProperty(%s:%lu) failed: %d",
		name,value,ret
	);
	return 0;
}
static int drm_dmabuf_set_plane(struct drm_buffer*buf){
	int ret;
	static int first=1;
	uint32_t flags=DRM_MODE_ATOMIC_ALLOW_MODESET;
	drm_dev.req=drmModeAtomicAlloc();
	if(first){
		drm_add_conn_property("CRTC_ID",drm_dev.crtc_id);
		drm_add_crtc_property("MODE_ID",drm_dev.blob_id);
		drm_add_crtc_property("ACTIVE",1);
		flags|=DRM_MODE_ATOMIC_ALLOW_MODESET;
	}
	drm_add_plane_property("FB_ID",buf->fb_handle);
	drm_add_plane_property("CRTC_ID",drm_dev.crtc_id);
	drm_add_plane_property("SRC_X",0);
	drm_add_plane_property("SRC_Y",0);
	drm_add_plane_property("SRC_W",drm_dev.width<<16);
	drm_add_plane_property("SRC_H",drm_dev.height<<16);
	drm_add_plane_property("CRTC_X",0);
	drm_add_plane_property("CRTC_Y",0);
	drm_add_plane_property("CRTC_W",drm_dev.width);
	drm_add_plane_property("CRTC_H",drm_dev.height);
	if((ret=drmModeAtomicCommit(drm_dev.fd,drm_dev.req,flags,NULL))){
		telog_error("drmModeAtomicCommit failed");
		drmModeAtomicFree(drm_dev.req);
		return ret;
	}
	return 0;
}
static int find_plane(unsigned int fourcc,uint32_t*plane_id,uint32_t crtc_idx){
	drmModePlaneResPtr planes;
	drmModePlanePtr plane;
	unsigned int i,j;
	int ret=0;
	unsigned int format=fourcc;
	if(!(planes=drmModeGetPlaneResources(drm_dev.fd)))
		return trlog_error(-1,"drmModeGetPlaneResources failed");
	for(i=0;i<planes->count_planes;++i){
		if(!(plane=drmModeGetPlane(drm_dev.fd,planes->planes[i]))){
			telog_error("drmModeGetPlane failed");
			break;
		}
		if(!(plane->possible_crtcs&(1<<crtc_idx))){
			drmModeFreePlane(plane);
			continue;
		}
		for(j=0;j<plane->count_formats;++j)
			if(plane->formats[j]==format)break;
		if(j==plane->count_formats){
			drmModeFreePlane(plane);
			continue;
		}
		*plane_id=plane->plane_id;
		drmModeFreePlane(plane);
		break;
	}
	if(i==planes->count_planes)ret=-1;
	drmModeFreePlaneResources(planes);
	return ret;
}
static int drm_find_connector(void){
	drmModeConnector*conn=NULL;
	drmModeEncoder*enc=NULL;
	drmModeRes*res;
	int i;
	if((res=drmModeGetResources(drm_dev.fd))==NULL)
		return trlog_error(-1,"drmModeGetResources failed");
	if(res->count_crtcs<=0){
		tlog_error("no crtcs");
		goto free_res;
	}
	for(i=0;i<res->count_connectors;i++){
		if(!(conn=drmModeGetConnector(drm_dev.fd,res->connectors[i])))continue;
		char*name;
		switch(conn->connection){
			case DRM_MODE_CONNECTED:name="connected";break;
			case DRM_MODE_DISCONNECTED:name="disconnected";break;
			case DRM_MODE_UNKNOWNCONNECTION:name="unknown connection";break;
			default:name="unknown";
		}
		tlog_info("connector %d: %s",conn->connector_id,name);
		if(conn->connection==DRM_MODE_CONNECTED&&conn->count_modes>0)break;
		drmModeFreeConnector(conn);
		conn=NULL;
	};
	if(!conn){
		tlog_error("suitable connector not found");
		goto free_res;
	}
	drm_dev.conn_id=conn->connector_id;
	drm_dev.mmWidth=conn->mmWidth;
	drm_dev.mmHeight=conn->mmHeight;
	memcpy(&drm_dev.mode,&conn->modes[0],sizeof(drmModeModeInfo));
	if(drmModeCreatePropertyBlob(
		drm_dev.fd,
		&drm_dev.mode,
		sizeof(drm_dev.mode),
		&drm_dev.blob_id
	)){
		tlog_error("error creating mode blob");
		goto free_res;
	}
	drm_dev.width=conn->modes[0].hdisplay;
	drm_dev.height=conn->modes[0].vdisplay;
	for(i=0;i<res->count_encoders;i++){
		if(!(enc=drmModeGetEncoder(
			drm_dev.fd,
			res->encoders[i]
		)))continue;
		if(enc->encoder_id==conn->encoder_id)break;
		drmModeFreeEncoder(enc);
		enc=NULL;
	}
	if(enc){
		drm_dev.enc_id=enc->encoder_id;
		drm_dev.crtc_id=enc->crtc_id;
		drmModeFreeEncoder(enc);
	}else{
		for(i=0;i<conn->count_encoders;i++){
			int crtc,crtc_id=-1;
			if(!(enc=drmModeGetEncoder(
				drm_dev.fd,
				conn->encoders[i]
			)))continue;
			for(crtc=0;crtc<res->count_crtcs;crtc++){
				uint32_t crtc_mask=1<<crtc;
				crtc_id=res->crtcs[crtc];
				if(enc->possible_crtcs&crtc_mask)break;
			}
			if(crtc_id>0){
				drm_dev.enc_id=enc->encoder_id;
				drm_dev.crtc_id=crtc_id;
				break;
			}
			drmModeFreeEncoder(enc);
			enc=NULL;
		}
		if(!enc){
			tlog_error("suitable encoder not found");
			goto free_res;
		}
		drmModeFreeEncoder(enc);
	}
	bool found=false;
	for(i=0;i<res->count_crtcs;++i)
		if(drm_dev.crtc_id==res->crtcs[i]){
			drm_dev.crtc_idx=i;
			found=true;
			break;
		}
	if(!found){
		tlog_error("crtc not found");
		goto free_res;
	}
	return 0;

free_res:
	drmModeFreeResources(res);
	return -1;
}
static int drm_find_connector_sysfs(int sfd){
	char buff[64];
	int conn,status;
	struct dirent*e;
	DIR*d=fdopendir(sfd);
	if(d)while((e=readdir(d))){
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
			tlog_debug("found connected connector %s\n",e->d_name);
			return conn;
		}
	free(d);
	ERET(ENOENT);
}
static int drm_find_connector_backlight(int conn){
	int bg;
	struct stat st;
	struct dirent*e;
	DIR*d=fdopendir(conn);
	if(d)while((e=readdir(d))){
		if(e->d_type!=DT_DIR)continue;
		if((bg=openat(conn,e->d_name,O_DIR))<0){
			telog_warn("open connector backlight %s folder failed",e->d_name);
			continue;
		}
		if(fstatat(bg,"brightness",&st,AT_SYMLINK_NOFOLLOW)<0){
			close(bg);
			continue;
		}
		tlog_debug("found connector backight %s\n",e->d_name);
		return bg;
	}
	free(d);
	ERET(ENOENT);
}
static int drm_find_backlight(int sfd){
	int conn=drm_find_connector_sysfs(sfd);
	if(conn<0)return conn;
	int bn=drm_find_connector_backlight(conn);
	close(conn);
	return bn;
}
static int drm_get_brightness(){
	if(drm_dev.bnfd<0)ERET(ENOTSUP);
	return led_get_brightness_percent(drm_dev.bnfd);
}
static void drm_set_brightness(int value){
	if(drm_dev.bnfd<0)errno=ENOTSUP;
	else if(led_set_brightness_percent(drm_dev.bnfd,value)>=0)
		tlog_debug("set backlight to %d%%",value);
}
static int drm_open(int fd,int sfd){
	int flags;
	uint64_t has_dumb;
	if((flags=fcntl(fd,F_GETFD))<0||fcntl(fd,F_SETFD,flags|FD_CLOEXEC)<0)
		return terlog_error(-1,"fcntl FD_CLOEXEC failed");
	if((flags=fcntl(sfd,F_GETFD))<0||fcntl(sfd,F_SETFD,flags|FD_CLOEXEC)<0)
		return terlog_error(-1,"fcntl FD_CLOEXEC failed");
	if(drmGetCap(fd,DRM_CAP_DUMB_BUFFER,&has_dumb)<0||has_dumb==0)
		return trlog_error(-1,"drmGetCap DRM_CAP_DUMB_BUFFER failed or no dumb buffer");
	drm_dev.fd=fd,drm_dev.sfd=sfd;
	if((drm_dev.bnfd=drm_find_backlight(sfd))<0)tlog_warn("no backlight device found");
	else{
		int b=drm_get_brightness();
		if(b>=0&&b<=100)tlog_info("current backlight: %d%%\n",b);
	}
	return fd;
}
static int drm_setup(int fd,int sfd,unsigned int fourcc){
	if(drm_open(fd,sfd)<0)return -1;
	if(drmSetClientCap(fd,DRM_CLIENT_CAP_ATOMIC,1))
		return terlog_error(-1,"no atomic modesetting support");
	if(drm_find_connector())
		return trlog_error(-1,"available drm devices not found");
	if(find_plane(
		fourcc,
		&drm_dev.plane_id,
		drm_dev.crtc_idx
	))return trlog_error(-1,"can not find plane");
	if(!(drm_dev.plane=drmModeGetPlane(fd,drm_dev.plane_id)))
		return trlog_error(-1,"can not get plane");
	if(!(drm_dev.crtc=drmModeGetCrtc(fd,drm_dev.crtc_id)))
		return trlog_error(-1,"can not get crtc");
	if(!(drm_dev.conn=drmModeGetConnector(fd,drm_dev.conn_id)))
		return trlog_error(-1,"can not get connector");
	if(drm_get_plane_props())return trlog_error(-1,"can not get plane props");
	if(drm_get_crtc_props())return trlog_error(-1,"can not get crtc props");
	if(drm_get_conn_props())return trlog_error(-1,"can not get connector props");
	drm_dev.fourcc=fourcc;
	tlog_info(
		"found plane %u, connector %d, crtc %d",
		drm_dev.plane_id,
		drm_dev.conn_id,
		drm_dev.crtc_id
	);
	tlog_info(
		"%dx%d(%dmmX%dmm) pixel format %c%c%c%c",
		drm_dev.width,drm_dev.height,
		drm_dev.mmWidth,drm_dev.mmHeight,
		(fourcc>>0)&0xff,
		(fourcc>>8)&0xff,
		(fourcc>>16)&0xff,
		(fourcc>>24)&0xff
	);
	return 0;
}
static int drm_allocate_dumb(struct drm_buffer*buf){
	struct drm_mode_create_dumb creq;
	struct drm_mode_map_dumb mreq;
	uint32_t handles[4]={0},pitches[4]={0},offsets[4]={0};
	memset(&creq,0,sizeof(creq));
	creq.width=drm_dev.width;
	creq.height=drm_dev.height;
	creq.bpp=LV_COLOR_DEPTH;
	if(drmIoctl(
		drm_dev.fd,
		DRM_IOCTL_MODE_CREATE_DUMB,
		&creq
	)<0)return trlog_error(-1,"DRM_IOCTL_MODE_CREATE_DUMB fail");
	buf->handle=creq.handle;
	buf->pitch=creq.pitch;
	buf->size=creq.size;
	memset(&mreq,0,sizeof(mreq));
	mreq.handle=creq.handle;
	if(drmIoctl(
		drm_dev.fd,
		DRM_IOCTL_MODE_MAP_DUMB,
		&mreq
	))return trlog_error(-1,"DRM_IOCTL_MODE_MAP_DUMB fail");
	buf->offset=mreq.offset;
	if((buf->map=mmap(
		0,
		creq.size,
		PROT_READ|PROT_WRITE,
		MAP_SHARED,
		drm_dev.fd,
		mreq.offset
	))==MAP_FAILED)return trlog_error(-1,"mmap fail");
	memset(buf->map,0,creq.size);
	handles[0]=creq.handle;
	pitches[0]=creq.pitch;
	offsets[0]=0;
	if(drmModeAddFB2(
		drm_dev.fd,
		drm_dev.width,
		drm_dev.height,
		drm_dev.fourcc,
		handles,
		pitches,
		offsets,
		&buf->fb_handle,
		0
	))return trlog_error(-1,"drmModeAddFB fail");
	return 0;
}
static int drm_setup_buffers(void){
	int ret;
	if((ret=drm_allocate_dumb(&drm_dev.drm_bufs[0])))return ret;
	if((ret=drm_allocate_dumb(&drm_dev.drm_bufs[1])))return ret;
	drm_dev.cur_bufs[0]=NULL;
	drm_dev.cur_bufs[1]=&drm_dev.drm_bufs[0];
	return 0;
}
static void drm_flush(lv_disp_drv_t*disp_drv,const lv_area_t*area,lv_color_t*color_p){
	struct drm_buffer*fbuf=drm_dev.cur_bufs[1];
	lv_coord_t w=(area->x2-area->x1+1);
	lv_coord_t h=(area->y2-area->y1+1);
	int i,y;
	if(
		(
			(uint32_t)w!=drm_dev.width||
			(uint32_t)h!=drm_dev.height
		)&&drm_dev.cur_bufs[0]
	)memcpy(fbuf->map,drm_dev.cur_bufs[0]->map,fbuf->size);
	for(y=0,i=area->y1;i<=area->y2;++i,++y)memcpy(
		fbuf->map+(area->x1*(LV_COLOR_SIZE/8))+(fbuf->pitch*i),
		(void*)color_p+(w*(LV_COLOR_SIZE/8)*y),
		w*(LV_COLOR_SIZE/8)
	);
	if(drm_dmabuf_set_plane(fbuf)){
		tlog_warn("plane flush fail");
		return;
	}
	drm_dev.cur_bufs[1]=(!drm_dev.cur_bufs[0])?
		&drm_dev.drm_bufs[1]:drm_dev.cur_bufs[0];
	drm_dev.cur_bufs[0]=fbuf;
	lv_disp_flush_ready(disp_drv);
}
static void drm_get_sizes(uint32_t*width,uint32_t*height){
	if(width)*width=drm_dev.width;
	if(height)*height=drm_dev.height;
}
static int _drm_register(){
	if(drm_dev.width<=0||drm_dev.height<=0){
		drm_exit();
		return -1;
	}
	size_t s=drm_dev.width*drm_dev.height;
	static lv_disp_buf_t disp_buf;
	if(!(buf1=malloc(s*sizeof(lv_color_t)))){
		telog_error("malloc buf1");
		drm_exit();
		return -1;
	}
	if(!(buf2=malloc(s*sizeof(lv_color_t)))){
		telog_error("malloc buf2");
		drm_exit();
		return -1;
	}
	memset(buf1,0,s);
	memset(buf2,0,s);
	lv_disp_buf_init(&disp_buf,buf1,buf2,s);
	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.hor_res=drm_dev.width;
	disp_drv.ver_res=drm_dev.height;
	tlog_notice(
		"screen resolution: %dx%d",
		drm_dev.width,
		drm_dev.height
	);
	disp_drv.buffer=&disp_buf;
	disp_drv.flush_cb=drm_flush;
	lv_disp_drv_register(&disp_drv);
	return 0;
}

static int _drm_init_fd(int fd,int sfd,unsigned int fourcc){
	if(drm_setup(fd,sfd,fourcc)){
		drm_dev.fd=-1;
		return -1;
	}
	if(drm_setup_buffers()){
		tlog_error("buffer allocation failed");
		drm_dev.fd=-1;
		return -1;
	}
	tlog_debug("initialized");
	return 0;
}

static char*_drm_get_driver_name(int fd,char*buff,size_t len){
	struct stat s;
	char buf[128]={0},*ret=NULL;
	memset(&s,0,sizeof(s));
	if(
		fstatat(fd,"device/driver",&s,0)<0||
		!S_ISLNK(s.st_mode)
	)return NULL;
	if(readlinkat(fd,"device/driver",buf,127)>0)ret=basename(buf);
	if(ret)strncpy(buff,ret,len-1);
	return ret;
}

static int drm_scan_init(unsigned int fourcc){
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
		if((sfd=open(sdev,O_DIRECTORY|O_RDONLY))<0){
			if(errno!=ENOENT)telog_warn("open sysfs %s",sdev);
			continue;
		}
		if((dfd=open(ddev,O_RDWR))<0){
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
		if(_drm_init_fd(dfd,sfd,fourcc)<0){
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
	if(drm_scan_init(DRM_FORMAT_ARGB8888)<0)return -1;
	if(_drm_register()<0)return -1;
	ts_scan_register();
	return 0;
}

static void drm_get_dpi(int*dpi){
	if(dpi)*dpi=DIV_ROUND_UP(drm_dev.width*25400,drm_dev.mmWidth*1000);
}

struct gui_driver guidrv_drm={
	.name="drm",
	.drv_register=drm_scan_init_register,
	.drv_getsize=drm_get_sizes,
	.drv_getdpi=drm_get_dpi,
	.drv_exit=drm_exit,
	.drv_getbrightness=drm_get_brightness,
	.drv_setbrightness=drm_set_brightness
};
#endif
#endif
