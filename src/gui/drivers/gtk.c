#ifdef ENABLE_GUI
#ifdef ENABLE_GTK
#include<stdlib.h>
#include<unistd.h>
#include<gtk/gtk.h>
#include<pthread.h>
#include<sys/time.h>
#include"gui.h"
#include"confd.h"
#include"logger.h"
#include"hardware.h"
#include"gui/guidrv.h"
#define TAG "gtk"

static uint32_t ww=540,hh=960;
static GtkWidget*window,*event_box,*output_image;
static GdkPixbuf*pixbuf;
static lv_coord_t mouse_x,mouse_y;
static lv_indev_state_t mouse_btn=LV_INDEV_STATE_REL,last_key_state;
static lv_key_t last_key;
static uint8_t*fb=NULL;

static void quit_handler(void){
	exit(0);
}
static void gtkdrv_taskhandler(){
	gtk_image_set_from_pixbuf(GTK_IMAGE(output_image),pixbuf);
}

static void*gtkdrv_handler(void*p __attribute__((unused))){
	gtk_main();
	return NULL;
}

static gboolean mouse_pressed(
	GtkWidget *widget __attribute__((unused)),
	GdkEventButton *event __attribute__((unused)),
	gpointer user_data __attribute__((unused))
){
	mouse_btn=LV_INDEV_STATE_PR;
	gui_quit_sleep();
	return FALSE;
}

static gboolean mouse_released(
	GtkWidget *widget __attribute__((unused)),
	GdkEventButton *event __attribute__((unused)),
	gpointer user_data __attribute__((unused))
){
	mouse_btn=LV_INDEV_STATE_REL;
	gui_quit_sleep();
	return FALSE;
}

static gboolean mouse_motion(
	GtkWidget *widget __attribute__((unused)),
	GdkEventMotion *event __attribute__((unused)),
	gpointer user_data __attribute__((unused))
){
	mouse_x=event->x,mouse_y=event->y;
	gui_quit_sleep();
	return FALSE;
}

static gboolean keyboard_press(
	GtkWidget *widget __attribute__((unused)),
	GdkEventKey *event __attribute__((unused)),
	gpointer user_data __attribute__((unused))
){
	uint32_t ascii_key=event->keyval;
	switch(event->keyval){
		case GDK_KEY_rightarrow:
		case GDK_KEY_Right:ascii_key=LV_KEY_RIGHT;break;
		case GDK_KEY_leftarrow:
		case GDK_KEY_Left:ascii_key=LV_KEY_LEFT;break;
		case GDK_KEY_uparrow:
		case GDK_KEY_Up:ascii_key=LV_KEY_UP;break;
		case GDK_KEY_downarrow:
		case GDK_KEY_Down:ascii_key=LV_KEY_DOWN;break;
		case GDK_KEY_Escape:ascii_key=LV_KEY_ESC;break;
		case GDK_KEY_BackSpace:ascii_key=LV_KEY_BACKSPACE;break;
		case GDK_KEY_Delete:ascii_key=LV_KEY_DEL;break;
		case GDK_KEY_Tab:ascii_key=LV_KEY_NEXT;break;
		case GDK_KEY_KP_Enter:
		case GDK_KEY_Return:
		case '\r':ascii_key=LV_KEY_ENTER;break;
		default:break;
	}
	last_key=ascii_key,last_key_state=LV_INDEV_STATE_PR;
	gui_quit_sleep();
	return TRUE;
}

static gboolean keyboard_release(
	GtkWidget *widget __attribute__((unused)),
	GdkEventKey *event __attribute__((unused)),
	gpointer user_data __attribute__((unused))
){
	last_key=0,last_key_state=LV_INDEV_STATE_REL;
	gui_quit_sleep();
	return TRUE;
}

static void gtk_apply_mode(){
	int cnt=0;
	char*name=NULL;
	struct display_mode*mode=NULL;
	for(cnt=0;builtin_modes[cnt].name[0];cnt++);
	name=confd_get_string("gui.mode",NULL);
	if(!name){
		char*n=getenv("GUIMODE");
		if(n)name=strdup(n);
	}
	if(!name)goto done;
	if(cnt<=0)EDONE(tlog_warn("no any modes found"));
	for(int i=0;i<cnt;i++)
		if(strcasecmp(name,builtin_modes[i].name)==0)
			mode=&builtin_modes[i];
	if(!mode)EDONE(tlog_warn("mode %s not found",name));
	tlog_info("set mode to %s",name);
	ww=mode->width;
	hh=mode->height;
	done:
	if(name)free(name);
}

static void gtkdrv_init(void){
	gtk_init(NULL,NULL);
	window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window),ww,hh);
	gtk_window_set_resizable(GTK_WINDOW(window),FALSE);
	output_image=gtk_image_new();
	event_box=gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(event_box),output_image);
	gtk_container_add(GTK_CONTAINER(window),event_box);
	gtk_widget_add_events(event_box,GDK_BUTTON_PRESS_MASK);
	gtk_widget_add_events(event_box,GDK_SCROLL_MASK);
	gtk_widget_add_events(event_box,GDK_POINTER_MOTION_MASK);
	gtk_widget_add_events(window,GDK_KEY_PRESS_MASK);
	g_signal_connect(window,"destroy",G_CALLBACK(quit_handler),NULL);
	g_signal_connect(event_box,"button-press-event",G_CALLBACK(mouse_pressed),NULL);
	g_signal_connect(event_box,"button-release-event",G_CALLBACK(mouse_released),NULL);
	g_signal_connect(event_box,"motion-notify-event",G_CALLBACK(mouse_motion),NULL);
	g_signal_connect(window,"key_press_event",G_CALLBACK(keyboard_press),NULL);
	g_signal_connect(window,"key_release_event",G_CALLBACK(keyboard_release),NULL);
	gtk_widget_show_all(window);
	pixbuf=gdk_pixbuf_new_from_data(
		(guchar*)fb,
		GDK_COLORSPACE_RGB,
		false,8,
		ww,hh,ww*3,
		NULL,NULL
	);
	if(!pixbuf){
		tlog_error("creating pixbuf failed");
		return;
	}
	pthread_t thread;
	pthread_create(&thread,NULL,gtkdrv_handler,NULL);
}

static uint32_t gtkdrv_tick_get(void){
	static uint64_t start_ms=0;
	if(start_ms==0){
		struct timeval tv_start;
		gettimeofday(&tv_start,NULL);
		start_ms=(tv_start.tv_sec*1000000+tv_start.tv_usec)/1000;
	}
	struct timeval tv_now;
	gettimeofday(&tv_now,NULL);
	uint64_t now_ms;
	now_ms=(tv_now.tv_sec*1000000+tv_now.tv_usec)/1000;
	uint32_t time_ms=now_ms-start_ms;
	return time_ms;
}

static void gtkdrv_flush_cb(lv_disp_drv_t*disp_drv,const lv_area_t*area,lv_color_t*color_p){
	lv_coord_t hres=disp_drv->rotated==0?disp_drv->hor_res:disp_drv->ver_res;
	lv_coord_t vres=disp_drv->rotated==0?disp_drv->ver_res:disp_drv->hor_res;
	if(
		area->x2<0||
		area->y2<0||
		area->x1>hres-1||
		area->y1>vres-1
	){
		lv_disp_flush_ready(disp_drv);
		return;
	}
	int32_t y,x,p;
	for(y=area->y1;y<=area->y2&&y<disp_drv->ver_res;y++){
		p=(y*disp_drv->hor_res+area->x1)*3;
		for(x=area->x1;x<=area->x2&&x<disp_drv->hor_res;x++){
			fb[p]=color_p->ch.red;
			fb[p+1]=color_p->ch.green;
			fb[p+2]=color_p->ch.blue;
			p+=3;
			color_p ++;
		}
	}
	lv_disp_flush_ready(disp_drv);
}

static void gtkdrv_mouse_read_cb(
	lv_indev_drv_t*drv __attribute__((unused)),
	lv_indev_data_t*data
){
	data->point.x=mouse_x;
	data->point.y=mouse_y;
	data->state=mouse_btn;
}

static void gtkdrv_keyboard_read_cb(
	lv_indev_drv_t*drv __attribute__((unused)),
	lv_indev_data_t*data
){
	data->key=last_key;
	data->state=last_key_state;
}

static int gtkdrv_scan_init_register(){
	size_t s=ww*hh;
	lv_color_t*buf=NULL;
	static lv_disp_draw_buf_t disp_buf;
	if(!getenv("DISPLAY"))return -1;
	gtk_apply_mode();
	if(!(buf=malloc(s*sizeof(lv_color_t))))
		return terlog_error(-1,"malloc display buffer");
	if(!(fb=malloc(ww*hh*3)))
		return terlog_error(-1,"malloc framebuffer failed");
	memset(buf,0,s);
	lv_disp_draw_buf_init(&disp_buf,buf,NULL,s);
	gtkdrv_init();

	static lv_disp_drv_t disp;
	lv_disp_drv_init(&disp);
	disp.draw_buf=&disp_buf;
	disp.flush_cb=gtkdrv_flush_cb;
	disp.hor_res=ww;
	disp.ver_res=hh;
	disp.draw_ctx_init=lv_draw_sw_init_ctx;
	disp.draw_ctx_deinit=lv_draw_sw_init_ctx;
	disp.draw_ctx_size=sizeof(lv_draw_sw_ctx_t);
	switch(gui_rotate){
		case 0:break;
		case 90:disp.sw_rotate=1,disp.rotated=LV_DISP_ROT_90;break;
		case 180:disp.sw_rotate=1,disp.rotated=LV_DISP_ROT_180;break;
		case 270:disp.sw_rotate=1,disp.rotated=LV_DISP_ROT_270;break;
	}
	lv_disp_drv_register(&disp);

	return 0;
}
static int mse_init(){
	static lv_indev_drv_t mouse;
	lv_indev_drv_init(&mouse);
	mouse.type=LV_INDEV_TYPE_POINTER;
	mouse.read_cb=gtkdrv_mouse_read_cb;
	lv_indev_drv_register(&mouse);
	return 0;
}
static int kbd_init(){
	static lv_indev_drv_t kbd;
	lv_indev_drv_init(&kbd);
	kbd.type=LV_INDEV_TYPE_KEYPAD;
	kbd.read_cb=gtkdrv_keyboard_read_cb;
	lv_indev_drv_register(&kbd);
	return 0;
}
static void gtkdrv_get_sizes(lv_coord_t*width,lv_coord_t*height){
	lv_coord_t w=0,h=0;
	switch(gui_rotate){
		case 0:case 180:w=ww,h=hh;break;
		case 90:case 270:w=hh,h=ww;break;
	}
	if(width)*width=w;
	if(height)*height=h;
}
static void gtkdrv_get_dpi(int*dpi){
	if(dpi)*dpi=200;
}
static bool gtkdrv_can_sleep(){
	return false;
}
static int gtkdrv_get_modes(int*cnt,struct display_mode**modes){
	if(!cnt||!modes)ERET(EINVAL);
	for(*cnt=0;builtin_modes[*cnt].name[0];(*cnt)++);
	if(*cnt<=0)return 0;
	size_t size=((*cnt)+1)*sizeof(struct display_mode);
	if(!(*modes=malloc(size)))ERET(ENOMEM);
	memcpy(*modes,builtin_modes,size);
	return 0;
}

struct input_driver indrv_gtk_kbd={
	.name="gtk-keyboard",
	.compatible={
		"gtk",
		NULL
	},
	.drv_register=kbd_init,
};
struct input_driver indrv_gtk_mse={
	.name="gtk-mouse",
	.compatible={
		"gtk",
		NULL
	},
	.drv_register=mse_init,
};
struct gui_driver guidrv_gtk={
	.name="gtk",
	.drv_register=gtkdrv_scan_init_register,
	.drv_getsize=gtkdrv_get_sizes,
	.drv_getdpi=gtkdrv_get_dpi,
	.drv_tickget=gtkdrv_tick_get,
	.drv_taskhandler=gtkdrv_taskhandler,
	.drv_cansleep=gtkdrv_can_sleep,
	.drv_get_modes=gtkdrv_get_modes,
};
#endif
#endif
