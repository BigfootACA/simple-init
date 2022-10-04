/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"bootmgr.h"

static int bootdata_memory_region_get_focus(struct gui_activity*d){
	struct bootdata_memory_region*bi=d->data;
	if(!bi)return 0;
	lv_group_add_obj(gui_grp,bi->txt_reg_base);
	lv_group_add_obj(gui_grp,bi->txt_reg_size);
	lv_group_add_obj(gui_grp,bi->txt_reg_end);
	lv_group_add_obj(gui_grp,bi->ok);
	lv_group_add_obj(gui_grp,bi->cancel);
	(void)bootmgr_base;
	return 0;
}

static int bootdata_memory_region_lost_focus(struct gui_activity*d){
	struct bootdata_memory_region*bi=d->data;
	if(!bi)return 0;
	lv_group_remove_obj(bi->txt_reg_base);
	lv_group_remove_obj(bi->txt_reg_size);
	lv_group_remove_obj(bi->txt_reg_end);
	lv_group_remove_obj(bi->ok);
	lv_group_remove_obj(bi->cancel);
	return 0;
}

static bool do_save(struct bootdata_memory_region*bi){
	char*es=NULL;
	if(!bi->path[0])return false;
	const char*base=lv_textarea_get_text(bi->txt_reg_base);
	const char*size=lv_textarea_get_text(bi->txt_reg_size);
	if(!base||!size)return false;
	errno=0;
	long long ab=strtoll(base,&es,16);
	if(errno!=0||es==base||ab<=0){
		msgbox_alert("Invalid memory address or size");
		return false;
	}
	long long as=strtoll(size,&es,16);
	if(errno!=0||es==size||as<=0){
		msgbox_alert("Invalid memory address or size");
		return false;
	}
	confd_delete_base(bi->path,"start");
	confd_delete_base(bi->path,"base");
	confd_delete_base(bi->path,"size");
	confd_delete_base(bi->path,"end");
	confd_set_integer_base(bi->path,"base",ab);
	confd_set_integer_base(bi->path,"size",as);
	bi->act->data_changed=true;
	return true;
}

static int init(struct gui_activity*act){
	if(!act->args)ERET(EINVAL);
	struct bootdata_memory_region*bi=malloc(sizeof(struct bootdata_memory_region));
	if(!bi)return -ENOMEM;
	memset(bi,0,sizeof(struct bootdata_memory_region));
	act->data=bi,bi->act=act;
	return 0;
}

static int do_load(struct gui_activity*act){
	char buff[64];
	struct bootdata_memory_region*bi=act->data;
	if(!bi||!act->args)return 0;
	strncpy(bi->path,act->args,sizeof(bi->path)-1);
	int64_t base=confd_get_integer_base(bi->path,"base",-1);
	int64_t start=confd_get_integer_base(bi->path,"start",-1);
	int64_t size=confd_get_integer_base(bi->path,"size",-1);
	int64_t end=confd_get_integer_base(bi->path,"end",-1);
	if(start>0){
		if(base>0&&start!=base)tlog_warn("base and start mismatched");
		base=start;
	}
	if(end>0){
		if(size>0&&end!=base+size)tlog_warn("size and end mismatched");
		size=end-base;
	}
	if(base<0)base=0;
	if(size<0)size=0;
	memset(buff,0,sizeof(buff));
	snprintf(buff,sizeof(buff)-1,"0x%llX",(unsigned long long)base);
	lv_textarea_set_text(bi->txt_reg_base,buff);
	memset(buff,0,sizeof(buff));
	snprintf(buff,sizeof(buff)-1,"0x%llX",(unsigned long long)size);
	lv_textarea_set_text(bi->txt_reg_size,buff);
	memset(buff,0,sizeof(buff));
	snprintf(buff,sizeof(buff)-1,"0x%llX",(unsigned long long)base+size);
	lv_textarea_set_text(bi->txt_reg_end,buff);
	act->args=NULL;
	return 0;
}

static int do_cleanup(struct gui_activity*act){
	struct bootdata_memory_region*bi=act->data;
	if(!bi)return 0;
	memset(bi,0,sizeof(struct bootdata_memory_region));
	free(bi);
	act->data=NULL;
	return 0;
}

static void input_cb(lv_event_t*e){
	long long sb=0,ss=0,se=0;
	struct gui_activity*act=e->user_data;
	struct bootdata_memory_region*bi=act->data;
	char*es=NULL,nb[64],ns[64],ne[64];
	const char*base=lv_textarea_get_text(bi->txt_reg_base);
	const char*size=lv_textarea_get_text(bi->txt_reg_size);
	const char*end=lv_textarea_get_text(bi->txt_reg_end);
	if(!base||!size||!end)return;
	errno=0;
	long long ab=strtoll(base,&es,16);
	if(errno!=0||es==base||ab<=0)return;
	long long as=strtoll(size,&es,16);
	if(errno!=0||es==size||as<=0)return;
	long long ae=strtoll(end,&es,16);
	if(errno!=0||es==end||ae<=0)return;
	memset(nb,0,sizeof(nb));
	memset(ns,0,sizeof(ns));
	memset(ne,0,sizeof(ne));
	if(e->target==bi->txt_reg_end)ss=MAX(0,ae-ab);
	else if(e->target==bi->txt_reg_size)se=ab+as;
	else if(e->target==bi->txt_reg_base)se=ab+as;
	if(se!=0)snprintf(ne,sizeof(ne)-1,"0x%llX",se);
	if(ss!=0)snprintf(ns,sizeof(ns)-1,"0x%llX",ss);
	if(sb!=0)snprintf(nb,sizeof(nb)-1,"0x%llX",sb);
	if(ne[0])lv_textarea_set_text(bi->txt_reg_end,ne);
	if(ns[0])lv_textarea_set_text(bi->txt_reg_size,ns);
	if(nb[0])lv_textarea_set_text(bi->txt_reg_base,nb);
}

static void ok_cb(lv_event_t*e){
	if(do_save(e->user_data))guiact_do_back();
}

static int draw_bootdata_memory_region(struct gui_activity*act){
	struct bootdata_memory_region*bi=act->data;
	bi->box=lv_draw_dialog_box(act->page,&bi->title,"Edit Memory Region");
	lv_draw_input(bi->box,"Memory Start Address (Hex):", NULL,NULL,&bi->txt_reg_base, NULL);
	lv_draw_input(bi->box,"Memory Size (Hex):",          NULL,NULL,&bi->txt_reg_size, NULL);
	lv_draw_input(bi->box,"Memory End Address (Hex):",   NULL,NULL,&bi->txt_reg_end,  NULL);
	lv_textarea_set_accepted_chars(bi->txt_reg_base,HEX"x");
	lv_textarea_set_accepted_chars(bi->txt_reg_size,HEX"x");
	lv_textarea_set_accepted_chars(bi->txt_reg_end,HEX"x");
	lv_obj_add_event_cb(bi->txt_reg_base,input_cb,LV_EVENT_DEFOCUSED,act);
	lv_obj_add_event_cb(bi->txt_reg_size,input_cb,LV_EVENT_DEFOCUSED,act);
	lv_obj_add_event_cb(bi->txt_reg_end,input_cb,LV_EVENT_DEFOCUSED,act);
	lv_draw_btns_ok_cancel(bi->box,&bi->ok,&bi->cancel,ok_cb,bi);
	return 0;
}

struct gui_register guireg_bootdata_memreg={
	.name="bootdata-memory-region",
	.title="Boot Item Extra Data Editor",
	.show_app=false,
	.quiet_exit=do_cleanup,
	.init=init,
	.get_focus=bootdata_memory_region_get_focus,
	.lost_focus=bootdata_memory_region_lost_focus,
	.draw=draw_bootdata_memory_region,
	.data_load=do_load,
	.back=true,
	.mask=true,
};
#endif
