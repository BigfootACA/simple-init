/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_UEFI
#include<stdlib.h>
#include<Library/BaseLib.h>
#include<Library/UefiBootManagerLib.h>
#include"gui.h"
#include"confd.h"
#include"logger.h"
#include"language.h"
#include"gui/tools.h"
#include"gui/activity.h"
#define TAG "bootmenu"
static lv_obj_t*lst=NULL;
static lv_obj_t*options_info,*btn_ok,*btn_refresh;
static EFI_BOOT_MANAGER_LOAD_OPTION*cur=NULL;
static struct option_info{
	bool enable;
	lv_obj_t*btn,*lbl;
	char name[512];
	EFI_BOOT_MANAGER_LOAD_OPTION*option;
}options[256]={0};
static struct option_info*selected=NULL;

static void option_click(lv_event_t*e){
	lv_obj_set_checked(e->target,true);
	if(selected){
		if(e->target==selected->btn)return;
		else lv_obj_set_checked(selected->btn,false);
	}
	selected=NULL;
	for(int i=0;i<256&&!selected;i++)
		if(options[i].enable&&options[i].btn==e->target)
			selected=&options[i];
	if(!selected)return;
	lv_obj_set_checked(selected->btn,true);
	lv_obj_set_enabled(btn_ok,true);
	lv_group_focus_obj(btn_ok);
	tlog_debug("selected option %s",selected->name);
}

static void bootmenu_option_clear(){
	selected=NULL;
	lv_obj_set_enabled(btn_ok,false);
	if(options_info)lv_obj_del(options_info);
	options_info=NULL;
	for(int i=0;i<256;i++){
		if(!options[i].enable)continue;
		lv_obj_del(options[i].btn);
		memset(&options[i],0,sizeof(struct option_info));
	}
}

static void bootmenu_set_options_info(char*text){
	bootmenu_option_clear();
	options_info=lv_label_create(lst);
	lv_label_set_long_mode(options_info,LV_LABEL_LONG_WRAP);
	lv_obj_set_width(options_info,lv_pct(100));
	lv_obj_set_style_text_align(options_info,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(options_info,text);
}

static void options_add_item(struct option_info*k){
	// button
	k->btn=lv_btn_create(lst);
	lv_obj_set_size(k->btn,lv_pct(100),LV_SIZE_CONTENT);
	lv_obj_set_style_min_height(k->btn,gui_font_size,0);
	lv_style_set_btn_item(k->btn);
	lv_obj_add_event_cb(k->btn,option_click,LV_EVENT_CLICKED,k);
	lv_group_add_obj(gui_grp,k->btn);
	lv_obj_update_layout(k->btn);

	// option name
	k->lbl=lv_label_create(k->btn);
	lv_label_set_long_mode(k->lbl,confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SCROLL_CIRCULAR:
		LV_LABEL_LONG_DOT
	);
	lv_label_set_text(k->lbl,k->name);
}

static void bootmenu_option_reload(){
	bootmenu_option_clear();
	UINTN cnt=0,i=0;
	EFI_BOOT_MANAGER_LOAD_OPTION*bo;
	EfiBootManagerRefreshAllBootOption();
	if(!(bo=EfiBootManagerGetLoadOptions(&cnt,LoadOptionTypeBoot)))
		bootmenu_set_options_info(_("Load options failed"));
	else if(cnt<=0)
		bootmenu_set_options_info(_("No options found"));
	else for(i=0;i<cnt;i++){
		struct option_info*k=&options[i];
		EFI_BOOT_MANAGER_LOAD_OPTION*b=&bo[i];
		UnicodeStrToAsciiStrS(b->Description,k->name,sizeof(k->name));
		tlog_debug("option %llu: %s",(unsigned long long)i+1,k->name);
		k->enable=true,k->option=b;
		options_add_item(k);
		if(i>=256){
			tlog_warn("options too many, only show 256 options");
			break;
		}
	}
	tlog_info("found %llu options",(unsigned long long)i);
}

static void refresh_click(lv_event_t*e){
	if(e->target!=btn_refresh)return;
	tlog_debug("request refresh");
	bootmenu_option_reload();
}

static int after_exit(void*d __attribute__((unused))){
	if(!cur)return -1;
	EfiBootManagerBoot(cur);
	return 0;
}

static void ok_click(lv_event_t*e){
	if(e->target!=btn_ok||!selected)return;
	tlog_notice("shutdown guiapp and start option %s",selected->name);
	cur=selected->option;
	gui_run_and_exit(after_exit);
}

static int do_cleanup(struct gui_activity*d __attribute__((unused))){
	bootmenu_option_clear();
	return 0;
}

static int do_load(struct gui_activity*d __attribute__((unused))){
	bootmenu_option_reload();
	return 0;
}

static int bootmenu_option_get_focus(struct gui_activity*d __attribute__((unused))){
	lv_group_add_obj(gui_grp,btn_ok);
	lv_group_add_obj(gui_grp,btn_refresh);
	return 0;
}

static int bootmenu_option_lost_focus(struct gui_activity*d __attribute__((unused))){
	for(int i=0;i<256;i++){
		if(!options[i].enable)continue;
		lv_group_remove_obj(options[i].btn);
	}
	lv_group_remove_obj(btn_ok);
	lv_group_remove_obj(btn_refresh);
	return 0;
}

static int uefi_bootmenu_draw(struct gui_activity*act){
	lv_obj_set_flex_flow(act->page,LV_FLEX_FLOW_COLUMN);

	// function title
	lv_obj_t*title=lv_label_create(act->page);
	lv_obj_set_width(title,lv_pct(100));
	lv_label_set_long_mode(title,LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_align(title,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(title,_("UEFI Boot Menu"));

	// options list
	lst=lv_obj_create(act->page);
	lv_obj_set_flex_grow(lst,1);
	lv_obj_set_flex_flow(lst,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_width(lst,lv_pct(100));

	lv_draw_buttons_auto_arg(
		act->page,
		#define BTN(tgt,en,title,cb,x)&(struct button_dsc){\
			&tgt,en,_(title),cb,NULL,x,1,0,1,NULL\
		}
		BTN(btn_ok,      false, "OK",      ok_click,      0),
		BTN(btn_refresh, true, "Refresh", refresh_click, 1),
		#undef BTN
		NULL
	);

	return 0;
}

struct gui_register guireg_uefi_bootmenu={
	.name="uefi-bootmenu",
	.title="UEFI Boot Menu",
	.show_app=true,
	.draw=uefi_bootmenu_draw,
	.quiet_exit=do_cleanup,
	.get_focus=bootmenu_option_get_focus,
	.lost_focus=bootmenu_option_lost_focus,
	.data_load=do_load,
	.back=true,
};
#endif
#endif
