#ifdef ENABLE_GUI
#ifdef ENABLE_UEFI
#include<Library/UefiRuntimeServicesTableLib.h>
#else
#include"init_internal.h"
#endif
#include"lvgl.h"
#include"logger.h"
#include"activity.h"
#include"defines.h"
#include"gui.h"
#include"str.h"
#include"tools.h"
#define TAG "reboot"

static bool valid=true;
static lv_obj_t*scr,*box;
static lv_obj_t*btn_po,*btn_rb_edl,*btn_rb_rec,*btn_rb_bl;
#ifdef ENABLE_UEFI
static lv_obj_t*btn_rb_cold,*btn_rb_warm;
#else
static lv_obj_t*btn_rb;
#endif

enum reboot_mode{
	#ifndef ENABLE_UEFI
	REBOOT,
	#else
	REBOOT_COLD,
	REBOOT_WARM,
	#endif
	REBOOT_EDL,
	REBOOT_RECOVERY,
	REBOOT_BOOTLOADER,
	POWEROFF,
};

static const char*reboot_str[]={
	#ifndef ENABLE_UEFI
	[REBOOT]            = "Reboot",
	#else
	[REBOOT_WARM]       = "Warm Reboot",
	[REBOOT_COLD]       = "Cold Reboot",
	#endif
	[REBOOT_EDL]        = "Reboot into EDL (9008)",
	[REBOOT_RECOVERY]   = "Reboot into Recovery",
	[REBOOT_BOOTLOADER] = "Reboot into Bootloader",
	[POWEROFF]          = "Power Off",
};

#ifndef ENABLE_UEFI
static void ok_msg_click(lv_obj_t*obj,lv_event_t e){
	if(e==LV_EVENT_DELETE){
		guiact_do_back();
	}else if(e==LV_EVENT_VALUE_CHANGED){
		lv_msgbox_start_auto_close(obj,0);
	}
}
#endif

static void reboot_action(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	void*d=lv_obj_get_user_data(obj);
	if(!d)return;
	enum reboot_mode m=*(enum reboot_mode*)d;
	#ifndef ENABLE_UEFI
	struct init_msg msg;
	init_initialize_msg(&msg,ACTION_REBOOT);
	switch(m){
		case REBOOT:break;
		case REBOOT_EDL:strcpy(msg.data.data,"edl");break;
		case REBOOT_RECOVERY:strcpy(msg.data.data,"recovery");break;
		case REBOOT_BOOTLOADER:strcpy(msg.data.data,"bootloader");break;
		case POWEROFF:msg.action=ACTION_POWEROFF;break;
		default:return;
	}
	struct init_msg response;
	errno=0;
	init_send(&msg,&response);
	if(errno!=0||response.data.status.ret!=0){
		int ex=(errno==0)?response.data.status.ret:errno;
		lv_create_ok_msgbox(scr,ok_msg_click,_("init control command failed: %s"),strerror(ex));
		lv_obj_del_async(box);
		valid=false;
		return;
	}
	#else
	UINTN s=0;
	VOID*x=NULL;
	EFI_RESET_TYPE t;
	switch(m){
		case REBOOT_BOOTLOADER: t=EfiResetPlatformSpecific,s=10,x="bootloader";break;
		case REBOOT_RECOVERY:   t=EfiResetPlatformSpecific,s=9, x="recovery";break;
		case REBOOT_EDL:        t=EfiResetPlatformSpecific,s=4, x="edl";break;
		case REBOOT_WARM:       t=EfiResetWarm;break;
		case REBOOT_COLD:       t=EfiResetCold;break;
		case POWEROFF:          t=EfiResetShutdown;break;
		default:return;
	}
	gRT->ResetSystem(t,EFI_SUCCESS,s,x);
	#endif
	guiact_do_back();
}

static int reboot_get_focus(struct gui_activity*d __attribute__((unused))){
	if(!valid)return 0;
	#ifndef ENABLE_UEFI
	lv_group_add_obj(gui_grp,btn_rb);
	#else
	lv_group_add_obj(gui_grp,btn_rb_cold);
	lv_group_add_obj(gui_grp,btn_rb_warm);
	#endif
	lv_group_add_obj(gui_grp,btn_rb_edl);
	lv_group_add_obj(gui_grp,btn_rb_rec);
	lv_group_add_obj(gui_grp,btn_rb_bl);
	lv_group_add_obj(gui_grp,btn_po);
	return 0;
}

static int reboot_lost_focus(struct gui_activity*d __attribute__((unused))){
	if(!valid)return 0;
	#ifndef ENABLE_UEFI
	lv_group_remove_obj(btn_rb);
	#else
	lv_group_remove_obj(btn_rb_cold);
	lv_group_remove_obj(btn_rb_warm);
	#endif
	lv_group_remove_obj(btn_rb_edl);
	lv_group_remove_obj(btn_rb_rec);
	lv_group_remove_obj(btn_rb_bl);
	lv_group_remove_obj(btn_po);
	return 0;
}

static lv_obj_t*add_reboot_button(lv_obj_t*last,enum reboot_mode mode){
	lv_obj_t*btn=lv_btn_create(box,NULL);
	lv_obj_t*txt=lv_label_create(btn,NULL);
	lv_label_set_text(txt,_(reboot_str[mode]));
	lv_obj_set_width(btn,lv_page_get_scrl_width(box)-gui_dpi/10);
	lv_obj_align(btn,last,LV_ALIGN_OUT_BOTTOM_MID,0,gui_font_size/2);
	lv_obj_set_user_data(btn,memdup(&mode,sizeof(mode)));
	lv_obj_set_event_cb(btn,reboot_action);
	return btn;
}

static int reboot_menu_draw(struct gui_activity*act){
	scr=act->page;

	static lv_style_t bs;
	lv_style_init(&bs);
	lv_style_set_pad_all(&bs,LV_STATE_DEFAULT,gui_font_size);

	box=lv_page_create(scr,NULL);
	lv_obj_add_style(box,LV_PAGE_PART_BG,&bs);
	lv_obj_set_click(box,false);
	lv_obj_set_width(box,gui_sw/6*5);

	lv_obj_t*txt=lv_label_create(box,NULL);
	lv_label_set_long_mode(txt,LV_LABEL_LONG_BREAK);
	lv_obj_set_width(txt,lv_page_get_scrl_width(box));
	lv_label_set_text(txt,_("Select an action you want to do"));

	#ifndef ENABLE_UEFI
	btn_rb=add_reboot_button(txt,REBOOT);
	btn_rb_edl=add_reboot_button(btn_rb,REBOOT_EDL);
	btn_rb_rec=add_reboot_button(btn_rb_edl,REBOOT_RECOVERY);
	btn_rb_bl=add_reboot_button(btn_rb_rec,REBOOT_BOOTLOADER);
	btn_po=add_reboot_button(btn_rb_bl,POWEROFF);
	#else
	btn_rb_cold=add_reboot_button(txt,REBOOT_COLD);
	btn_rb_warm=add_reboot_button(btn_rb_cold,REBOOT_WARM);
	btn_rb_edl=add_reboot_button(btn_rb_warm,REBOOT_EDL);
	btn_rb_rec=add_reboot_button(btn_rb_edl,REBOOT_RECOVERY);
	btn_rb_bl=add_reboot_button(btn_rb_rec,REBOOT_BOOTLOADER);
	btn_po=add_reboot_button(btn_rb_bl,POWEROFF);
	#endif
	lv_obj_set_height(box,lv_obj_get_y(btn_po)+lv_obj_get_height(btn_po)+(gui_font_size/3*8));
	lv_obj_align(box,NULL,LV_ALIGN_CENTER,0,0);
	return 0;
}

struct gui_register guireg_reboot={
	.name="reboot-menu",
	.title="Reboot Menu",
	.icon="reboot.png",
	.show_app=true,
	.draw=reboot_menu_draw,
	.lost_focus=reboot_lost_focus,
	.get_focus=reboot_get_focus,
	.back=true,
	.mask=true,
};
#endif
