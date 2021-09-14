#define _GNU_SOURCE
#ifdef ENABLE_GUI
#include<errno.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<sys/time.h>
#ifdef ENABLE_UEFI
#include<Library/UefiLib.h>
#include<Library/ReportStatusCodeLib.h>
#include<Library/UefiBootServicesTableLib.h>
#else
#include<semaphore.h>
#endif
#include"lvgl.h"
#include"logger.h"
#include"defines.h"
#include"gui.h"
#include"font.h"
#include"sysbar.h"
#include"hardware.h"
#include"guidrv.h"
#define TAG "gui"

#ifndef ENABLE_UEFI
// default backlight device fd
int default_backlight=-1;
#endif

// gui dpi
int gui_dpi_def=200,gui_dpi_force=0,gui_dpi=200;

// font size
int gui_font_size=24,gui_font_size_small=16;

// gui size
uint32_t gui_w=-1,gui_h=-1;

// gui usable size
uint32_t gui_sw,gui_sh,gui_sx,gui_sy;

// gui fonts
lv_font_t*gui_font=&lv_font_montserrat_24,*gui_font_small=&lv_font_montserrat_24,*symbol_font=NULL;

// gui group (for buttons)
lv_group_t*gui_grp=NULL;

// mouse pointer
lv_obj_t*gui_cursor=NULL;

// keep running
bool gui_run=true;

#ifndef ENABLE_UEFI
// is sleeping
bool gui_sleep=false;

// gui sleep lock
static sem_t gui_wait;
#endif

// usable gui fontsize
static int font_sizes[]={10,16,24,32,48,64,72,96};

// run on exit
static runnable_t*run_exit=NULL;

void gui_do_quit(){
	guidrv_exit();
}

void gui_quit_sleep(){
	#ifndef ENABLE_UEFI
	if(gui_sleep){
		gui_sleep=false;
		lv_tick_inc(LV_DISP_DEF_REFR_PERIOD);
		lv_task_handler();
		lv_disp_trig_activity(NULL);
		sem_post(&gui_wait);
	}
	#endif
}

void guess_font_size(){
	int i=0;
	if(gui_dpi>50)i++;
	if(gui_dpi>150)i++;
	if(gui_dpi>250)i++;
	if(gui_dpi>350)i++;
	if(gui_dpi>450)i++;
	if(gui_dpi>550)i++;
	gui_font_size=font_sizes[i];
	gui_font_size_small=font_sizes[i-1];
	tlog_debug("select font size %d/%d based on dpi",gui_font_size,gui_font_size_small);
}

static void lvgl_logger(lv_log_level_t level,const char*file,uint32_t line,const char*func,const char*buf){
	enum log_level lvl;
	switch(level){
		case LV_LOG_LEVEL_TRACE: lvl=LEVEL_DEBUG;break;
		case LV_LOG_LEVEL_INFO:  lvl=LEVEL_INFO;break;
		case LV_LOG_LEVEL_WARN:  lvl=LEVEL_WARNING;break;
		case LV_LOG_LEVEL_ERROR: lvl=LEVEL_ERROR;break;
		case LV_LOG_LEVEL_USER:  lvl=LEVEL_INFO;break;
		default:                 lvl=LEVEL_DEBUG;break;
	}
	static char fn[PATH_MAX]={0};
	strncpy(fn,file,PATH_MAX-1);
	logger_printf(lvl,"lvgl","%s:%d@%s %s",basename(fn),line,func,buf);
}

static int gui_pre_init(){
	lv_init();
	lv_log_register_print_cb(lvgl_logger);
	gui_grp=lv_group_create();

	#ifndef ENABLE_UEFI
	// parse backlight device
	char*x=getenv("BACKLIGHT");
	if(x)default_backlight=led_parse_arg(x,"backlight");
	#endif

	// init gui
	if(guidrv_init(&gui_w,&gui_h,&gui_dpi)<0)return -1;
	gui_sx=0,gui_sy=0,gui_sw=gui_w,gui_sh=gui_h;
	tlog_debug("driver init done");
	guess_font_size();
	#ifdef ENABLE_LODEPNG
	png_decoder_init();
	#endif
	#ifdef ENABLE_FREETYPE2
	lv_freetype_init(64,1,0);

	#ifdef ENABLE_UEFI
	gui_font=NULL,gui_font_small=NULL,symbol_font=NULL;
	#else
	// load default font with normal size
	gui_font=lv_ft_init(getenv("FONT"),gui_font_size,0);

	// load default font with small size
	gui_font_small=lv_ft_init(getenv("FONT"),gui_font_size_small,0);

	// load symbols fonts
	symbol_font=lv_ft_init(getenv("FONT_SYMBOL"),gui_font_size,0);
	#endif

	// load default font with normal size
	if(!gui_font)gui_font=lv_ft_init_rootfs(_PATH_ETC"/default.ttf",gui_font_size,0);

	// load default font with small size
	if(!gui_font_small)gui_font_small=lv_ft_init_rootfs(_PATH_ETC"/default.ttf",gui_font_size_small,0);

	// load symbols fonts
	if(!symbol_font)symbol_font=lv_ft_init_rootfs(_PATH_ETC"/symbols.ttf",gui_font_size,0);

	if(!symbol_font)telog_error("failed to load symbol font");
	#endif
	if(!gui_font||!gui_font_small)return terlog_error(-1,"failed to load font");

	// set current fonts
	lv_theme_t*th=LV_THEME_DEFAULT_INIT(
		LV_THEME_DEFAULT_COLOR_PRIMARY,
		LV_THEME_DEFAULT_COLOR_SECONDARY,
		LV_THEME_DEFAULT_FLAG,
		gui_font_small,gui_font,gui_font,gui_font
	);
	lv_theme_set_act(th);
	return 0;
}

#ifndef ENABLE_UEFI
static void off_screen(int s __attribute__((unused))){
	tlog_debug("screen sleep");
	guidrv_set_brightness(0);
}

static void gui_enter_sleep(){

	// brightness level min 20
	int o=guidrv_get_brightness();
	if(o<=0)o=100;
	if(o>20)guidrv_set_brightness(20);

	// turn off the screen after 20 seconds
	alarm(20);
	signal(SIGALRM,off_screen);
	tlog_debug("enter sleep");
	gui_sleep=true;
	sem_wait(&gui_wait);
	gui_sleep=false;
	alarm(0);
	guidrv_set_brightness(o);
	tlog_debug("quit sleep");
}

uint32_t custom_tick_get(void){
	errno=0;
	if(guidrv_get_driver()){
		uint32_t u=guidrv_tickget();
		if(errno==0)return u;
	}
	static uint64_t start_ms=0;
	if(start_ms==0){
		struct timeval tv_start;
		gettimeofday(&tv_start,NULL);
		start_ms=(tv_start.tv_sec*1000000+tv_start.tv_usec)/1000;
	}
	struct timeval tv_now;
	gettimeofday(&tv_now,NULL);
	return (uint64_t)((tv_now.tv_sec*1000000+tv_now.tv_usec)/1000)-start_ms;
}
#else
static EFI_EVENT e_timer,e_loop;

static VOID EFIAPI efi_loop(IN EFI_EVENT e,IN VOID*ctx){}

static VOID EFIAPI efi_timer(IN EFI_EVENT e,IN VOID*ctx){
	lv_tick_inc(10);
	lv_task_handler();
	guidrv_taskhandler();
	if(!gui_run){
		tlog_notice("exiting");
		gBS->CloseEvent(e_timer);
		gBS->CloseEvent(e_loop);
	}
}

#endif

int gui_init(draw_func draw){
	if(gui_pre_init()<0)return -1;
	lv_obj_t*screen;
	if(!(screen=lv_scr_act()))return trlog_error(-1,"failed to get screen");
	gui_cursor=lv_img_create(screen,NULL);
	lv_img_set_src(gui_cursor,"\xef\x89\x85"); // mouse-pointer
	sysbar_draw(screen);
	draw(sysbar.content);
	lv_disp_trig_activity(NULL);
	#ifdef ENABLE_UEFI
	REPORT_STATUS_CODE(EFI_PROGRESS_CODE,(EFI_SOFTWARE_DXE_BS_DRIVER|EFI_SW_PC_INPUT_WAIT));
	gBS->SetWatchdogTimer(0,0,0,NULL);
	if(EFI_ERROR(gBS->CreateEvent(
		EVT_NOTIFY_SIGNAL|EVT_TIMER,TPL_CALLBACK,
		efi_timer,NULL,&e_timer
	)))return trlog_error(-1,"create timer event failed");
	if(EFI_ERROR(gBS->CreateEvent(
		EVT_NOTIFY_WAIT,TPL_NOTIFY,
		efi_loop,NULL,&e_loop
	)))return trlog_error(-1,"create loop event failed");
	if(EFI_ERROR(gBS->SetTimer(
		e_timer,TimerPeriodic,
		EFI_TIMER_PERIOD_MILLISECONDS(10)
	)))return trlog_error(-1,"create timer failed");
	UINTN wi;
	while(gui_run&&!EFI_ERROR(gBS->WaitForEvent(1,&e_loop,&wi)));
	#else
	sem_init(&gui_wait,0,0);
	bool cansleep=guidrv_can_sleep();
	if(!cansleep)tlog_notice("gui driver disabled sleep");
	while(gui_run){
		if(lv_disp_get_inactive_time(NULL)<10000||!cansleep){
			lv_task_handler();
			guidrv_taskhandler();
		}else gui_enter_sleep();
		usleep(30000);
	}
	#endif
	gui_do_quit();
	return run_exit?run_exit(NULL):0;
}

void gui_set_run_exit(runnable_t*run){
	run_exit=run;
}

void gui_run_and_exit(runnable_t*run){
	run_exit=run,gui_run=false;
}

#endif
