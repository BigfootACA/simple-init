#ifdef ENABLE_GUI
#ifdef ENABLE_UEFI
#include<stdlib.h>
#include<Library/UefiBootManagerLib.h>
#include"gui.h"
#include"logger.h"
#include"language.h"
#include"gui/tools.h"
#include"gui/activity.h"
#define TAG "bootmenu"
static lv_obj_t*lst=NULL,*last=NULL;
static lv_obj_t*options_info,*btn_ok,*btn_refresh;
static EFI_BOOT_MANAGER_LOAD_OPTION*cur=NULL;
static struct option_info{
	bool enable;
	lv_obj_t*btn,*chk;
	char name[512];
	EFI_BOOT_MANAGER_LOAD_OPTION*option;
}options[256]={0};
static struct option_info*selected=NULL;

static void option_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_VALUE_CHANGED)return;
	lv_checkbox_set_checked(obj,true);
	if(selected){
		if(obj==selected->chk)return;
		else{
			lv_checkbox_set_checked(selected->chk,false);
			lv_obj_clear_state(selected->btn,LV_STATE_CHECKED);
		}
	}
	selected=NULL;
	for(int i=0;i<256&&!selected;i++)
		if(options[i].enable&&options[i].chk==obj)
			selected=&options[i];
	if(!selected)return;
	lv_obj_add_state(selected->btn,LV_STATE_CHECKED);
	lv_obj_clear_state(btn_ok,LV_STATE_DISABLED);
	tlog_debug("selected option %s",selected->name);
}

static void bootmenu_option_clear(){
	selected=NULL;
	lv_obj_add_state(btn_ok,LV_STATE_DISABLED);
	if(options_info)lv_obj_del(options_info);
	options_info=NULL,last=NULL;
	for(int i=0;i<256;i++){
		if(!options[i].enable)continue;
		lv_obj_del(options[i].btn);
		memset(&options[i],0,sizeof(struct option_info));
	}
}

static void bootmenu_set_options_info(char*text){
	bootmenu_option_clear();
	options_info=lv_label_create(lst,NULL);
	lv_label_set_long_mode(options_info,LV_LABEL_LONG_BREAK);
	lv_obj_set_size(options_info,lv_page_get_scrl_width(lst),gui_sh/16);
	lv_label_set_align(options_info,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(options_info,text);
}

static void options_add_item(struct option_info*k){
	lv_coord_t bw;
	bw=lv_page_get_scrl_width(lst);

	// option select button
	k->btn=lv_btn_create(lst,NULL);
	lv_obj_set_size(k->btn,bw,gui_dpi/3);
	lv_obj_align(
		k->btn,last,
		last?LV_ALIGN_OUT_BOTTOM_MID:LV_ALIGN_IN_TOP_MID,
		0,last?gui_dpi/8:0
	);
	lv_style_set_btn_item(k->btn);
	lv_obj_set_click(k->btn,false);
	last=k->btn;

	// option name and checkbox
	k->chk=lv_checkbox_create(k->btn,NULL);
	lv_checkbox_ext_t*e=lv_obj_get_ext_attr(k->chk);
	lv_label_set_long_mode(e->label,LV_LABEL_LONG_SROLL_CIRC);
	lv_obj_set_width(e->label,bw-gui_dpi/5*2);
	lv_checkbox_set_text(k->chk,k->name);
	lv_obj_set_event_cb(k->chk,option_click);
	lv_style_set_focus_checkbox(k->chk);
	lv_group_add_obj(gui_grp,k->chk);
}

static void bootmenu_option_reload(){
	bootmenu_option_clear();
	UINTN cnt,i;
	EFI_BOOT_MANAGER_LOAD_OPTION*bo;
	EfiBootManagerRefreshAllBootOption();
	if(!(bo=EfiBootManagerGetLoadOptions(&cnt,LoadOptionTypeBoot)))
		bootmenu_set_options_info(_("Load options failed"));
	else if(cnt<=0)
		bootmenu_set_options_info(_("No options found"));
	else for(i=0;i<cnt;i++){
		struct option_info*k=&options[i];
		EFI_BOOT_MANAGER_LOAD_OPTION*b=&bo[i];
		wcstombs(k->name,b->Description,sizeof(k->name)-1);
		tlog_debug("option %lld: %s",i+1,k->name);
		k->enable=true,k->option=b;
		options_add_item(k);
		if(i>=256){
			tlog_warn("options too many, only show 256 options");
			break;
		}
	}
	tlog_info("found %lld options",i);
}

static void refresh_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED||obj!=btn_refresh)return;
	tlog_debug("request refresh");
	bootmenu_option_reload();
}

static int after_exit(void*d __attribute__((unused))){
	if(!cur)return -1;
	EfiBootManagerBoot(cur);
	return 0;
}

static void ok_click(lv_obj_t*obj,lv_event_t e){
	if(e!=LV_EVENT_CLICKED||obj!=btn_ok||!selected)return;
	tlog_notice("shutdown guiapp and start option %s",selected->name);
	cur=selected->option;
	gui_run_and_exit(after_exit);
}

static int do_cleanup(struct gui_activity*d __attribute__((unused))){
	bootmenu_option_clear();
	return 0;
}

static void do_reload(lv_task_t*t __attribute__((unused))){
	bootmenu_option_reload();
	lv_group_add_obj(gui_grp,btn_ok);
	lv_group_add_obj(gui_grp,btn_refresh);
}

static int bootmenu_option_get_focus(struct gui_activity*d __attribute__((unused))){
	lv_task_once(lv_task_create(do_reload,100,LV_TASK_PRIO_MID,NULL));
	return 0;
}

static int bootmenu_option_lost_focus(struct gui_activity*d __attribute__((unused))){
	for(int i=0;i<256;i++){
		if(!options[i].enable)continue;
		lv_group_remove_obj(options[i].chk);
	}
	lv_group_remove_obj(btn_ok);
	lv_group_remove_obj(btn_refresh);
	return 0;
}

static int uefi_bootmenu_draw(struct gui_activity*act){

	lv_coord_t btm=gui_dpi/10;
	lv_coord_t btw=gui_sw/2-btm*2;
	lv_coord_t bth=gui_font_size+gui_dpi/10;

	// function title
	lv_obj_t*title=lv_label_create(act->page,NULL);
	lv_label_set_long_mode(title,LV_LABEL_LONG_BREAK);
	lv_obj_set_y(title,gui_sh/16);
	lv_obj_set_size(title,gui_sw,gui_sh/16);
	lv_label_set_align(title,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(title,_("UEFI Boot Menu"));

	// options list
	static lv_style_t lst_style;
	lv_style_init(&lst_style);
	lv_style_set_border_width(&lst_style,LV_STATE_DEFAULT,0);
	lv_style_set_border_width(&lst_style,LV_STATE_FOCUSED,0);
	lv_style_set_border_width(&lst_style,LV_STATE_PRESSED,0);
	lst=lv_page_create(act->page,NULL);
	lv_obj_add_style(lst,LV_PAGE_PART_BG,&lst_style);
	lv_obj_set_pos(lst,gui_dpi/20,gui_sh/8);
	lv_obj_set_size(lst,gui_sw-gui_dpi/10,gui_sh-lv_obj_get_y(lst)-bth-btm*2);

	// button style
	static lv_style_t btn_style;
	lv_style_init(&btn_style);
	lv_style_set_radius(&btn_style,LV_STATE_DEFAULT,2);
	lv_style_set_outline_width(&btn_style,LV_STATE_PRESSED,0);

	// ok button
	btn_ok=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_ok,btw,bth);
	lv_obj_align(btn_ok,NULL,LV_ALIGN_IN_BOTTOM_LEFT,btm,-btm);
	lv_obj_set_event_cb(btn_ok,ok_click);
	lv_style_set_action_button(btn_ok,false);
	lv_label_set_text(lv_label_create(btn_ok,NULL),_("OK"));

	// refresh button
	btn_refresh=lv_btn_create(act->page,NULL);
	lv_obj_set_size(btn_refresh,btw,bth);
	lv_obj_align(btn_refresh,NULL,LV_ALIGN_IN_BOTTOM_RIGHT,-btm,-btm);
	lv_obj_set_event_cb(btn_refresh,refresh_click);
	lv_style_set_action_button(btn_refresh,true);
	lv_label_set_text(lv_label_create(btn_refresh,NULL),_("Refresh"));

	return 0;
}

struct gui_register guireg_uefi_bootmenu={
	.name="uefi-bootmenu",
	.title="UEFI Boot Menu",
	.icon="bootmgr.png",
	.show_app=true,
	.draw=uefi_bootmenu_draw,
	.quiet_exit=do_cleanup,
	.get_focus=bootmenu_option_get_focus,
	.lost_focus=bootmenu_option_lost_focus,
	.back=true,
};
#endif
#endif
