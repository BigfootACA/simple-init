#ifndef _INPUTBOX_H
#define _INPUTBOX_H
#include"gui.h"
struct inputbox;
typedef bool(*inputbox_callback)(bool ok,const char*content,void*user_data);
extern struct inputbox*inputbox_create(inputbox_callback callback,const char*title,...) __attribute__((format(printf,2,3)));
extern void inputbox_set_content(struct inputbox*input,const char*content,...) __attribute__((format(printf,2,3)));
extern void inputbox_set_holder(struct inputbox*input,const char*holder,...) __attribute__((format(printf,2,3)));
extern void inputbox_set_title(struct inputbox*input,const char*title,...) __attribute__((format(printf,2,3)));
extern void inputbox_set_accept(struct inputbox*input,const char*accept);
extern void inputbox_set_one_line(struct inputbox*input,bool one_line);
extern void inputbox_set_pwd_mode(struct inputbox*input,bool pwd);
extern void inputbox_set_input_align(struct inputbox*input,lv_label_align_t align);
extern void inputbox_set_max_length(struct inputbox*input,uint32_t max);
extern void inputbox_set_input_event_cb(struct inputbox*input,lv_event_cb_t cb);
extern void inputbox_set_callback(struct inputbox*input,inputbox_callback cb);
extern void inputbox_set_user_data(struct inputbox*input,void*user_data);
#endif
