#ifdef ENABLE_GUI
#include<stdlib.h>
#include"gui.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/sysbar.h"
#include"gui/inputbox.h"
#include"gui/activity.h"

struct inputbox{
	char text[BUFSIZ];
	char holder[BUFSIZ];
	char content[BUFSIZ];
	const char*accepts;
	inputbox_callback callback;
	lv_obj_t*mask,*box;
	lv_obj_t*label,*input;
	lv_obj_t*ok,*cancel;
	lv_label_align_t align;
	lv_event_cb_t input_cb;
	uint32_t max;
	bool one_line,pwd;
	void*user_data;
	struct gui_activity*act;
};

static void input_click(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	struct inputbox*box;
	if(!(box=lv_obj_get_user_data(obj)))return;
	if(guiact_get_last()->args!=box)return;
	sysbar_keyboard_close();
	sysbar_focus_input(NULL);
	const char*cont=lv_textarea_get_text(box->input);
	if(box->callback&&box->callback(obj==box->ok,cont,box->user_data))return;
	lv_obj_set_user_data(box->ok,NULL);
	lv_obj_set_user_data(box->cancel,NULL);
	box->act->args=NULL;
	free(box);
	guiact_do_back();
}

static void input_repos(struct inputbox*box){
	lv_coord_t px=gui_sw,py=gui_sh;
	px-=lv_obj_get_width(box->box),py-=lv_obj_get_height(box->box);
	if(sysbar.keyboard)py-=lv_obj_get_height(sysbar.keyboard);
	px/=2,py/=2;
	lv_obj_set_pos(box->box,px,py);
}

static void text_cb(lv_obj_t*obj,lv_event_t e){
	if(!obj)return;
	struct inputbox*box;
	if(!(box=lv_obj_get_user_data(obj)))return;
	if(guiact_get_last()->args!=box)return;
	switch(e){
		case LV_EVENT_CLICKED:
			sysbar_focus_input(obj);
			sysbar_keyboard_open();
			//fallthrough
		case LV_EVENT_DEFOCUSED:
		case LV_EVENT_FOCUSED:
			input_repos(box);
	}
	if(box->input_cb)box->input_cb(obj,e);
}

static int inputbox_draw(struct gui_activity*act){
	lv_coord_t box_h=0;
	lv_coord_t max_w=gui_dpi*4,cur_w=gui_sw/4*3,xw=MIN(max_w,cur_w);
	lv_coord_t max_h=gui_dpi*6,cur_h=gui_sh/3*2,xh=MIN(max_h,cur_h);
	struct inputbox*box=act->args;
	box->act=act;

	box->mask=lv_create_opa_mask(act->page);
	box->box=lv_page_create(box->mask,NULL);
	lv_obj_set_style_local_border_width(box->box,LV_PAGE_PART_BG,LV_STATE_DEFAULT,0);
	lv_obj_set_style_local_border_width(box->box,LV_PAGE_PART_BG,LV_STATE_PRESSED,0);
	lv_obj_set_style_local_border_width(box->box,LV_PAGE_PART_BG,LV_STATE_FOCUSED,0);
	lv_obj_set_width(box->box,xw);

	box->label=lv_label_create(box->box,NULL);
	lv_label_set_align(box->label,LV_LABEL_ALIGN_CENTER);
	lv_label_set_long_mode(box->label,LV_LABEL_LONG_BREAK);
	lv_obj_set_width(box->label,lv_page_get_scrl_width(box->box));
	lv_label_set_text(box->label,box->text);
	box_h+=lv_obj_get_height(box->label);

	lv_coord_t
		btn_m=gui_font_size/2,
		btn_w=lv_page_get_scrl_width(box->box)/2,
		btn_h=gui_font_size+(gui_dpi/8);
	box_h+=btn_m;

	box->input=lv_textarea_create(box->box,NULL);
	lv_textarea_set_text(box->input,box->content);
	lv_textarea_set_cursor_hidden(box->input,true);
	lv_textarea_set_pwd_mode(box->input,box->pwd);
	lv_textarea_set_one_line(box->input,box->one_line);
	if(box->align!=LV_LABEL_ALIGN_AUTO)lv_textarea_set_text_align(box->input,box->align);
	if(box->max>0)lv_textarea_set_max_length(box->input,box->max);
	if(box->holder[0])lv_textarea_set_placeholder_text(box->input,box->holder);
	if(box->accepts)lv_textarea_set_accepted_chars(box->input,box->accepts);
	lv_obj_set_style_local_margin_bottom(box->input,LV_TEXTAREA_PART_BG,LV_STATE_DEFAULT,btn_m);
	lv_obj_set_pos(box->input,btn_m/2,box_h);
	lv_obj_set_user_data(box->input,box);
	lv_obj_set_event_cb(box->input,text_cb);
	lv_obj_set_width(box->input,lv_page_get_width_fit(box->box)-btn_m);
	box_h+=lv_obj_get_height(box->input)+btn_m;

	box->ok=lv_btn_create(box->box,NULL);
	lv_label_set_text(lv_label_create(box->ok,NULL),LV_SYMBOL_OK);
	lv_obj_set_style_local_margin_bottom(box->ok,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btn_m);
	lv_obj_set_style_local_radius(box->ok,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,gui_dpi/15);
	lv_obj_set_size(box->ok,btn_w-btn_m,btn_h);
	lv_obj_set_user_data(box->ok,box);
	lv_obj_set_event_cb(box->ok,input_click);
	lv_obj_set_pos(box->ok,btn_m/2,box_h);

	box->cancel=lv_btn_create(box->box,NULL);
	lv_label_set_text(lv_label_create(box->cancel,NULL),LV_SYMBOL_CLOSE);
	lv_obj_set_style_local_margin_bottom(box->cancel,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btn_m);
	lv_obj_set_style_local_radius(box->cancel,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,gui_dpi/15);
	lv_obj_set_size(box->cancel,btn_w-btn_m,btn_h);
	lv_obj_set_user_data(box->cancel,box);
	lv_obj_set_event_cb(box->cancel,input_click);
	lv_obj_set_pos(box->cancel,btn_m/2+btn_w,box_h);

	box_h+=btn_m+btn_h+gui_dpi/4;
	lv_obj_set_height(box->box,MIN(box_h,xh));
	input_repos(box);

	return 0;
}

static int inputbox_clean(struct gui_activity*d){
	struct inputbox*box=d->args;
	if(!box)return 0;
	sysbar_focus_input(NULL);
	free(box);
	return 0;
}

static int inputbox_get_focus(struct gui_activity*d){
	struct inputbox*box=d->args;
	if(!box)return 0;
	lv_group_add_obj(gui_grp,box->input);
	lv_group_add_obj(gui_grp,box->ok);
	lv_group_add_obj(gui_grp,box->cancel);
	return 0;
}

static int inputbox_lost_focus(struct gui_activity*d){
	struct inputbox*box=d->args;
	if(!box)return 0;
	lv_group_remove_obj(box->input);
	lv_group_remove_obj(box->ok);
	lv_group_remove_obj(box->cancel);
	return 0;
}

struct gui_register guireg_inputbox={
	.name="inputbox",
	.title="Input Box",
	.show_app=false,
	.draw=inputbox_draw,
	.quiet_exit=inputbox_clean,
	.get_focus=inputbox_get_focus,
	.lost_focus=inputbox_lost_focus,
	.back=true,
	.mask=true,
};

static void inputbox_cb(lv_task_t*t){
	guiact_start_activity(&guireg_inputbox,t->user_data);
}

struct inputbox*inputbox_create(inputbox_callback callback,const char*title,...){
	struct inputbox*input=malloc(sizeof(struct inputbox));
	if(!input)return NULL;
	memset(input,0,sizeof(struct inputbox));
	if(title){
		va_list va;
		va_start(va,title);
		vsnprintf(input->text,sizeof(input->text)-1,_(title),va);
		va_end(va);
	}
	input->callback=callback;
	input->one_line=true;
	input->align=LV_LABEL_ALIGN_AUTO;
	lv_task_once(lv_task_create(inputbox_cb,0,LV_TASK_PRIO_LOWEST,input));
	return input;
}

void inputbox_set_one_line(struct inputbox*input,bool one_line){
	if(!input)return;
	input->one_line=one_line;
}

void inputbox_set_pwd_mode(struct inputbox*input,bool pwd){
	if(!input)return;
	input->pwd=pwd;
}

void inputbox_set_input_align(struct inputbox*input,lv_label_align_t align){
	if(!input)return;
	input->align=align;
}

void inputbox_set_max_length(struct inputbox*input,uint32_t max){
	if(!input)return;
	input->max=max;
}

void inputbox_set_input_event_cb(struct inputbox*input,lv_event_cb_t cb){
	if(!input)return;
	input->input_cb=cb;
}

void inputbox_set_callback(struct inputbox*input,inputbox_callback cb){
	if(!input)return;
	input->callback=cb;
}

void inputbox_set_accept(struct inputbox*input,const char*accept){
	if(!input||!accept)return;
	input->accepts=accept;
}

void inputbox_set_title(struct inputbox*input,const char*title,...){
	if(!input)return;
	memset(input->text,0,sizeof(input->text));
	if(!title)return;
	va_list va;
	va_start(va,title);
	vsnprintf(input->text,sizeof(input->text)-1,_(title),va);
	va_end(va);
}

void inputbox_set_content(struct inputbox*input,const char*content,...){
	if(!input)return;
	memset(input->content,0,sizeof(input->content));
	if(!content)return;
	va_list va;
	va_start(va,content);
	vsnprintf(input->content,sizeof(input->content)-1,content,va);
	va_end(va);
}

void inputbox_set_holder(struct inputbox*input,const char*holder,...){
	if(!input)return;
	memset(input->holder,0,sizeof(input->holder));
	if(!holder)return;
	va_list va;
	va_start(va,holder);
	vsnprintf(input->holder,sizeof(input->holder)-1,_(holder),va);
	va_end(va);
}

void inputbox_set_user_data(struct inputbox*input,void*user_data){
	if(!input)return;
	input->user_data=user_data;
}

#endif
