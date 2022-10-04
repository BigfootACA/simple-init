/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"bootmgr.h"

static int bootdata_efi_get_focus(struct gui_activity*d){
	struct bootdata_efi*bi=d->data;
	if(!bi)return 0;
	lv_group_add_obj(gui_grp,bi->clr_path);
	lv_group_add_obj(gui_grp,bi->txt_path);
	lv_group_add_obj(gui_grp,bi->clr_guid);
	lv_group_add_obj(gui_grp,bi->txt_guid);
	lv_group_add_obj(gui_grp,bi->clr_options);
	lv_group_add_obj(gui_grp,bi->txt_options);
	lv_group_add_obj(gui_grp,bi->chk_char16);
	lv_group_add_obj(gui_grp,bi->ok);
	lv_group_add_obj(gui_grp,bi->cancel);
	return 0;
}

static int bootdata_efi_lost_focus(struct gui_activity*d){
	struct bootdata_efi*bi=d->data;
	if(!bi)return 0;
	lv_group_remove_obj(bi->clr_path);
	lv_group_remove_obj(bi->txt_path);
	lv_group_remove_obj(bi->clr_guid);
	lv_group_remove_obj(bi->txt_guid);
	lv_group_remove_obj(bi->clr_options);
	lv_group_remove_obj(bi->txt_options);
	lv_group_remove_obj(bi->chk_char16);
	lv_group_remove_obj(bi->ok);
	lv_group_remove_obj(bi->cancel);
	return 0;
}

static bool do_save(struct bootdata_efi*bi){
	if(!bi->name[0])return false;
	const char*efi=lv_textarea_get_text(bi->txt_path);
	const char*guid=lv_textarea_get_text(bi->txt_guid);
	const char*opt=lv_textarea_get_text(bi->txt_options);
	bool w_char=lv_obj_has_flag(bi->chk_char16,LV_STATE_CHECKED);
	if(!efi||!guid||!opt)return false;
	if(efi[0]&&guid[0]){
		msgbox_alert("Conflict options");
		return false;
	}
	if(!efi[0])confd_delete_dict(bootmgr_base,bi->name,"extra.efi_path");
	else confd_set_string_dict(bootmgr_base,bi->name,"extra.efi_path",(char*)efi);
	if(!guid[0])confd_delete_dict(bootmgr_base,bi->name,"extra.efi_fv_guid");
	else confd_set_string_dict(bootmgr_base,bi->name,"extra.efi_fv_guid",(char*)guid);
	if(!opt[0]){
		confd_delete_dict(bootmgr_base,bi->name,"extra.options");
		confd_delete_dict(bootmgr_base,bi->name,"extra.options_widechar");
	}else{
		confd_set_string_dict(bootmgr_base,bi->name,"extra.options",(char*)opt);
		confd_set_boolean_dict(bootmgr_base,bi->name,"extra.options_widechar",w_char);
	}
	bi->act->data_changed=true;
	return true;
}

static int init(struct gui_activity*act){
	if(!act->args)ERET(EINVAL);
	struct bootdata_efi*bi=malloc(sizeof(struct bootdata_efi));
	if(!bi)return -ENOMEM;
	memset(bi,0,sizeof(struct bootdata_efi));
	act->data=bi,bi->act=act;
	return 0;
}

static int do_load(struct gui_activity*act){
	struct bootdata_efi*bi=act->data;
	if(!bi)return 0;
	struct boot_config*c=NULL;
	if(act->args)c=boot_get_config(act->args),act->args=NULL;
	if(c){
		char*efi=confd_get_string_base(c->key,"efi_file",NULL);
		char*guid=confd_get_string_base(c->key,"efi_fv_guid",NULL);
		char*opt=confd_get_string_base(c->key,"options",NULL);
		bool char16=confd_get_boolean_base(c->key,"options_widechar",false);
		lv_textarea_set_text(bi->txt_path,efi?efi:"");
		lv_textarea_set_text(bi->txt_guid,guid?guid:"");
		lv_textarea_set_text(bi->txt_options,opt?opt:"");
		lv_obj_set_checked(bi->chk_char16,char16);
		strncpy(bi->name,c->ident,sizeof(bi->name)-1);
	}
	if(c)free(c);
	if(!bi->name[0])ERET(EINVAL);
	return 0;
}

static int do_cleanup(struct gui_activity*act){
	struct bootdata_efi*bi=act->data;
	if(!bi)return 0;
	memset(bi,0,sizeof(struct bootdata_efi));
	free(bi);
	act->data=NULL;
	return 0;
}

static void ok_cb(lv_event_t*e){
	if(do_save(e->user_data))guiact_do_back();
}

static int draw_bootdata_efi(struct gui_activity*act){
	struct bootdata_efi*bi=act->data;
	bi->box=lv_draw_dialog_box(act->page,&bi->title,"Edit EFI Boot Item");
	lv_draw_input(bi->box, "EFI File Path:",            NULL, &bi->clr_path,    &bi->txt_path,    NULL);
	lv_draw_input(bi->box, "EFI Firmware Volume Guid:", NULL, &bi->clr_guid,    &bi->txt_guid,    NULL);
	lv_draw_input(bi->box, "EFI Load Options:",         NULL, &bi->clr_options, &bi->txt_options, NULL);
	lv_textarea_set_accepted_chars(bi->txt_guid,HEX"{-}");
	bi->chk_char16=lv_draw_checkbox(bi->box,_("Use options as wide char"),false,NULL,NULL);
	lv_draw_btns_ok_cancel(bi->box,&bi->ok,&bi->cancel,ok_cb,bi);
	return 0;
}

struct gui_register guireg_bootdata_efi={
	.name="bootdata-efi",
	.title="Boot Item Extra Data Editor",
	.show_app=false,
	.quiet_exit=do_cleanup,
	.init=init,
	.get_focus=bootdata_efi_get_focus,
	.lost_focus=bootdata_efi_lost_focus,
	.draw=draw_bootdata_efi,
	.data_load=do_load,
	.back=true,
	.mask=true,
};
#endif
