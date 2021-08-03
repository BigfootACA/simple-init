#include<libintl.h>
#include"init_internal.h"
#include"lvgl.h"
#include"logger.h"
#include"activity.h"
#include"defines.h"
#include"gui.h"
#include"str.h"
#include"tools.h"
#define TAG "reboot"

static lv_obj_t*scr,*box;

enum reboot_mode{
	REBOOT,
	REBOOT_EDL,
	REBOOT_RECOVERY,
	REBOOT_BOOTLOADER,
	POWEROFF,
};

static const char*reboot_str[]={
	[REBOOT]            = "Reboot",
	[REBOOT_EDL]        = "Reboot into EDL (9008)",
	[REBOOT_RECOVERY]   = "Reboot into Recovery",
	[REBOOT_BOOTLOADER] = "Reboot into Bootloader",
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
		lv_create_ok_msgbox(scr,ok_msg_click,"init control command failed: %s",strerror(ex));
		lv_obj_del_async(box);
		return;
	}
	guiact_do_back();
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
	lv_obj_set_width(box,gui_sw/6*5);

	lv_obj_t*txt=lv_label_create(box,NULL);
	lv_label_set_long_mode(txt,LV_LABEL_LONG_BREAK);
	lv_obj_set_width(txt,lv_page_get_scrl_width(box));
	lv_label_set_text(txt,_("Select an action you want to do"));

	lv_obj_t*l=txt;
	l=add_reboot_button(l,REBOOT);
	l=add_reboot_button(l,REBOOT_EDL);
	l=add_reboot_button(l,REBOOT_RECOVERY);
	l=add_reboot_button(l,REBOOT_BOOTLOADER);
	l=add_reboot_button(l,POWEROFF);
	lv_obj_set_height(box,lv_obj_get_y(l)+lv_obj_get_height(l)+(gui_font_size*2));
	lv_obj_align(box,NULL,LV_ALIGN_CENTER,0,0);

	guiact_register_activity(&(struct gui_activity){
		.name="reboot-menu",
		.ask_exit=NULL,
		.quiet_exit=NULL,
		.back=true,
		.page=scr
	});
}
