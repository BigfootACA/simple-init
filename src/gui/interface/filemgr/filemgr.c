#include<stdlib.h>
#include"lvgl.h"
#include"gui.h"
#include"tools.h"
#include"logger.h"
#include"array.h"
#include"filetab.h"
#include"activity.h"
#define TAG "filemgr"

static lv_obj_t*tabview,*scr,*path;
static struct filetab*tabs[4],*active=NULL;

static int filemgr_get_focus(struct gui_activity*d __attribute__((unused))){
	if(active)filetab_add_group(active,gui_grp);
	return 0;
}

static int filemgr_lost_focus(struct gui_activity*d __attribute__((unused))){
	if(active)filetab_remove_group(active);
	return 0;
}

static void update_active(){
	struct filetab*old=active;
	filetab_remove_group(active);
	active=NULL;
	for(size_t i=0;i<ARRLEN(tabs);i++){
		if(!filetab_is_active(tabs[i]))continue;
		active=tabs[i];
		filetab_add_group(active,gui_grp);
		lv_label_set_text(path,filetab_get_path(active));
		if(old!=active)tlog_debug(
			"switch to tab %d",
			filetab_get_id(active)
		);
		break;
	}
}

int do_cleanup(struct gui_activity*d __attribute__((unused))){
	for(size_t i=0;i<ARRLEN(tabs);i++){
		if(tabs[i])filetab_free(tabs[i]);
		tabs[i]=NULL;
	}
	active=NULL;
	return 0;
}

static void tabview_cb(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_VALUE_CHANGED)return;
	update_active();
}

static void on_change_dir(struct filetab*fv,char*old __attribute__((unused)),char*new){
	if(filetab_is_active(fv))lv_label_set_text(path,new);
}

static void tabview_create(){
	for(size_t i=0;i<ARRLEN(tabs);i++){
		if(tabs[i])continue;
		if(!(tabs[i]=filetab_create(tabview,"/"))){
			tlog_error("cannot allocate filetab");
			abort();
		}
		filetab_set_on_change_dir(tabs[i],on_change_dir);
		filetab_set_show_parent(tabs[i],false);
		update_active();
	}
}

static int do_back(struct gui_activity*d __attribute__((unused))){
	lv_obj_t*tab=filetab_get_tab(active);
	if(!tab)return 0;
	if(!lv_page_is_top(tab)){
		lv_page_go_top(tab);
		return 1;
	}
	if(!filetab_is_top(active)){
		filetab_go_back(active);
		return 1;
	}
	return 0;
}

static const char*btns[]={
	LV_SYMBOL_REFRESH,
	LV_SYMBOL_COPY,
	LV_SYMBOL_CUT,
	LV_SYMBOL_PASTE,
	LV_SYMBOL_TRASH,
	LV_SYMBOL_PLUS,
	""
};

static void btns_cb(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_VALUE_CHANGED)return;
	switch(lv_btnmatrix_get_active_btn(obj)){
		case 0://refresh
			filetab_set_path(active,NULL);
		break;
		case 1://copy
		break;
		case 2://cut
		break;
		case 3://paste
		break;
		case 4://delete
		break;
		case 5://new
		break;
	}
}

static int filemgr_draw(struct gui_activity*act){
	scr=act->page;

	static lv_style_t s;
	lv_style_init(&s);
	lv_style_set_pad_all(&s,LV_STATE_DEFAULT,gui_font_size/2);

	// file view
	tabview=lv_tabview_create(scr,NULL);
	lv_obj_set_event_cb(tabview,tabview_cb);
	lv_obj_add_style(tabview,LV_TABVIEW_PART_TAB_BTN,&s);

	static lv_style_t btn;
	lv_style_init(&btn);
	lv_style_set_radius(&btn,LV_STATE_DEFAULT,0);
	lv_style_set_border_width(&btn,LV_STATE_DEFAULT,0);

	// action buttons
	lv_obj_t*b=lv_btnmatrix_create(scr,NULL);
	lv_btnmatrix_set_map(b,btns);
	lv_obj_add_style(b,LV_BTNMATRIX_PART_BG,&btn);
	lv_obj_set_size(b,gui_sw,gui_dpi/5*2);
	lv_obj_set_event_cb(b,btns_cb);
	lv_obj_align(b,NULL,LV_ALIGN_IN_BOTTOM_MID,0,0);

	// current path
	path=lv_label_create(scr,NULL);
	lv_label_set_align(path,LV_LABEL_ALIGN_CENTER);
	lv_label_set_long_mode(path,LV_LABEL_LONG_SROLL_CIRC);
	lv_obj_set_size(path,gui_sw,gui_dpi/7);
	lv_obj_align(path,b,LV_ALIGN_OUT_TOP_LEFT,0,0);
	lv_obj_set_size(tabview,gui_sw,lv_obj_get_y(path));

	tabview_create();
	return 0;
}

struct gui_register guireg_filemgr={
	.name="file-manager",
	.title="File Manager",
	.icon="filemgr.png",
	.show_app=true,
	.ask_exit=do_back,
	.quiet_exit=do_cleanup,
	.get_focus=filemgr_get_focus,
	.lost_focus=filemgr_lost_focus,
	.draw=filemgr_draw,
	.back=true
};
