#ifdef ENABLE_GUI
#ifdef ENABLE_GTK
#include<stdlib.h>
#include<unistd.h>
#include<gtk/gtk.h>
#include<pthread.h>
#include<sys/time.h>
#include"lvgl.h"
#include"gui.h"
#include"guidrv.h"
#include"logger.h"
#include"hardware.h"
#define TAG "gtk"

#define GTK_W 540
#define GTK_H 960
static GtkWidget*window,*event_box,*output_image;
static GdkPixbuf*pixbuf;
static lv_coord_t mouse_x,mouse_y;
static lv_indev_state_t mouse_btn=LV_INDEV_STATE_REL,last_key_state;
static lv_key_t last_key;
static uint8_t fb[GTK_W*GTK_H*3];

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

static void gtkdrv_init(void){
	gtk_init(NULL,NULL);
	window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window),GTK_W,GTK_H);
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
		false,
		8,
		GTK_W,
		GTK_H,
		GTK_W*3,
		NULL,
		NULL
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

static bool gtkdrv_mouse_read_cb(
	lv_indev_drv_t*drv __attribute__((unused)),
	lv_indev_data_t*data
){
	data->point.x=mouse_x;
	data->point.y=mouse_y;
	data->state=mouse_btn;
	return false;
}

static bool gtkdrv_keyboard_read_cb(
	lv_indev_drv_t*drv __attribute__((unused)),
	lv_indev_data_t*data
){
	data->key=last_key;
	data->state=last_key_state;
	return false;
}

static int gtkdrv_scan_init_register(){
	static size_t s=GTK_W*GTK_H;
	static lv_color_t*buf=NULL;
	static lv_disp_buf_t disp_buf;
	if(!getenv("DISPLAY"))return -1;
	if(!(buf=malloc(s*sizeof(lv_color_t)))){
		telog_error("malloc display buffer");
		return -1;
	}
	memset(buf,0,s);
	lv_disp_buf_init(&disp_buf,buf,NULL,s);
	gtkdrv_init();

	lv_disp_drv_t disp;
	lv_disp_drv_init(&disp);
	disp.buffer=&disp_buf;
	disp.flush_cb=gtkdrv_flush_cb;
	disp.hor_res=GTK_W;
	disp.ver_res=GTK_H;
	lv_disp_drv_register(&disp);
	lv_indev_drv_t mouse;
	lv_indev_drv_init(&mouse);
	mouse.type=LV_INDEV_TYPE_POINTER;
	mouse.read_cb=gtkdrv_mouse_read_cb;
	lv_indev_drv_register(&mouse);

	lv_indev_drv_t kbd;
	lv_indev_drv_init(&kbd);
	kbd.type=LV_INDEV_TYPE_KEYPAD;
	kbd.read_cb=gtkdrv_keyboard_read_cb;
	lv_indev_drv_register(&kbd);
	return 0;
}
static void gtkdrv_get_sizes(uint32_t*w,uint32_t*h){
	if(w)*w=GTK_W;
	if(h)*h=GTK_H;
}
static void gtkdrv_get_dpi(int*dpi){
	if(dpi)*dpi=200;
}
static bool gtkdrv_can_sleep(){
	return false;
}

struct gui_driver guidrv_gtk={
	.name="gtk",
	.drv_register=gtkdrv_scan_init_register,
	.drv_getsize=gtkdrv_get_sizes,
	.drv_getdpi=gtkdrv_get_dpi,
	.drv_tickget=gtkdrv_tick_get,
	.drv_taskhandler=gtkdrv_taskhandler,
	.drv_cansleep=gtkdrv_can_sleep
};
#endif
#endif
