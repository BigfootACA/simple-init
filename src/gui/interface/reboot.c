#ifdef ENABLE_GUI
#ifndef ENABLE_UEFI
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

static lv_obj_t*scr,*box;
static lv_obj_t*btn_rb,*btn_po;
#ifndef ENABLE_UEFI
static lv_obj_t*btn_rb_edl,*btn_rb_rec,*btn_rb_bl,*btn_po;
#endif

enum reboot_mode{
	REBOOT,
	#ifndef ENABLE_UEFI
	REBOOT_EDL,
	REBOOT_RECOVERY,
	REBOOT_BOOTLOADER,
	#endif
	POWEROFF,
};

static const char*reboot_str[]={
	[REBOOT]            = "Reboot",
	#ifndef ENABLE_UEFI
	[REBOOT_EDL]        = "Reboot into EDL (9008)",
	[REBOOT_RECOVERY]   = "Reboot into Recovery",
	[REBOOT_BOOTLOADER] = "Reboot into Bootloader",
	#endif
	[POWEROFF]          = "Power Off",
};

static void ok_msg_click(lv_obj_t*obj,lv_event_t e){
	if(e==LV_EVENT_DELETE){
		guiact_do_back();
	}else if(e==LV_EVENT_VALUE_CHANGED){
		lv_msgbox_start_auto_close(obj,0);
	}
}

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
		return;
	}
	#else
	lv_create_ok_msgbox(scr,ok_msg_click,_("not supported operation: %d"),m);
	lv_obj_del_async(box);
	#endif
	guiact_do_back();
}

static int reboot_get_focus(void*d __attribute__((unused))){
	lv_group_add_obj(gui_grp,btn_rb);
	#ifndef ENABLE_UEFI
	lv_group_add_obj(gui_grp,btn_rb_edl);
	lv_group_add_obj(gui_grp,btn_rb_rec);
	lv_group_add_obj(gui_grp,btn_rb_bl);
	#endif
	lv_group_add_obj(gui_grp,btn_po);
	return 0;
}

static int reboot_lost_focus(void*d __attribute__((unused))){
	lv_group_remove_obj(btn_rb);
	#ifndef ENABLE_UEFI
	lv_group_remove_obj(btn_rb_edl);
	lv_group_remove_obj(btn_rb_rec);
	lv_group_remove_obj(btn_rb_bl);
	#endif
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

void reboot_menu_draw(lv_obj_t*screen){
	scr=lv_create_opa_mask(screen);

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
	btn_rb=add_reboot_button(txt,REBOOT);
	btn_po=add_reboot_button(btn_rb,POWEROFF);
	#endif
	lv_obj_set_height(box,lv_obj_get_y(btn_po)+lv_obj_get_height(btn_po)+(gui_font_size/3*8));
	lv_obj_align(box,NULL,LV_ALIGN_CENTER,0,0);

	guiact_register_activity(&(struct gui_activity){
		.name="reboot-menu",
		.ask_exit=NULL,
		.quiet_exit=NULL,
		.lost_focus=reboot_lost_focus,
		.get_focus=reboot_get_focus,
		.back=true,
		.page=scr
	});
}
#endif
