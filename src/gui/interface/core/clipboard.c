/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include<time.h>
#include"gui.h"
#include"list.h"
#include"confd.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/inputbox.h"
#include"gui/activity.h"
#include"gui/clipboard.h"
#define TAG "clipboard"
#define BASE "gui.clipboard"
#define BASE_HIST BASE".history"

static enum clipboard_type clip_type=CLIP_NULL;
static char*clip_cont=NULL;
struct clipboard_item;
struct clipboard_app{
	lv_obj_t*list,*info,*btns;
	lv_obj_t*add,*reload,*clear;
	lv_obj_t*use,*delete,*edit;
	list*items;
	struct clipboard_item*selected;
};
struct clipboard_item{
	list*item;
	lv_obj_t*btn,*lbl;
	char key[256];
	struct clipboard_app*clip;
};

void clipboard_reset(void){
	confd_delete_base(BASE,"last");
	if(clip_cont)free(clip_cont);
	clip_type=CLIP_NULL,clip_cont=NULL;
	tlog_debug("reset clipboard");
}

void clipboard_clear(void){
	clipboard_reset();
	confd_delete(BASE);
}

void clipboard_load(const char*key){
	if(!key)return;
	clip_type=confd_get_integer_dict(BASE_HIST,key,"type",CLIP_NULL);
	clip_cont=confd_get_string_dict(BASE_HIST,key,"content",NULL);
	if(!clip_cont)clip_type=CLIP_NULL;
}

void clipboard_init(void){
	char*last=confd_get_string_base(BASE,"last",NULL);
	if(!last)return;
	clipboard_load(last);
	if(clip_type!=CLIP_NULL)tlog_debug(
		"restore clipboard type %d, content %s",
		clip_type,clip_cont
	);
	free(last);
}

int clipboard_set(enum clipboard_type type,const char*content,size_t len){
	if(type==CLIP_NULL){
		if(content)return -EINVAL;
		clipboard_reset();
		return 0;
	}
	if(!content)return -EINVAL;
	char*cont=len==0?strdup(content):strndup(content,len);
	if(!cont)return -ENOMEM;
	if(clip_cont){
		if(
			clip_type==type&&
			strcmp(clip_cont,cont)==0
		)return 0;
		free(clip_cont);
	}
	int c=1;
	char buf[255];
	time_t t;
	clip_type=type,clip_cont=cont;
	tlog_debug("set clipboard to type %d, content %s",type,cont);
	time(&t);
	for(;;){
		memset(buf,0,sizeof(buf));
		snprintf(buf,sizeof(buf)-1,"%ld%04d",(long int)t,c);
		if(confd_get_type_base(BASE_HIST,buf)!=TYPE_KEY)break;
		if(++c>=10000)return -EAGAIN;
	}
	confd_set_integer_dict(BASE_HIST,buf,"type",type);
	confd_set_string_dict(BASE_HIST,buf,"content",cont);
	confd_set_string_base(BASE,"last",buf);
	return 0;
}

enum clipboard_type clipboard_get_type(void){
	return clip_type;
}

char*clipboard_get_content(void){
	return clip_cont;
}

static void clipboard_item_click(lv_event_t*e){
	struct clipboard_item*item=e->user_data;
	if(!item||!item->clip)return;
	bool found=false;
	list*f=list_first(item->clip->items);
	if(f)do{
		LIST_DATA_DECLARE(i,f,struct clipboard_item*);
		if(i==item){
			found=true;
			continue;
		}
		lv_obj_set_checked(i->btn,false);
	}while((f=f->next));
	if(!found)return;
	lv_obj_set_checked(item->btn,true);
	lv_obj_set_enabled(item->clip->use,true);
	lv_obj_set_enabled(item->clip->edit,true);
	lv_obj_set_enabled(item->clip->delete,true);
	item->clip->selected=item;
}

static void clipboard_add_item(struct clipboard_app*clip,char*key){
	char*str=confd_get_string_dict(BASE_HIST,key,"content",NULL);
	if(!str)return;
	struct clipboard_item*i=malloc(sizeof(struct clipboard_item));
	if(!i){
		free(str);
		return;
	}
	memset(i,0,sizeof(struct clipboard_item));
	strncpy(i->key,key,sizeof(i->key)-1);
	i->clip=clip;

	i->btn=lv_btn_create(i->clip->list);
	lv_obj_set_size(i->btn,lv_pct(100),LV_SIZE_CONTENT);
	lv_style_set_btn_item(i->btn);
	lv_obj_add_event_cb(i->btn,clipboard_item_click,LV_EVENT_CLICKED,i);
	lv_group_add_obj(gui_grp,i->btn);

	i->lbl=lv_label_create(i->btn);
	lv_obj_set_width(i->lbl,lv_pct(100));
	lv_label_set_text(i->lbl,str);

	i->item=list_new_notnull(i);
	list_obj_add(&clip->items,i->item);
	free(str);
}

static void clipboard_reload(struct clipboard_app*clip){
	if(clip->info)lv_obj_del(clip->info);
	list_free_all_def(clip->items);
	clip->items=NULL,clip->selected=NULL,clip->info=NULL;
	lv_obj_set_enabled(clip->use,false);
	lv_obj_set_enabled(clip->edit,false);
	lv_obj_set_enabled(clip->delete,false);
	lv_obj_clean(clip->list);
	bool found=false;
	char**cs=confd_ls(BASE_HIST);
	if(cs){
		for(size_t s=0;cs[s];s++){
			found=true;
			clipboard_add_item(clip,cs[s]);
		}
		if(cs[0])free(cs[0]);
		free(cs);
	}
	if(!found){
		clip->info=lv_label_create(clip->list);
		lv_label_set_long_mode(clip->info,LV_LABEL_LONG_WRAP);
		lv_obj_set_size(clip->info,lv_pct(100),LV_SIZE_CONTENT);
		lv_obj_set_style_text_align(clip->info,LV_TEXT_ALIGN_CENTER,0);
		lv_label_set_text(clip->info,_("(none)"));
	}
}

static bool input_cb(bool ok,const char*content,void*user_data __attribute__((unused))){
	if(ok)clipboard_set(CLIP_TEXT,content,0);
	return false;
}

static void clipboard_click(lv_event_t*e){
	struct clipboard_app*clip=e->user_data;
	if(!clip)return;
	struct clipboard_item*sel=clip->selected;
	if(e->target==clip->add){
		struct inputbox*in=inputbox_create(input_cb,"Add into clipboard");
		inputbox_set_one_line(in,false);
	}else if(e->target==clip->reload){
		clipboard_reload(clip);
	}else if(e->target==clip->clear){
		clipboard_clear();
		clipboard_reload(clip);
	}else if(e->target==clip->use){
		if(!sel)return;
		confd_set_string_base(BASE,"last",sel->key);
		clipboard_load(sel->key);
	}else if(e->target==clip->delete){
		if(!sel)return;
		char*last=confd_get_string_base(BASE,"last",NULL);
		if(last){
			if(strcmp(sel->key,last)==0){
				list*l=sel->item;
				struct clipboard_item*la=NULL;
				if(l->prev)la=LIST_DATA(l->prev,struct clipboard_item*);
				if(l->next)la=LIST_DATA(l->next,struct clipboard_item*);
				if(la){
					confd_set_string_base(BASE,"last",la->key);
					clipboard_load(la->key);
				}
			}
			free(last);
		}
		confd_delete_base(BASE_HIST,sel->key);
		clipboard_reload(clip);
	}else if(e->target==clip->edit){
		if(!sel)return;
		struct inputbox*in=inputbox_create(input_cb,"Edit clipboard item");
		inputbox_set_one_line(in,false);
		char*str=confd_get_string_dict(BASE_HIST,sel->key,"content",NULL);
		if(str){
			inputbox_set_content(in,"%s",str);
			free(str);
		}
	}
}

static int clipboard_get_focus(struct gui_activity*act){
	struct clipboard_app*clip=act->data;
	if(!clip)return 0;
	clipboard_reload(clip);
	lv_group_add_obj(gui_grp,clip->add);
	lv_group_add_obj(gui_grp,clip->reload);
	lv_group_add_obj(gui_grp,clip->clear);
	lv_group_add_obj(gui_grp,clip->use);
	lv_group_add_obj(gui_grp,clip->delete);
	lv_group_add_obj(gui_grp,clip->edit);
	return 0;
}

static int clipboard_lost_focus(struct gui_activity*act){
	struct clipboard_app*clip=act->data;
	if(!clip)return 0;
	lv_group_remove_obj(clip->add);
	lv_group_remove_obj(clip->reload);
	lv_group_remove_obj(clip->clear);
	lv_group_remove_obj(clip->use);
	lv_group_remove_obj(clip->delete);
	lv_group_remove_obj(clip->edit);
	return 0;
}

static int clipboard_exit(struct gui_activity*act){
	if(act->data)free(act->data);
	act->data=NULL;
	return 0;
}

static int clipboard_do_init(struct gui_activity*act){
	static size_t s=sizeof(struct clipboard_app);
	struct clipboard_app*clip;
	if(!(clip=malloc(s)))return -ENOMEM;
	memset(clip,0,s);
	act->data=clip;
	return 0;
}

static int clipboard_draw(struct gui_activity*act){
	struct clipboard_app*clip=act->data;
	lv_obj_set_flex_flow(act->page,LV_FLEX_FLOW_COLUMN);

	// function title
	lv_obj_t*title=lv_label_create(act->page);
	lv_label_set_long_mode(title,LV_LABEL_LONG_WRAP);
	lv_obj_set_size(title,lv_pct(100),LV_SIZE_CONTENT);
	lv_obj_set_style_text_align(title,LV_TEXT_ALIGN_CENTER,0);
	lv_label_set_text(title,_("Clipboard"));

	// options list
	clip->list=lv_obj_create(act->page);
	lv_obj_set_width(clip->list,lv_pct(100));
	lv_obj_set_flex_flow(clip->list,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_grow(clip->list,1);

	clip->btns=lv_draw_buttons_auto_arg(
		act->page,
		#define BTN(tgt,title,x,y)&(struct button_dsc){\
			&tgt,true,_(title),\
			clipboard_click,\
			clip,x,1,y,1,NULL\
		}
		BTN(clip->add,    "Add",    0,0),
		BTN(clip->reload, "Reload", 1,0),
		BTN(clip->clear,  "Clear",  2,0),
		BTN(clip->use,    "Use",    0,1),
		BTN(clip->delete, "Delete", 1,1),
		BTN(clip->edit,   "Edit",   2,1),
		#undef BTN
		NULL
	);

	clipboard_reload(clip);
	return 0;
}

struct gui_register guireg_clipboard={
	.name="clipboard",
	.title="Clipboard",
	.show_app=true,
	.init=clipboard_do_init,
	.quiet_exit=clipboard_exit,
	.get_focus=clipboard_get_focus,
	.lost_focus=clipboard_lost_focus,
	.draw=clipboard_draw,
	.back=true,
};

#endif
