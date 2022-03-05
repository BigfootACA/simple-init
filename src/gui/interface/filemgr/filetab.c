/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#define _GNU_SOURCE
#include<stdlib.h>
#include<string.h>
#include"gui.h"
#include"logger.h"
#include"gui/filetab.h"
#include"gui/fileview.h"
#define TAG "filetab"

struct filetab{
	uint16_t tab_id;
	lv_obj_t*tab;
	lv_obj_t*view;
	struct fileview*fv;
	filetab_on_change_dir on_change_dir;
	filetab_on_item_click on_item_click;
	filetab_on_item_select on_item_select;
};

static void on_change_dir(struct fileview*fv,char*old,char*new){
	struct filetab*ft=fileview_get_data(fv);
	size_t cnt=strlen(new);
	char path[PATH_MAX]={0},*p;
	strncpy(path,new,cnt);
	if(cnt>1&&(path[cnt-1]=='/'||path[cnt-1]=='\\'))path[cnt-1]=0;
	p=strrchr(path,'/');
	if(!p)p=strrchr(path,'\\');
	if(!p)p=path;
	else if(cnt>1)p++;
	lv_tabview_set_tab_name(ft->view,ft->tab_id,p);
	if(ft->on_change_dir)ft->on_change_dir(ft,old,new);
}

static bool on_item_click(struct fileview*fv,char*item,enum item_type type){
	struct filetab*ft=fileview_get_data(fv);
	return (ft->on_item_click)?ft->on_item_click(ft,item,type):true;
}

static void on_item_select(struct fileview*fv,char*item,enum item_type type,bool checked,uint16_t cnt){
	struct filetab*ft=fileview_get_data(fv);
	if(ft->on_item_select)ft->on_item_select(ft,item,type,checked,cnt);
}

struct filetab*filetab_create(lv_obj_t*view,char*path){
	struct filetab*tb=malloc(sizeof(struct filetab));
	if(!tb)return NULL;
	memset(tb,0,sizeof(struct filetab));
	tb->view=view;
	if(!(tb->tab=lv_tabview_add_tab(view,_("File Manager")))){
		free(tb);
		return NULL;
	}
	if(!(tb->fv=fileview_create(tb->tab))){
		free(tb);
		tlog_error("initialize fileview failed");
		abort();
	}
	fileview_set_data(tb->fv,tb);
	fileview_set_on_change_dir(tb->fv,on_change_dir);
	fileview_set_on_item_click(tb->fv,on_item_click);
	fileview_set_on_item_select(tb->fv,on_item_select);
	for(uint16_t i=0;i<lv_tabview_get_tab_count(view);i++){
		if(lv_tabview_get_tab(view,i)!=tb->tab)continue;
		tb->tab_id=i;
		if(path)fileview_set_path(tb->fv,path);
		return tb;
	}
	tlog_error("cannot get new tab");
	abort();
}

void filetab_add_group(struct filetab*tab,lv_group_t*grp){
	if(tab)fileview_add_group(tab->fv,grp);
}

void filetab_remove_group(struct filetab*tab){
	if(tab)fileview_remove_group(tab->fv);
}

void filetab_free(struct filetab*tab){
	if(!tab)return;
	if(tab->fv)fileview_free(tab->fv);
	if(tab->tab)lv_tabview_clean_tab(tab->tab);
	free(tab);
}

uint16_t filetab_get_id(struct filetab*tab){
	return tab?tab->tab_id:0;
}

lv_obj_t*filetab_get_tab(struct filetab*tab){
	return tab?tab->tab:NULL;
}

char*filetab_get_path(struct filetab*tab){
	return tab?fileview_get_path(tab->fv):NULL;
}

char*filetab_get_lvgl_path(struct filetab*tab){
	return tab?fileview_get_lvgl_path(tab->fv):NULL;
}

uint16_t filetab_get_checked_count(struct filetab*tab){
	return tab?fileview_get_checked_count(tab->fv):0;
}

char**filetab_get_checked(struct filetab*tab){
	return tab?fileview_get_checked(tab->fv):NULL;
}

bool filetab_is_top(struct filetab*tab){
	return tab?fileview_is_top(tab->fv):true;
}

bool filetab_is_active(struct filetab*tab){
	if(!tab||!tab->view)return false;
	return lv_tabview_get_tab_act(tab->view)==tab->tab_id;
}

void filetab_set_on_change_dir(struct filetab*tab,filetab_on_change_dir cb){
	tab->on_change_dir=cb;
}

void filetab_set_on_item_click(struct filetab*tab,filetab_on_item_click cb){
	tab->on_item_click=cb;
}

void filetab_set_on_item_select(struct filetab*tab,filetab_on_item_select cb){
	tab->on_item_select=cb;
}

void filetab_set_show_parent(struct filetab*tab,bool parent){
	if(!tab)return;
	fileview_set_show_parent(tab->fv,parent);
}

void filetab_go_back(struct filetab*tab){
	if(!tab)return;
	fileview_go_back(tab->fv);
}

void filetab_set_path(struct filetab*tab,char*path){
	if(!tab)return;
	fileview_set_path(tab->fv,path);

}
#endif
