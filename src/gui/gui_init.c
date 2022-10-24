/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#ifdef ENABLE_GUI
#include<errno.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<sys/time.h>
#ifdef ENABLE_UEFI
#include<Library/PcdLib.h>
#include<Library/UefiLib.h>
#include<Library/TimerLib.h>
#include<Library/ReportStatusCodeLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/Timestamp.h>
#else
#include<semaphore.h>
#include<pthread.h>
#endif
#ifdef ENABLE_LUA
#include"xlua.h"
#include"gui/lua.h"
#endif
#include"str.h"
#include"gui.h"
#include"confd.h"
#include"system.h"
#include"logger.h"
#include"defines.h"
#include"hardware.h"
#include"gui/font.h"
#include"gui/image.h"
#include"gui/sysbar.h"
#include"gui/guidrv.h"
#include"gui/activity.h"
#include"gui/clipboard.h"
#define TAG "gui"

// gui dpi
int gui_dpi_def=200,gui_dpi_force=0,gui_dpi=200;

// font size
int gui_font_size=24,gui_font_size_small=16;

// gui size
lv_coord_t gui_w=-1,gui_h=-1;

// gui usable size
lv_coord_t gui_sw,gui_sh,gui_sx,gui_sy;

// gui rotate
uint16_t gui_rotate;

// gui fonts
const lv_font_t*gui_font=NULL,*gui_font_small=NULL,*symbol_font=NULL;

// gui group (for buttons)
lv_group_t*gui_grp=NULL;

// mouse pointer
lv_obj_t*gui_cursor=NULL;

// keep running
bool gui_run=true;

// dark mode
#ifdef DEFAULT_DARK
#define DARK_MODE true
#else
#define DARK_MODE false
#endif
bool gui_dark=DARK_MODE;

#ifdef ENABLE_UEFI
#define DEF_ROTATE PcdGet16(PcdGuiDefaultRotate)

static uint64_t tick_ms=0;
#else
#define DEF_ROTATE confd_get_integer("runtime.cmdline.rotate",0)

// is sleeping
bool gui_sleep=false;

// gui sleep lock
static sem_t gui_wait;

// default backlight device fd
int default_backlight=-1;
#endif

#ifdef ENABLE_LUA
lua_State*gui_global_lua;
#endif

// gui process lock
mutex_t gui_lock;

// usable gui fontsize
static int font_sizes[]={10,16,24,32,48,64,72,96};

// run on exit
static runnable_t*run_exit=NULL;

void*lv_mem_pool=NULL;
size_t lv_mem_pool_size=0;

void gui_do_quit(){
	#ifdef ENABLE_LUA
	if(gui_global_lua)
		xlua_run_confd(gui_global_lua,TAG,"lua.on_gui_quit");
	#endif
	#ifndef ENABLE_UEFI
	sem_destroy(&gui_wait);
	MUTEX_DESTROY(gui_lock);
	#endif
	image_cache_clean();
	guidrv_exit();
	#if !LV_MEM_CUSTOM
	lv_deinit();
	if(lv_mem_pool){
		free(lv_mem_pool);
		lv_mem_pool_size=0;
		lv_mem_pool=NULL;
	}
	#endif
	#ifdef ENABLE_LUA
	if(gui_global_lua)lua_close(gui_global_lua);
	gui_global_lua=NULL;
	#endif
}

void gui_quit_sleep(){
	#ifndef ENABLE_UEFI
	if(gui_sleep){
		gui_sleep=false;
		lv_tick_elaps(LV_DISP_DEF_REFR_PERIOD);
		lv_task_handler();
		lv_disp_trig_activity(NULL);
		sem_post(&gui_wait);
	}
	#endif
}

void guess_font_size(){
	//                        DPI     Font Size
	int i=0;            //   0 -  50 : 10
	if(gui_dpi>50)i++;  //  51 - 150 : 16
	if(gui_dpi>150)i++; // 151 - 250 : 24
	if(gui_dpi>250)i++; // 251 - 350 : 32
	if(gui_dpi>350)i++; // 351 - 450 : 48
	if(gui_dpi>450)i++; // 451 - 550 : 64
	if(gui_dpi>550)i++; // >= 551    : 72

	gui_font_size=font_sizes[i];
	gui_font_size_small=font_sizes[i-1];
	tlog_debug("select font size %d/%d based on dpi",gui_font_size,gui_font_size_small);
}

static void lvgl_logger(const char*buf){
	logger_print(LEVEL_INFO,"lvgl",(char*)buf);
}

int gui_pre_init(){
	// initialize lvgl
	if(!lv_is_initialized()){
		#if !LV_MEM_CUSTOM
		char buf[64];
		if(lv_mem_pool_size<=0)lv_mem_pool_size=16*1024*1024;
		if(lv_mem_pool)free(lv_mem_pool);
		if(!(lv_mem_pool=malloc(lv_mem_pool_size))){
			telog_emerg("alloc mem pool failed");
			abort();
		}
		tlog_debug(
			"allocate %zu bytes (%s) memory pool at %p",
			lv_mem_pool_size,
			make_readable_str_buf(buf,sizeof(buf),lv_mem_pool_size,1,0),
			lv_mem_pool
		);
		memset(lv_mem_pool,0,lv_mem_pool_size);
		#endif
		lv_init();
	}

	#ifdef ENABLE_LUA
	if(!gui_global_lua)
		gui_global_lua=xlua_init();
	if(gui_global_lua)
		lua_gui_init(gui_global_lua);
	#endif

	#ifdef ENABLE_UEFI
	// load dpi
	gui_dpi_def=(int)PcdGet16(PcdGuiDefaultDPI);
	#endif

	// redirect lvgl log to loggerd
	lv_log_register_print_cb(lvgl_logger);

	// create group for buttons
	gui_grp=lv_group_create();

	// parse backlight device
	#ifndef ENABLE_UEFI
	char*x=confd_get_string("runtime.cmdline.backlight",NULL);
	if(x)default_backlight=led_parse_arg(x,"backlight");
	#endif
	gui_dpi=confd_get_integer("runtime.cmdline.dpi",confd_get_integer("gui.dpi",gui_dpi_def));
	gui_dpi_force=confd_get_integer("runtime.cmdline.dpi_force",confd_get_integer("gui.dpi_force",0));
	gui_dark=confd_get_boolean("gui.dark",DARK_MODE);
	gui_rotate=(uint16_t)confd_get_integer("gui.rotate",DEF_ROTATE);
	if(gui_rotate>=360)gui_rotate%=360;
	switch(gui_rotate){
		case 0:case 90:case 180:case 270:break;
		default:
			tlog_warn("invalid rotate %d",gui_rotate);
			gui_rotate=0;
		break;
	}

	// init gui
	if(guidrv_init(&gui_w,&gui_h,&gui_dpi)<0)return -1;
	gui_sx=0,gui_sy=0,gui_sw=gui_w,gui_sh=gui_h;
	indrv_init();
	int cnt=0;
	struct display_mode*modes=NULL;
	if(guidrv_get_modes(&cnt,&modes)==0&&cnt>0&&modes){
		tlog_debug("available display modes:");
		for(int i=0;i<cnt;i++)tlog_debug(
			"    %d: %s (%ux%u)",
			i,modes[i].name,
			modes[i].width,
			modes[i].height
		);
		free(modes);
	}
	tlog_debug("driver init done");
	guess_font_size();
	image_decoder_init();
	#ifdef ENABLE_FREETYPE2
	lv_freetype_init(64,1,0);

	#ifdef ENABLE_UEFI
	gui_font=NULL,gui_font_small=NULL,symbol_font=NULL;
	#else
	// load default font with normal size from environment variable
	gui_font=lv_ft_init(getenv("FONT"),gui_font_size,0);

	// load default font with small size from environment variable
	gui_font_small=lv_ft_init(getenv("FONT"),gui_font_size_small,0);

	// load symbols fonts from environment variable
	symbol_font=lv_ft_init(getenv("FONT_SYMBOL"),gui_font_size,0);
	#endif

	// load default font with normal size from builtin rootfs
	if(!gui_font)gui_font=lv_ft_init_rootfs(_PATH_ETC"/default.ttf",gui_font_size,0);

	// load default font with small size from builtin rootfs
	if(!gui_font_small)gui_font_small=lv_ft_init_rootfs(_PATH_ETC"/default.ttf",gui_font_size_small,0);

	// load symbols fonts from builtin rootfs
	if(!symbol_font)symbol_font=lv_ft_init_rootfs(_PATH_ETC"/symbols.ttf",gui_font_size,0);

	if(!symbol_font)telog_error("failed to load symbol font");
	#endif
	if(!gui_font||!gui_font_small)return terlog_error(-1,"failed to load font");

	((lv_font_t*)gui_font)->fallback=symbol_font;
	((lv_font_t*)gui_font_small)->fallback=symbol_font;

	#ifdef ENABLE_LUA
	if(gui_global_lua)
		xlua_run_confd(gui_global_lua,TAG,"lua.on_gui_pre_init");
	#endif
	return 0;
}

#ifndef ENABLE_UEFI
static void off_screen(int s __attribute__((unused))){
	tlog_debug("screen sleep");
	guidrv_set_brightness(0);
}

static void gui_enter_sleep(){

	// brightness level min 20
	int s=confd_get_integer("gui.sleep_brightness",20);
	int o=guidrv_get_brightness();
	if(s<0)s=0;
	if(s>100)s=100;
	if(o<=0)o=100;
	if(o>s)guidrv_set_brightness(s);

	#ifdef ENABLE_LUA
	if(gui_global_lua)
		xlua_run_confd(gui_global_lua,TAG,"lua.on_gui_enter_sleep");
	#endif

	// turn off the screen after 20 seconds
	int d=confd_get_integer("gui.screen_off_delay",20);
	if(d>0)alarm(d);
	signal(SIGALRM,off_screen);
	tlog_debug("enter sleep");
	gui_sleep=true;
	sem_wait(&gui_wait);
	gui_sleep=false;
	alarm(0);
	guidrv_set_brightness(o);
	tlog_debug("quit sleep");

	#ifdef ENABLE_LUA
	if(gui_global_lua)
		xlua_run_confd(gui_global_lua,TAG,"lua.on_gui_quit_sleep");
	#endif
}
#else

extern bool conf_store_changed;
static void conf_save_cb(lv_timer_t*t __attribute__((unused))){
	if(!conf_store_changed)return;
	int r=confd_save_file(NULL);
	if(r!=0||errno!=0)return;
	tlog_debug("config changed");
	conf_store_changed=false;
}

static void gui_enter_sleep(){
	lv_disp_trig_activity(NULL);
}
#endif

uint32_t custom_tick_get(void){
	static uint64_t start_ms=0;
	uint64_t cur_ms;
	errno=0;
	if(guidrv_get_driver()){
		uint32_t u=guidrv_tickget();
		if(errno==0)return u;
	}
	#ifdef ENABLE_UEFI
	static EFI_TIMESTAMP_PROTOCOL*tp=NULL;
	static EFI_TIMESTAMP_PROPERTIES ps;
	if(start_ms==0&&!tp){
		EFI_STATUS st;
		if(!tp){
			st=gBS->LocateProtocol(
				&gEfiTimestampProtocolGuid,
				NULL,(VOID**)&tp
			);
			if(EFI_ERROR(st))tp=NULL;
			if(tp)st=tp->GetProperties(&ps);
			if(EFI_ERROR(st))tp=NULL;
		}
		#ifdef NO_TIMER
		static bool warned=false;
		if(!tp&&warned){
			tlog_warn("unable to get stable timer, time may be inaccurate");
			warned=true;
		}
		#endif
	}
	if(tp){
		UINT64 t=tp->GetTimestamp(),f=ps.Frequency;
		cur_ms=((t/f)*1000000000u+((t%f)*1000000000u)/f)/1000/1000;
	}else
	#ifdef NO_TIMER
		cur_ms=tick_ms;
	#else
		cur_ms=GetTimeInNanoSecond(GetPerformanceCounter())/1000/1000;
	#endif
	#else
	struct timeval tv_start;
	gettimeofday(&tv_start,NULL);
	cur_ms=(tv_start.tv_sec*1000000+tv_start.tv_usec)/1000;
	#endif
	if(start_ms==0)start_ms=cur_ms;
	return cur_ms-start_ms;
}

int gui_screen_init(){
	lv_obj_t*screen;
	if(!(screen=lv_scr_act()))
		return trlog_error(-1,"failed to get screen");
	lv_obj_set_scroll_dir(screen,LV_DIR_NONE);

	#ifdef ENABLE_LUA
	if(gui_global_lua)
		xlua_run_confd(gui_global_lua,TAG,"lua.on_gui_screen_init_pre");
	#endif

	// set current fonts and themes
	lv_theme_t*th=lv_theme_default_init(
		lv_disp_get_default(),
		lv_palette_main(LV_PALETTE_BLUE),
		lv_palette_main(LV_PALETTE_RED),
		gui_dark,gui_font
	);
	lv_disp_set_theme(lv_disp_get_default(),th);

	// init gui activity
	guiact_init();

	// clean screen
	lv_obj_clean(screen);
	gui_cursor=NULL;
	memset(&sysbar,0,sizeof(struct sysbar));

	// add lvgl mouse pointer
	static char*cursor=NULL;
	if(!cursor)cursor=confd_get_string(
		"gui.cursor_image","\xef\x89\x85"
	); // mouse-pointer
	if(cursor){
		gui_cursor=lv_img_create(screen);
		lv_img_set_src(gui_cursor,cursor);
		lv_obj_set_pos(gui_cursor,-gui_w,-gui_h);
	}
	if(!gui_cursor)abort();

	char*k="gui.cursor_color";
	bool color=confd_get_type(k)==TYPE_INTEGER;
	if(color||!cursor)lv_obj_set_style_img_recolor(
		gui_cursor,
		color?lv_color_hex((uint32_t)confd_get_integer(k,0)):
		lv_obj_get_style_text_color(screen,LV_PART_MAIN),
		LV_PART_MAIN
	);
	#ifdef ENABLE_LUA
	if(gui_global_lua)
		xlua_run_confd(gui_global_lua,TAG,"lua.on_gui_screen_init_post");
	#endif
	return 0;
}

int gui_draw(){
	#ifdef ENABLE_LUA
	if(gui_global_lua)
		xlua_run_confd(gui_global_lua,TAG,"lua.on_gui_screen_draw_pre");
	#endif

	// draw top and bottom sysbar
	sysbar_draw(lv_scr_act());

	// restore clipboard
	clipboard_init();

	// draw callback
	guiact_start_activity_by_name("guiapp",NULL);

	#ifdef ENABLE_LUA
	if(gui_global_lua)
		xlua_run_confd(gui_global_lua,TAG,"lua.on_gui_screen_draw_post");
	#endif

	lv_disp_trig_activity(NULL);
	return 0;
}

#ifndef ENABLE_UEFI
static void gui_quit_handler(int s __attribute((unused))){
	gui_quit_sleep();
	gui_run=false;
}
#endif

static void image_cache_cb(lv_timer_t*t __attribute__((unused))){
	image_print_stat();
}

int gui_main(){
	int64_t i=confd_get_integer("gui.image_cache_statistics",0);
	if(i>0)lv_timer_create(image_cache_cb,i,NULL);
	MUTEX_INIT(gui_lock);
	#ifdef ENABLE_UEFI
	REPORT_STATUS_CODE(EFI_PROGRESS_CODE,(EFI_SOFTWARE_DXE_BS_DRIVER|EFI_SW_PC_INPUT_WAIT));

	// kill the watchdog
	gBS->SetWatchdogTimer(0,0,0,NULL);

	lv_timer_create(
		conf_save_cb,
		confd_get_integer("confd.save_interval",10)*1000,
		NULL
	);

	#else
	sem_init(&gui_wait,0,0);
	handle_signals((int[]){SIGINT,SIGQUIT,SIGTERM},3,gui_quit_handler);
	#endif
	bool cansleep=guidrv_can_sleep();
	if(!cansleep)tlog_notice("gui driver disabled sleep");
	if(!confd_get_boolean("gui.can_sleep",true)){
		tlog_notice("config disabled sleep");
		cansleep=false;
	}
	#ifdef ENABLE_LUA
	if(gui_global_lua)
		xlua_run_confd(gui_global_lua,TAG,"lua.on_gui_pre_main");
	#endif
	uint32_t time=30;
	while(gui_run){
		// 10 seconds inactive sleep
		if(lv_disp_get_inactive_time(NULL)<10000||!cansleep){
			MUTEX_LOCK(gui_lock);
			time=lv_task_handler();
			guidrv_taskhandler();
			MUTEX_UNLOCK(gui_lock);
		}else gui_enter_sleep();
		if(time>0){
			#ifdef ENABLE_UEFI
			tick_ms+=time;
			gBS->Stall(EFI_TIMER_PERIOD_MILLISECONDS(time));
			#else
			usleep(time*1000);
			#endif
		}
	}
	tlog_notice("exiting");

	gui_do_quit();
	#ifdef ENABLE_UEFI
	conf_save_cb(NULL);
	#endif
	return run_exit?run_exit(NULL):0;
}

int gui_init(){
	return (
		gui_pre_init()<0||
		gui_screen_init()<0||
		gui_draw()<0||
		gui_main()<0
	)?-1:0;
}

void gui_set_run_exit(runnable_t*run){
	run_exit=run;
}

void gui_run_and_exit(runnable_t*run){
	run_exit=run,gui_run=false;
}

#endif
