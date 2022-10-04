/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"bootmgr.h"
#ifdef ENABLE_UEFI
#include<Library/PrintLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/UefiBootManagerLib.h>
#endif

static int bootdata_uefi_option_get_focus(struct gui_activity*d){
	struct bootdata_uefi_option*bi=d->data;
	if(!bi)return 0;
	lv_group_add_obj(gui_grp,bi->txt_opt);
	#ifdef ENABLE_UEFI
	lv_group_add_obj(gui_grp,bi->sel_opt);
	#endif
	lv_group_add_obj(gui_grp,bi->ok);
	lv_group_add_obj(gui_grp,bi->cancel);
	return 0;
}

static int bootdata_uefi_option_lost_focus(struct gui_activity*d){
	struct bootdata_uefi_option*bi=d->data;
	if(!bi)return 0;
	lv_group_remove_obj(bi->txt_opt);
	#ifdef ENABLE_UEFI
	lv_group_add_obj(gui_grp,bi->sel_opt);
	#endif
	lv_group_remove_obj(bi->ok);
	lv_group_remove_obj(bi->cancel);
	return 0;
}

static bool do_save(struct bootdata_uefi_option*bi){
	if(!bi->name[0])return false;
	char*end=NULL;
	const char*opt=lv_textarea_get_text(bi->txt_opt);
	if(!opt)return false;
	errno=0;
	uint option=(uint)strtol(opt,&end,16);
	if(errno!=0||end==opt||option>0xFFFF){
		msgbox_alert("Invalid number");
		return true;
	}
	confd_set_integer_dict(
		bootmgr_base,bi->name,
		"extra.option",
		(int64_t)option
	);
	bi->act->data_changed=true;
	return true;
}

static int init(struct gui_activity*act){
	if(!act->args)ERET(EINVAL);
	struct bootdata_uefi_option*bi=malloc(sizeof(struct bootdata_uefi_option));
	if(!bi)return -ENOMEM;
	memset(bi,0,sizeof(struct bootdata_uefi_option));
	act->data=bi,bi->act=act;
	return 0;
}

#ifdef ENABLE_UEFI
static void load_uefi_options(struct bootdata_uefi_option*bi,uint opt){
	uint pos=0;
	UINTN cnt=0;
	EFI_BOOT_MANAGER_LOAD_OPTION*bo;
	char name[BUFSIZ];
	ZeroMem(bi->options,sizeof(bi->options));
	lv_dropdown_clear_options(bi->sel_opt);
	lv_dropdown_add_option(bi->sel_opt,_("(none)"),0);
	EfiBootManagerRefreshAllBootOption();
	if((bo=EfiBootManagerGetLoadOptions(
		&cnt,LoadOptionTypeBoot
	))&&cnt>0)for(UINTN i=0;i<cnt;i++){
		ZeroMem(name,sizeof(name));
		AsciiSPrint(
			name,sizeof(name)-1,
			"%s (%04X)",
			bo[i].Description,
			bo[i].OptionNumber
		);
		pos++;
		bi->options[pos]=(uint)bo[i].OptionNumber;
		lv_dropdown_add_option(bi->sel_opt,name,pos);
		if(opt==bi->options[pos])
			lv_dropdown_set_selected(bi->sel_opt,pos);
		if(pos>=(uint)ARRLEN(bi->options)-1)break;
	}
	tlog_info("found %u options",(uint)pos);
}

static void option_cb(lv_event_t*e){
	char option[8];
	struct bootdata_uefi_option*bi=e->user_data;
	uint16_t o=lv_dropdown_get_selected(bi->sel_opt);
	memset(option,0,sizeof(option));
	if(o>0)snprintf(option,5,"%04X",bi->options[o]);
	lv_textarea_set_text(bi->txt_opt,option);
}

static void input_cb(lv_event_t*e){
	char*end=NULL;
	struct bootdata_uefi_option*bi=e->user_data;
	const char*option=lv_textarea_get_text(bi->txt_opt);
	if(!option)return;
	errno=0;
	uint pos=0;
	uint opt=(uint)strtol(option,&end,16);
	if(errno!=0||end==option||opt>0xFFFF)return;
	for(uint i=0;i<ARRLEN(bi->options);i++){
		if(opt==bi->options[i]){
			pos=i;
			break;
		}
	}
	if(pos==0||pos>lv_dropdown_get_option_cnt(bi->sel_opt))return;
	lv_dropdown_set_selected(bi->sel_opt,pos);
}
#endif

static int do_load(struct gui_activity*act){
	struct bootdata_uefi_option*bi=act->data;
	if(!bi)return 0;
	struct boot_config*c=NULL;
	if(act->args)c=boot_get_config(act->args),act->args=NULL;
	if(c){
		char option[8];
		uint opt=confd_get_integer_base(c->key,"option",0x10000);
		memset(option,0,sizeof(option));
		if(opt<=0xFFFF)snprintf(option,5,"%04X",opt);
		lv_textarea_set_text(bi->txt_opt,option);
		#ifdef ENABLE_UEFI
		load_uefi_options(bi,opt);
		#endif
		strncpy(bi->name,c->ident,sizeof(bi->name)-1);
	}
	if(c)free(c);
	if(!bi->name[0])ERET(EINVAL);
	return 0;
}

static int do_cleanup(struct gui_activity*act){
	struct bootdata_uefi_option*bi=act->data;
	if(!bi)return 0;
	memset(bi,0,sizeof(struct bootdata_uefi_option));
	free(bi);
	act->data=NULL;
	return 0;
}

static void ok_cb(lv_event_t*e){
	if(do_save(e->user_data))guiact_do_back();
}

static int draw_bootdata_uefi_option(struct gui_activity*act){
	struct bootdata_uefi_option*bi=act->data;
	bi->box=lv_draw_dialog_box(act->page,&bi->title,"Edit UEFI Option Boot Item");
	lv_draw_input(bi->box,"UEFI Option:", NULL,NULL,&bi->txt_opt,NULL);
	lv_textarea_set_accepted_chars(bi->txt_opt,HEX);

	#ifdef ENABLE_UEFI
	lv_obj_add_event_cb(bi->txt_opt,input_cb,LV_EVENT_DEFOCUSED,bi);
	bi->sel_opt=lv_dropdown_create(bi->box);
	lv_obj_set_width(bi->sel_opt,lv_pct(100));
	lv_obj_set_user_data(bi->sel_opt,bi);
	lv_obj_add_event_cb(bi->sel_opt,lv_default_dropdown_cb,LV_EVENT_ALL,NULL);
	lv_obj_add_event_cb(bi->sel_opt,option_cb,LV_EVENT_VALUE_CHANGED,bi);
	#endif

	lv_draw_btns_ok_cancel(bi->box,&bi->ok,&bi->cancel,ok_cb,bi);
	return 0;
}

struct gui_register guireg_bootdata_uefi_option={
	.name="bootdata-uefi-option",
	.title="Boot Item Extra Data Editor",
	.show_app=false,
	.quiet_exit=do_cleanup,
	.init=init,
	.get_focus=bootdata_uefi_option_get_focus,
	.lost_focus=bootdata_uefi_option_lost_focus,
	.draw=draw_bootdata_uefi_option,
	.data_load=do_load,
	.back=true,
	.mask=true,
};
#endif
