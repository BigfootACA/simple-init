#ifndef TOOLS_H
#define TOOLS_H
extern lv_style_t*lv_style_opa_mask();
extern lv_obj_t*lv_create_opa_mask(lv_obj_t*par);
extern lv_obj_t*lv_create_msgbox(lv_obj_t*par,const char**btns,lv_event_cb_t cb,const char*text,...);
extern lv_obj_t*lv_create_msgbox_mask(lv_obj_t*par,const char**btns,lv_event_cb_t cb,const char*text,...);
extern lv_obj_t*lv_create_ok_msgbox(lv_obj_t*par,lv_event_cb_t cb,const char*text,...);
extern lv_obj_t*lv_create_ok_msgbox_mask(lv_obj_t*par,lv_event_cb_t cb,const char*text,...);
extern lv_obj_t*lv_create_yesno_msgbox(lv_obj_t*par,lv_event_cb_t cb,const char*text,...);
extern lv_obj_t*lv_create_yesno_msgbox_mask(lv_obj_t*par,lv_event_cb_t cb,const char*text,...);
#endif
