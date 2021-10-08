#ifdef ENABLE_GUI
#include<stdlib.h>
#include"gui.h"
#include"logger.h"
#include"gui/tools.h"
#include"gui/msgbox.h"
#include"gui/activity.h"

struct msgbox{
	char text[BUFSIZ];
	const char**buttons;
	msgbox_callback callback;
	uint16_t btn_cnt;
	lv_obj_t*mask;
	lv_obj_t*box;
	lv_obj_t*label;
	lv_obj_t*btn[64];
	void*btn_data_p;
	void*user_data;
	struct gui_activity*act;
};

struct msgbox_btn{
	uint16_t id;
	const char*text;
	struct msgbox*box;
};

static const char*null_btn[]={""};
static const char*ok_btn[]={LV_SYMBOL_OK,""};
static const char*yesno_btn[]={LV_SYMBOL_OK,LV_SYMBOL_CLOSE,""};

static void msg_click(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	struct msgbox_btn*btn;
	struct msgbox*box;
	if(!(btn=lv_obj_get_user_data(obj))||!(box=btn->box))return;
	if(guiact_get_last()->args!=box)return;
	if(box->callback&&box->callback(btn->id,btn->text,box->user_data))return;
	for(uint16_t i=0;i<box->btn_cnt;i++)
		lv_obj_set_user_data(box->btn[i],NULL);
	box->act->args=NULL;
	free(box->btn_data_p);
	free(box);
	guiact_do_back();
}

static int msgbox_draw(struct gui_activity*act){
	lv_coord_t box_h=0;
	lv_coord_t max_w=gui_dpi*4,cur_w=gui_sw/4*3,xw=MIN(max_w,cur_w);
	lv_coord_t max_h=gui_dpi*6,cur_h=gui_sh/3*2,xh=MIN(max_h,cur_h);
	struct msgbox*box=act->args;
	if(!box->buttons)box->buttons=null_btn;
	for(box->btn_cnt=0;*box->buttons[box->btn_cnt];box->btn_cnt++);
	if(box->btn_cnt>64)return 3;
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

	if(box->btn_cnt>0){
		lv_coord_t
			btn_m=gui_font_size/2,
			btn_w=lv_page_get_scrl_width(box->box),
			btn_h=gui_font_size+(gui_dpi/8);
		box_h+=btn_m;
		struct msgbox_btn*d=malloc(sizeof(struct msgbox_btn)*box->btn_cnt);
		if(!d){
			free(box);
			return 4;
		}
		box->btn_data_p=d;
		for(uint16_t i=0;i<box->btn_cnt;i++){
			d[i].box=box,d[i].id=i,d[i].text=box->buttons[i];
			box->btn[i]=lv_btn_create(box->box,NULL);
			lv_label_set_text(lv_label_create(box->btn[i],NULL),box->buttons[i]);
			lv_obj_set_style_local_margin_bottom(box->btn[i],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,btn_m);
			lv_obj_set_style_local_radius(box->btn[i],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,gui_dpi/15);
			lv_obj_set_size(box->btn[i],btn_w-btn_m,btn_h);
			lv_obj_set_user_data(box->btn[i],&d[i]);
			lv_obj_set_event_cb(box->btn[i],msg_click);
		}
		if(box->btn_cnt<=5){
			btn_w/=box->btn_cnt,box_h+=btn_m;
			for(uint16_t i=0;i<box->btn_cnt;i++){
				lv_obj_set_pos(box->btn[i],(btn_w*i)+(btn_m/2),box_h);
				lv_obj_set_width(box->btn[i],btn_w-btn_m);
			}
			box_h+=btn_h+btn_m;
		}else for(uint16_t i=0;i<box->btn_cnt;i++){
			lv_obj_set_pos(box->btn[i],0,box_h+btn_m);
			box_h+=btn_h+btn_m;
		}
	}
	box_h+=gui_dpi/4;
	lv_obj_set_height(box->box,MIN(box_h,xh));
	lv_obj_align(box->box,NULL,LV_ALIGN_CENTER,0,0);

	return 0;
}

static int msgbox_get_focus(struct gui_activity*d){
	struct msgbox*box=d->args;
	if(!box)return 0;
	for(uint16_t i=0;i<box->btn_cnt;i++)
		lv_group_add_obj(gui_grp,box->btn[i]);
	return 0;
}

static int msgbox_lost_focus(struct gui_activity*d){
	struct msgbox*box=d->args;
	if(!box)return 0;
	for(uint16_t i=0;i<box->btn_cnt;i++)
		lv_group_remove_obj(box->btn[i]);
	return 0;
}

struct gui_register guireg_msgbox={
	.name="msgbox",
	.title="Message Box",
	.show_app=false,
	.draw=msgbox_draw,
	.get_focus=msgbox_get_focus,
	.lost_focus=msgbox_lost_focus,
	.back=true,
	.mask=true,
};

static void msgbox_cb(lv_task_t*t){
	guiact_start_activity(&guireg_msgbox,t->user_data);
}

static struct msgbox*_msgbox_create(
	msgbox_callback callback,
	const char**buttons,
	const char*content,
	va_list va
){
	struct msgbox*msg=malloc(sizeof(struct msgbox));
	if(!msg)return NULL;
	memset(msg,0,sizeof(struct msgbox));
	vsnprintf(msg->text,sizeof(msg->text)-1,_(content),va);
	msg->callback=callback;
	msg->buttons=buttons;
	lv_task_once(lv_task_create(msgbox_cb,0,LV_TASK_PRIO_LOWEST,msg));
	return msg;
}

struct msgbox*msgbox_create_yesno(msgbox_callback callback,const char*content,...){
	va_list va;
	va_start(va,content);
	struct msgbox*msg=_msgbox_create(callback,yesno_btn,content,va);
	va_end(va);
	return msg;
}

struct msgbox*msgbox_create_ok(msgbox_callback callback,const char*content,...){
	va_list va;
	va_start(va,content);
	struct msgbox*msg=_msgbox_create(callback,ok_btn,content,va);
	va_end(va);
	return msg;
}

struct msgbox*msgbox_create_custom(msgbox_callback callback,const char**btns,const char*content,...){
	va_list va;
	va_start(va,content);
	struct msgbox*msg=_msgbox_create(callback,btns,content,va);
	va_end(va);
	return msg;
}

struct msgbox*msgbox_alert(const char*content,...){
	va_list va;
	va_start(va,content);
	struct msgbox*msg=_msgbox_create(NULL,ok_btn,content,va);
	va_end(va);
	return msg;
}

void msgbox_set_user_data(struct msgbox*box,void*user_data){
	if(!box)return;
	box->user_data=user_data;
}
#endif
