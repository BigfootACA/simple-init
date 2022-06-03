/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_HIVEX
#define _GNU_SOURCE
#include<hivex.h>
#include<stdlib.h>
#include<string.h>
#include"gui.h"
#include"list.h"
#include"confd.h"
#include"logger.h"
#include"regedit.h"
#include"gui/tools.h"
#include"gui/fsext.h"
#include"gui/msgbox.h"
#include"gui/activity.h"
#include"gui/inputbox.h"
#include"gui/filepicker.h"
#define TAG "regedit"
#define MIME_EXT ".svg"

struct reg_item{
	struct regedit*reg;
	bool parent;
	char name[255];
	hive_node_h node;
	hive_value_h dir;
	hive_value_h value;
	hive_type type;
	lv_obj_t*btn,*chk,*img,*val,*xtype;
};

static char*get_string_path(struct regedit*reg,char*sub){
	if(!reg)return NULL;
	static char string[BUFSIZ],*p,*s,*e;
	if(!reg->path)return sub?sub:strcpy(string,"\\");
	memset(string,0,sizeof(string));
	p=string,s=p,e=p+BUFSIZ-1;
	*p++='\\';
	list*o=list_first(reg->path);
	if(o)do{
		if(p!=s&&*(p-1)!='\\')*p++='\\';
		LIST_DATA_DECLARE(item,o,char*);
		strncpy(p,item,e-p-1);
		p+=strlen(item);
	}while((o=o->next));
	if(sub){
		if(p!=s&&*(p-1)!='\\')*p++='\\';
		strncpy(p,sub,e-p-1);
	}
	return string;
}

static const char*get_icon(struct reg_item*ci){
	if(!ci)return NULL;
	if(ci->parent)return "inode-parent"MIME_EXT;
	switch(ci->type){
		case hive_t_REG_NONE:return "inode-dir"MIME_EXT;
		default:return "text-x-plain"MIME_EXT;
	}
	return "unknown"MIME_EXT;
}

static int clean_item(void*d){
	struct reg_item*i=(struct reg_item*)d;
	if(i){
		if(i->btn)lv_obj_del(i->btn);
		free(i);
	}
	return 0;
}

static void clean_view(struct regedit*reg){
	if(!reg)return;
	if(reg->info)lv_obj_del(reg->info);
	list_free_all(reg->items,clean_item);
	reg->items=NULL,reg->info=NULL,reg->last_btn=NULL;
	lv_obj_set_enabled(reg->btn_delete,false);
	lv_obj_set_enabled(reg->btn_edit,false);
}

static void set_info(struct regedit*reg,char*text){
	if(!reg||!text)return;
	clean_view(reg);
	if(reg->info)lv_obj_del(reg->info);
	reg->info=lv_label_create(reg->view,NULL);
	lv_label_set_long_mode(reg->info,LV_LABEL_LONG_BREAK);
	lv_obj_set_width(reg->info,lv_page_get_scrl_width(reg->view)-gui_font_size);
	lv_label_set_align(reg->info,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(reg->info,text);
}

static void load_view(struct regedit*reg);
static void go_back(struct regedit*reg){
	if(!reg)return;
	hive_node_h par=hivex_node_parent(reg->hive,reg->node);
	if(par<=0)return;
	reg->node=par;
	list_obj_del(&reg->path,list_last(reg->path),list_default_free);
	load_view(reg);
}

static void click_item(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	struct reg_item*ci=(struct reg_item*)lv_obj_get_user_data(obj);
	if(!ci||!ci->reg)return;
	if(ci->type==hive_t_REG_NONE){
		if(ci->parent)go_back(ci->reg);
		else if(ci->node>0){
			list_obj_add_new_strdup(&ci->reg->path,ci->name);
			ci->reg->node=ci->node;
			load_view(ci->reg);
		}
	}else{
		static struct regedit_value value;
		value.hive=ci->reg->hive;
		value.value=ci->value;
		value.node=ci->dir;
		value.reg=ci->reg;
		guiact_start_activity(
			&guireg_regedit_value,&value
		);
	}
}

static void check_item(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_VALUE_CHANGED)return;
	struct reg_item*ci=(struct reg_item*)lv_obj_get_user_data(obj);
	if(!ci||!ci->reg)return;
	if(ci->parent){
		go_back(ci->reg);
		return;
	}
	lv_obj_set_checked(ci->btn,lv_checkbox_is_checked(obj));
	size_t c=0;
	list*o=list_first(ci->reg->items);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct reg_item*);
		if(item&&item->chk&&lv_checkbox_is_checked(item->chk))c++;
	}while((o=o->next));
	if(c==0){
		lv_obj_set_enabled(ci->reg->btn_delete,false);
		lv_obj_set_enabled(ci->reg->btn_edit,false);
	}else if(c==1){
		lv_obj_set_enabled(ci->reg->btn_delete,true);
		lv_obj_set_enabled(ci->reg->btn_edit,true);
	}else{
		lv_obj_set_enabled(ci->reg->btn_delete,true);
		lv_obj_set_enabled(ci->reg->btn_edit,false);
	}
}

static void add_item(struct reg_item*ci){
	if(!ci||!ci->reg)return;

	// reg item button
	ci->btn=lv_btn_create(ci->reg->view,NULL);
	lv_obj_set_size(ci->btn,ci->reg->bw,ci->reg->bh);
	lv_style_set_btn_item(ci->btn);
	lv_obj_set_click(ci->btn,false);
	lv_obj_align(
		ci->btn,ci->reg->last_btn,ci->reg->last_btn?
			 LV_ALIGN_OUT_BOTTOM_LEFT:
			 LV_ALIGN_IN_TOP_MID,
		0,ci->reg->bm/8+(ci->reg->last_btn?gui_dpi/20:0)
	);
	ci->reg->last_btn=ci->btn;
	if(list_obj_add_new(&ci->reg->items,ci)!=0){
		telog_error("cannot add reg item list");
		abort();
	}

	// line for button text
	lv_obj_t*line=lv_line_create(ci->btn,NULL);
	lv_obj_set_width(line,ci->reg->bw);

	// reg image
	ci->img=lv_img_create(line,NULL);
	lv_obj_set_size(ci->img,ci->reg->si,ci->reg->si);
	lv_obj_align(ci->img,ci->btn,LV_ALIGN_IN_LEFT_MID,gui_font_size/2,0);
	lv_obj_set_drag_parent(ci->img,true);
	lv_obj_set_click(ci->img,true);
	lv_obj_set_event_cb(ci->img,click_item);
	lv_obj_set_user_data(ci->img,ci);
	lv_obj_add_style(ci->img,LV_IMG_PART_MAIN,&ci->reg->img_s);
	lv_img_ext_t*ext=lv_obj_get_ext_attr(ci->img);
	lv_img_set_src(ci->img,get_icon(ci));
	if((ext->w<=0||ext->h<=0))
		lv_img_set_src(ci->img,"inode-file"MIME_EXT);
	if(ext->w>0&&ext->h>0)lv_img_set_zoom(ci->img,(int)(((float)ci->reg->si/MAX(ext->w,ext->h))*256));
	lv_img_set_pivot(ci->img,0,0);
	lv_group_add_obj(gui_grp,ci->img);

	// reg name and checkbox
	ci->chk=lv_checkbox_create(line,NULL);
	lv_checkbox_set_text(ci->chk,ci->parent?_("Parent key"):ci->name);
	lv_style_set_focus_checkbox(ci->chk);
	lv_obj_set_drag_parent(ci->chk,true);
	lv_obj_set_event_cb(ci->chk,check_item);
	lv_obj_set_user_data(ci->chk,ci);
	lv_obj_align(
		ci->chk,NULL,
		LV_ALIGN_IN_LEFT_MID,
		gui_font_size+ci->reg->si,
		ci->type==hive_t_REG_NONE?0:-gui_font_size
	);
	lv_group_add_obj(gui_grp,ci->chk);
	lv_checkbox_ext_t*e=lv_obj_get_ext_attr(ci->chk);
	lv_label_set_long_mode(e->label,LV_LABEL_LONG_DOT);

	lv_coord_t lbl_w=ci->reg->bw-ci->reg->si-gui_font_size*2-lv_obj_get_width(e->bullet);
	if(lv_obj_get_width(e->label)>lbl_w)lv_obj_set_width(e->label,lbl_w);
	if(ci->type!=hive_t_REG_NONE){
		ci->val=lv_label_create(line,NULL);
		lv_label_set_long_mode(ci->val,LV_LABEL_LONG_DOT);
		lv_obj_set_width(ci->val,ci->reg->bw-ci->reg->si-(gui_font_size*2));
		lv_obj_set_small_text_font(ci->val,LV_LABEL_PART_MAIN);
		lv_obj_align(
			ci->val,NULL,LV_ALIGN_IN_LEFT_MID,
			gui_font_size+ci->reg->si,gui_font_size
		);
		char buf[256]={0};
		hivex_value_to_string(buf,256,ci->reg->hive,ci->value);
		lv_label_set_text(ci->val,buf);
		ci->xtype=lv_label_create(line,NULL);
		lv_label_set_align(ci->xtype,LV_LABEL_ALIGN_RIGHT);
		lv_label_set_long_mode(ci->xtype,LV_LABEL_LONG_EXPAND);
		lv_label_set_text(ci->xtype,hivex_type_to_string(ci->type));
		lv_obj_set_small_text_font(ci->xtype,LV_LABEL_PART_MAIN);
		lv_coord_t xs=lv_obj_get_width(ci->xtype);
		lv_obj_align(
			ci->xtype,ci->btn,LV_ALIGN_IN_TOP_RIGHT,
			-gui_font_size/2,gui_font_size/2
		);
		lbl_w-=xs+gui_dpi/20;
		if(lv_obj_get_width(e->label)>lbl_w)
			lv_obj_set_width(e->label,lbl_w);
	}
}

static void add_node_item(struct regedit*reg,bool parent,hive_node_h dir,hive_node_h n){
	if(!reg)return;
	char*key;
	struct reg_item*ci=malloc(sizeof(struct reg_item));
	if(!ci)return;
	memset(ci,0,sizeof(struct reg_item));
	if(!parent&&(key=hivex_node_name(reg->hive,n))){
		strncpy(ci->name,key,sizeof(ci->name)-1);
		free(key);
	}
	ci->parent=parent;
	ci->node=n;
	ci->reg=reg;
	ci->dir=dir;
	add_item(ci);
}

static void add_value_item(struct regedit*reg,hive_node_h dir,hive_value_h v){
	if(!reg)return;
	char*key=hivex_value_key(reg->hive,v);
	struct reg_item*ci=malloc(sizeof(struct reg_item));
	if(!ci||!key)goto fail;
	memset(ci,0,sizeof(struct reg_item));
	size_t len;
	if(hivex_value_type(reg->hive,v,&ci->type,&len)==-1)goto fail;
	strncpy(ci->name,key,sizeof(ci->name)-1);
	free(key);
	ci->parent=false;
	ci->value=v;
	ci->reg=reg;
	ci->dir=dir;
	add_item(ci);
	return;
	fail:
	if(ci)free(ci);
	if(key)free(key);
}

static void load_view(struct regedit*reg){
	if(!reg)return;
	size_t i=0;
	reg->bm=gui_font_size;
	reg->bw=lv_page_get_scrl_width(reg->view)-reg->bm;
	reg->bh=gui_dpi/2,reg->si=reg->bh-gui_font_size;
	lv_style_init(&reg->img_s);
	lv_style_set_outline_width(&reg->img_s,LV_STATE_FOCUSED,gui_dpi/100);
	lv_style_set_outline_color(&reg->img_s,LV_STATE_FOCUSED,lv_theme_get_color_primary());
	lv_style_set_radius(&reg->img_s,LV_STATE_FOCUSED,gui_dpi/50);
	lv_label_set_text(reg->lbl_path,get_string_path(reg,NULL));
	clean_view(reg);
	if(reg->hive&&reg->node){
		hive_node_h pa=hivex_node_parent(reg->hive,reg->node);
		if(pa)add_node_item(reg,true,0,pa);
		hive_node_h*cs=hivex_node_children(reg->hive,reg->node);
		if(cs){
			for(i=0;cs[i];i++)add_node_item(reg,false,reg->node,cs[i]);
			free(cs);
		}
		hive_value_h*vs=hivex_node_values(reg->hive,reg->node);
		if(vs){
			for(i=0;vs[i];i++)add_value_item(reg,reg->node,vs[i]);
			free(vs);
		}
	}else set_info(reg,_("nothing here"));
}

static int regedit_get_focus(struct gui_activity*d){
	struct regedit*reg=d->data;
	if(!reg)return 0;
	load_view(reg);
	lv_group_add_obj(gui_grp,reg->btn_add);
	lv_group_add_obj(gui_grp,reg->btn_reload);
	lv_group_add_obj(gui_grp,reg->btn_delete);
	lv_group_add_obj(gui_grp,reg->btn_edit);
	lv_group_add_obj(gui_grp,reg->btn_home);
	lv_group_add_obj(gui_grp,reg->btn_save);
	lv_group_add_obj(gui_grp,reg->btn_load);
	lv_obj_set_enabled(reg->btn_save,reg->changed);
	return 0;
}

static int regedit_lost_focus(struct gui_activity*d){
	struct regedit*reg=d->data;
	if(!reg)return 0;
	list*o=list_first(reg->items);
	if(o)do{
		LIST_DATA_DECLARE(item,o,struct reg_item*);
		if(item->img)lv_group_remove_obj(item->img);
		if(item->chk)lv_group_remove_obj(item->chk);
	}while((o=o->next));
	lv_group_remove_obj(reg->btn_add);
	lv_group_remove_obj(reg->btn_reload);
	lv_group_remove_obj(reg->btn_delete);
	lv_group_remove_obj(reg->btn_edit);
	lv_group_remove_obj(reg->btn_home);
	lv_group_remove_obj(reg->btn_save);
	lv_group_remove_obj(reg->btn_load);
	return 0;
}

static void close_file(struct regedit*reg){
	lv_obj_set_enabled(reg->btn_save,false);
	lv_obj_set_enabled(reg->btn_reload,false);
	lv_obj_set_enabled(reg->btn_delete,false);
	lv_obj_set_enabled(reg->btn_edit,false);
	lv_obj_set_enabled(reg->btn_add,false);
	lv_obj_set_enabled(reg->btn_home,false);
	if(reg->hive)hivex_close(reg->hive);
	list_free_all_def(reg->path);
	reg->node=0,reg->root=0,reg->changed=false;
	reg->hive=NULL,reg->path=NULL;
	load_view(reg);
}

static void load_file(struct regedit*reg,char*xpath){
	if(!reg)return;
	close_file(reg);
	#ifdef ENABLE_UEFI
	if(xpath[2]!=':')return;
	EFI_FILE_PROTOCOL*dir=fs_get_root_by_letter(xpath[0]);
	if(!dir){
		msgbox_alert("get root '%c' failed",xpath[0]);
		return;
	}
	reg->hive=hivex_open_uefi(dir,xpath+2,HIVEX_OPEN_WRITE);
	#else
	if(xpath[0]!='/'&&xpath[1]==':')xpath+=2;
	reg->hive=hivex_open(xpath,HIVEX_OPEN_WRITE);
	#endif
	if(!reg->hive){
		msgbox_alert("open registry file '%s' failed: %m",xpath);
		return;
	}
	if((reg->root=hivex_root(reg->hive))<=0){
		msgbox_alert("open registry root failed: %m");
		close_file(reg);
		return;
	}
	tlog_debug("loaded file %s",xpath);
	reg->node=reg->root,reg->changed=false;
	lv_obj_set_enabled(reg->btn_save,false);
	lv_obj_set_enabled(reg->btn_reload,true);
	lv_obj_set_enabled(reg->btn_add,true);
	lv_obj_set_enabled(reg->btn_home,true);
	load_view(reg);
}

static bool reg_load_cb(bool ok,const char**p,uint16_t cnt,void*user_data){
	if(!ok)return false;
	if(!p||!p[0]||p[1]||cnt!=1)return true;
	load_file((struct regedit*)user_data,(char*)p[0]);
	return false;
}

struct create_data{
	uint16_t id;
	struct regedit*reg;
};

static bool reg_create_input_cb(bool ok,const char*content,void*user_data){
	if(!ok)return false;
	if(!content||!content[0])return true;
	bool b;
	struct hive_set_value sv;
	struct create_data*data=user_data;
	sv.len=1,sv.key=(char*)content;
	sv.value=(char[]){0,0,0,0,0,0,0,0};
	switch(data->id){
		case 0:{
			b=hivex_node_add_child(
				data->reg->hive,
				data->reg->node,
				content
			)==0;
			if(b)msgbox_alert("Add registry key failed: %m");
			return b;
		}break;
		case 1:sv.t=hive_t_REG_SZ;break;
		case 2:sv.t=hive_t_REG_MULTI_SZ;break;
		case 3:sv.t=hive_t_REG_DWORD,sv.len=sizeof(int32_t);break;
		case 4:sv.t=hive_t_REG_QWORD,sv.len=sizeof(int64_t);break;
		case 5:sv.t=hive_t_REG_BINARY;break;
		default:return true;
	}
	b=hivex_node_set_value(
		data->reg->hive,
		data->reg->node,
		&sv,0
	)!=0;
	if(b)msgbox_alert("Add registry value failed: %m");
	data->reg->changed=true;
	return b;
}

static bool reg_create_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	if(id>5)return false;
	static struct create_data data;
	data.id=id,data.reg=user_data;
	inputbox_set_user_data(inputbox_create(
		reg_create_input_cb,
		"Input new name"
	),&data);
	return false;
}

static bool reg_save_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	if(id!=0)return false;
	struct regedit*reg=user_data;
	if(hivex_commit(reg->hive,NULL,0)==0){
		reg->changed=false;
		lv_obj_set_enabled(reg->btn_save,reg->changed);
		tlog_debug("file saved");
	}else{
		msgbox_alert("Save registry file failed: %m");
		tlog_warn("save registry failed: %m");
	}
	return false;
}

static bool reg_delete_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	list*l;
	if(id!=0)return false;
	struct regedit*reg=user_data;
	bool failed=false,change_value=false;
	size_t len=hivex_node_nr_values(reg->hive,reg->node);
	hive_value_h*vs=hivex_node_values(reg->hive,reg->node);
	if(!vs){
		telog_warn("get values failed");
		failed=true;
	}else if((l=list_first(reg->items)))do{
		LIST_DATA_DECLARE(item,l,struct reg_item*);
		if(!lv_checkbox_is_checked(item->chk))continue;
		if(
			item->node&&!item->value&&item->type==hive_t_REG_NONE&&
			hivex_node_delete_child(reg->hive,item->node)!=0
		){
			telog_warn(
				"delete child key %zu(%s) failed",
				item->node,item->name
			);
			failed=true;
		}
		if(!item->node&&item->value&&item->type!=hive_t_REG_NONE){
			bool found=false;
			for(size_t i=0;i<len;i++){
				if(vs[i]!=item->value)continue;
				vs[i]=0,found=true;
			}
			if(found)change_value=true;
			else{
				telog_warn(
					"search child value %zu(%s) failed",
					item->node,item->name
				);
				failed=true;
			}
		}
		reg->changed=true;
	}while((l=l->next));
	if(change_value&&!failed){
		size_t new_len=0;
		struct hive_set_value*sv;
		size_t size=(len+1)*sizeof(struct hive_set_value);
		if(!(sv=malloc(size))){
			telog_warn("failed to allocate set values");
			failed=true;
		}else{
			memset(sv,0,size);
			for(size_t i=0;i<len;i++){
				if(!vs[i])continue;
				if(!(sv[new_len].key=hivex_value_key(
					reg->hive,vs[i]
				))||!(sv[new_len].value=hivex_value_value(
					reg->hive,vs[i],
					&sv[new_len].t,
					&sv[new_len].len
				))){
					telog_warn("failed to get value %zu",vs[i]);
					failed=true;
					break;
				}
				new_len++;
			}
			if(!failed&&hivex_node_set_values(
				reg->hive,reg->node,new_len,sv,0
			)!=0){
				telog_warn("failed to set values");
				failed=true;
			}
			for(size_t i=0;i<len;i++){
				if(sv[i].value)free(sv[i].value);
				if(sv[i].key)free(sv[i].key);
			}
			free(sv);
		}
	}
	if(failed)msgbox_alert("One or more items failed to delete");
	return false;
}

static void btns_cb(lv_obj_t*obj,lv_event_t e){
	list*l;
	static struct regedit_value value;
	static const char*create_buttons[]={
		"Registry Key",
		"String Value",
		"Multi String Value",
		"DWORD Value (int32)",
		"QWORD Value (int64)",
		"Binary Data Value",
		"Cancel",
		""
	};
	if(!obj||e!=LV_EVENT_CLICKED)return;
	struct regedit*reg=lv_obj_get_user_data(obj);
	if(!reg||strcmp(guiact_get_last()->name,"regedit")!=0)return;
	value.hive=reg->hive,value.reg=reg;
	value.value=0,value.node=0;
	if(obj==reg->btn_add){
		msgbox_set_user_data(msgbox_create_custom(
			reg_create_cb,
			create_buttons,
			"Select type to add"
		),reg);
	}else if(obj==reg->btn_reload){
		load_view(reg);
	}else if(obj==reg->btn_delete){
		msgbox_set_user_data(msgbox_create_yesno(
			reg_delete_cb,
			"Are you sure you want to delete selected items?"
		),reg);
	}else if(obj==reg->btn_edit){
		if((l=list_first(reg->items)))do{
			LIST_DATA_DECLARE(item,l,struct reg_item*);
			if(lv_checkbox_is_checked(item->chk)){
				value.value=item->value;
				value.node=item->dir;
			}
		}while((l=l->next));
		if(value.value)guiact_start_activity(
			&guireg_regedit_value,&value
		);
	}else if(obj==reg->btn_home){
		list_free_all_def(reg->path);
		reg->path=NULL,reg->node=reg->root;
		load_view(reg);
	}else if(obj==reg->btn_save){
		if(reg->changed)msgbox_set_user_data(msgbox_create_yesno(
			reg_save_cb,
			"Notice: Registry Editor is currently in an early beta version, "
			"it MAY DAMAGE YOUR REGISTRY, are you sure you want to continue save?"
		),reg);
	}else if(obj==reg->btn_load){
		struct filepicker*fp=filepicker_create(reg_load_cb,"Select registry file to load");
		filepicker_set_max_item(fp,1);
		filepicker_set_user_data(fp,reg);
	}
}

static int regedit_draw(struct gui_activity*act){
	lv_coord_t btx=gui_font_size,btm=btx/2,btw=(gui_sw-btm)/7-btm,bth=btx*2;
	struct regedit*reg=(struct regedit*)act->data;
	if(!reg)return 0;
	reg->scr=act->page;

	reg->view=lv_page_create(reg->scr,NULL);
	lv_obj_set_style_local_border_width(reg->view,LV_PAGE_PART_BG,LV_STATE_DEFAULT,0);
	lv_obj_set_style_local_border_width(reg->view,LV_PAGE_PART_BG,LV_STATE_FOCUSED,0);
	lv_obj_set_style_local_border_width(reg->view,LV_PAGE_PART_BG,LV_STATE_PRESSED,0);
	lv_obj_set_width(reg->view,gui_sw);

	// current path
	reg->lbl_path=lv_label_create(reg->scr,NULL);
	lv_label_set_align(reg->lbl_path,LV_LABEL_ALIGN_CENTER);
	lv_label_set_long_mode(reg->lbl_path,confd_get_boolean("gui.text_scroll",true)?
		LV_LABEL_LONG_SROLL_CIRC:
		LV_LABEL_LONG_DOT
	);
	lv_obj_set_size(reg->lbl_path,gui_sw,gui_dpi/7);
	lv_obj_align(reg->lbl_path,NULL,LV_ALIGN_IN_BOTTOM_LEFT,0,-bth-btm*3);
	lv_obj_set_size(reg->view,gui_sw,lv_obj_get_y(reg->lbl_path));
	lv_label_set_text(reg->lbl_path,get_string_path(reg,NULL));

	lv_obj_t*line=lv_obj_create(act->page,NULL);
	lv_obj_set_size(line,gui_sw,gui_dpi/100);
	lv_obj_align(line,reg->lbl_path,LV_ALIGN_OUT_BOTTOM_MID,0,0);
	lv_theme_apply(line,LV_THEME_SCR);
	lv_obj_set_style_local_bg_color(
		line,LV_OBJ_PART_MAIN,LV_STATE_DEFAULT,
		lv_color_darken(lv_obj_get_style_bg_color(line,LV_OBJ_PART_MAIN),LV_OPA_20)
	);

	reg->btn_add=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(reg->btn_add,false);
	lv_obj_set_size(reg->btn_add,btw,bth);
	lv_obj_set_event_cb(reg->btn_add,btns_cb);
	lv_obj_set_user_data(reg->btn_add,reg);
	lv_obj_align(reg->btn_add,NULL,LV_ALIGN_IN_BOTTOM_LEFT,btm,-btm);
	lv_obj_set_style_local_radius(reg->btn_add,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(reg->btn_add,NULL),LV_SYMBOL_PLUS);

	reg->btn_reload=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(reg->btn_reload,false);
	lv_obj_set_size(reg->btn_reload,btw,bth);
	lv_obj_set_event_cb(reg->btn_reload,btns_cb);
	lv_obj_set_user_data(reg->btn_reload,reg);
	lv_obj_align(reg->btn_reload,reg->btn_add,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(reg->btn_reload,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(reg->btn_reload,NULL),LV_SYMBOL_REFRESH);

	reg->btn_delete=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(reg->btn_delete,false);
	lv_obj_set_size(reg->btn_delete,btw,bth);
	lv_obj_set_event_cb(reg->btn_delete,btns_cb);
	lv_obj_set_user_data(reg->btn_delete,reg);
	lv_obj_align(reg->btn_delete,reg->btn_reload,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(reg->btn_delete,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(reg->btn_delete,NULL),LV_SYMBOL_TRASH);

	reg->btn_edit=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(reg->btn_edit,false);
	lv_obj_set_size(reg->btn_edit,btw,bth);
	lv_obj_set_event_cb(reg->btn_edit,btns_cb);
	lv_obj_set_user_data(reg->btn_edit,reg);
	lv_obj_align(reg->btn_edit,reg->btn_delete,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(reg->btn_edit,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(reg->btn_edit,NULL),LV_SYMBOL_EDIT);

	reg->btn_home=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(reg->btn_home,false);
	lv_obj_set_size(reg->btn_home,btw,bth);
	lv_obj_set_event_cb(reg->btn_home,btns_cb);
	lv_obj_set_user_data(reg->btn_home,reg);
	lv_obj_align(reg->btn_home,reg->btn_edit,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(reg->btn_home,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(reg->btn_home,NULL),LV_SYMBOL_HOME);

	reg->btn_save=lv_btn_create(act->page,NULL);
	lv_obj_set_enabled(reg->btn_save,false);
	lv_obj_set_size(reg->btn_save,btw,bth);
	lv_obj_set_event_cb(reg->btn_save,btns_cb);
	lv_obj_set_user_data(reg->btn_save,reg);
	lv_obj_align(reg->btn_save,reg->btn_home,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(reg->btn_save,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(reg->btn_save,NULL),LV_SYMBOL_SAVE);

	reg->btn_load=lv_btn_create(act->page,NULL);
	lv_obj_set_size(reg->btn_load,btw,bth);
	lv_obj_set_event_cb(reg->btn_load,btns_cb);
	lv_obj_set_user_data(reg->btn_load,reg);
	lv_obj_align(reg->btn_load,reg->btn_save,LV_ALIGN_OUT_RIGHT_MID,btm,0);
	lv_obj_set_style_local_radius(reg->btn_load,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btm);
	lv_label_set_text(lv_label_create(reg->btn_load,NULL),LV_SYMBOL_UPLOAD);

	if(act->args)load_file(reg,act->args);
	msgbox_alert(
		"Notice: Registry Editor is currently in an early beta version, "
		"it MAY DAMAGE YOUR REGISTRY, please backup and use it with caution."
	);
	return 0;
}

static int do_clean(struct gui_activity*d){
	struct regedit*reg=d->data;
	if(!reg)return 0;
	list_free_all_def(reg->items);
	list_free_all_def(reg->path);
	if(reg->hive)hivex_close(reg->hive);
	reg->items=NULL,reg->path=NULL,reg->info=NULL;
	reg->hive=NULL,reg->root=0,reg->node=0;
	free(reg);
	d->data=NULL;
	return 0;
}

static bool back_cb(uint16_t id,const char*btn __attribute__((unused)),void*user_data){
	struct regedit*reg=user_data;
	if(id==0){
		reg->changed=false;
		guiact_do_back();
		guiact_do_back();
		return true;
	}
	return false;
}

static int do_back(struct gui_activity*d){
	struct regedit*reg=d->data;
	if(!reg)return 0;
	if(reg->path){
		go_back(reg);
		return 1;
	}
	if(!reg->changed)return 0;
	msgbox_set_user_data(msgbox_create_yesno(
		back_cb,
		"Registry has changed, "
		"do you want to discard any changes?"
	),reg);
	return 1;
}

static int do_init(struct gui_activity*d){
	struct regedit*reg=malloc(sizeof(struct regedit));
	if(!reg)ERET(ENOMEM);
	memset(reg,0,sizeof(struct regedit));
	d->data=reg;
	return 0;
}

struct gui_register guireg_regedit={
	.name="regedit",
	.title="Registry Editor",
	.icon="regedit.svg",
	.show_app=true,
	.open_file=true,
	.open_regex=(char*[]){
		".*/windows/system32/config/.+",
		".*/software$",
		".*/system$",
		".*/security$",
		".*/sam$",
		".*/software$",
		".*/bcd$",
		NULL
	},
	.init=do_init,
	.ask_exit=do_back,
	.quiet_exit=do_clean,
	.get_focus=regedit_get_focus,
	.lost_focus=regedit_lost_focus,
	.draw=regedit_draw,
	.back=true
};
#endif
#endif
