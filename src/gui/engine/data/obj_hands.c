/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_MXML
#include"../render_internal.h"

#define DECL_OBJ_HANDLER(name)\
	extern bool xml_attr_object_pre_##name(xml_render_obj*obj);\
	extern bool xml_attr_object_post_##name(xml_render_obj*obj);
#define DECL_OBJ_HAND(_name,_type,_pre_hand,_post_hand){\
	.valid=true,\
	.name=(#_name),\
	.type=(_type),\
	.pre_hand=(_pre_hand),\
	.post_hand=(_post_hand)\
}
#define DECL_OBJ_CHAND(_name,_type,_hand)\
	DECL_OBJ_HAND(_name,_type,xml_attr_object_pre_##_hand,xml_attr_object_post_##_hand)
#define DECL_OBJ_SHAND(_name,_type,_hand)\
	DECL_OBJ_HAND(_name,_type,_hand,NULL)
#define DECL_OBJ_SCHAND(_name,_type,_hand)\
	DECL_OBJ_SHAND(_name,_type,xml_attr_object_pre_##_hand)

DECL_OBJ_HANDLER(obj);
DECL_OBJ_HANDLER(label);
DECL_OBJ_HANDLER(text);
DECL_OBJ_HANDLER(btn);
DECL_OBJ_HANDLER(checkbox);
DECL_OBJ_HANDLER(dropdown);
DECL_OBJ_HANDLER(arc);
DECL_OBJ_HANDLER(bar);
DECL_OBJ_HANDLER(canvas);
DECL_OBJ_HANDLER(calendar);
DECL_OBJ_HANDLER(keyboard);
DECL_OBJ_HANDLER(led);
DECL_OBJ_HANDLER(img);
DECL_OBJ_HANDLER(imgbtn);
DECL_OBJ_HANDLER(line);
DECL_OBJ_HANDLER(list);
DECL_OBJ_HANDLER(roller);
DECL_OBJ_HANDLER(slider);
DECL_OBJ_HANDLER(spinbox);
DECL_OBJ_HANDLER(switch);
DECL_OBJ_HANDLER(spinner);
DECL_OBJ_HANDLER(btnmatrix);
DECL_OBJ_HANDLER(chart);

xml_obj_handle xml_obj_handles[]={
	DECL_OBJ_SCHAND(VerticalContainer,   OBJ_VER_BOX,  obj),
	DECL_OBJ_SCHAND(HorizontalContainer, OBJ_HOR_BOX,  obj),
	DECL_OBJ_SCHAND(Container,           OBJ_OBJ,      obj),
	DECL_OBJ_SCHAND(VerticalBox,         OBJ_VER_BOX,  obj),
	DECL_OBJ_SCHAND(HorizontalBox,       OBJ_HOR_BOX,  obj),
	DECL_OBJ_SCHAND(VerBox,              OBJ_VER_BOX,  obj),
	DECL_OBJ_SCHAND(HorBox,              OBJ_HOR_BOX,  obj),
	DECL_OBJ_SCHAND(Wrapper,             OBJ_WRAPPER,  obj),
	DECL_OBJ_SCHAND(Wrap,                OBJ_WRAPPER,  obj),
	DECL_OBJ_SCHAND(Box,                 OBJ_OBJ,      obj),
	DECL_OBJ_SCHAND(Object,              OBJ_OBJ,      obj),
	DECL_OBJ_SCHAND(Page,                OBJ_OBJ,      obj),
	DECL_OBJ_SCHAND(View,                OBJ_OBJ,      obj),

	#if LV_USE_LABEL
	DECL_OBJ_SCHAND(Label, OBJ_LABEL, label),
	#endif

	#if LV_USE_BTN
	DECL_OBJ_SCHAND(Btn,        OBJ_BTN,      btn),
	DECL_OBJ_SCHAND(Button,     OBJ_BTN,      btn),
	DECL_OBJ_SCHAND(BtnItem,    OBJ_BTN_ITEM, btn),
	DECL_OBJ_SCHAND(ButtonItem, OBJ_BTN_ITEM, btn),
	#endif

	#if LV_USE_CHECKBOX
	DECL_OBJ_SCHAND(CheckBox, OBJ_CHECKBOX, checkbox),
	#endif

	#if LV_USE_DROPDOWN
	DECL_OBJ_SCHAND(DropDown, OBJ_DROPDOWN, dropdown),
	DECL_OBJ_SCHAND(Combo,    OBJ_DROPDOWN, dropdown),
	#endif

	#if LV_USE_TEXTAREA
	DECL_OBJ_CHAND(Edit,     OBJ_TEXTAREA, text),
	DECL_OBJ_CHAND(EditText, OBJ_TEXTAREA, text),
	DECL_OBJ_CHAND(Text,     OBJ_TEXTAREA, text),
	DECL_OBJ_CHAND(TextArea, OBJ_TEXTAREA, text),
	#endif

	#if LV_USE_ARC
	DECL_OBJ_SCHAND(Arc,      OBJ_ARC, arc),
	DECL_OBJ_SCHAND(Progress, OBJ_ARC, arc),
	#endif

	#if LV_USE_IMG
	DECL_OBJ_SCHAND(Img,     OBJ_IMG, img),
	DECL_OBJ_SCHAND(Image,   OBJ_IMG, img),
	DECL_OBJ_SCHAND(Picture, OBJ_IMG, img),
	#endif

	#if LV_USE_BAR
	DECL_OBJ_SCHAND(Bar, OBJ_BAR, bar),
	#endif

	#if LV_USE_IMGBTN
	DECL_OBJ_SCHAND(ImgBtn,      OBJ_IMGBTN, imgbtn),
	DECL_OBJ_SCHAND(ImgButton,   OBJ_IMGBTN, imgbtn),
	DECL_OBJ_SCHAND(ImageBtn,    OBJ_IMGBTN, imgbtn),
	DECL_OBJ_SCHAND(ImageButton, OBJ_IMGBTN, imgbtn),
	#endif

	#if LV_USE_BTNMATRIX
	DECL_OBJ_SCHAND(BtnMatrix,    OBJ_BTNMATRIX, btnmatrix),
	DECL_OBJ_SCHAND(ButtonMatrix, OBJ_BTNMATRIX, btnmatrix),
	#endif

	#if LV_USE_CALENDAR
	DECL_OBJ_SCHAND(Calendar, OBJ_CALENDAR, calendar),
	DECL_OBJ_SCHAND(Date,     OBJ_CALENDAR, calendar),
	#endif

	#if LV_USE_CANVAS
	DECL_OBJ_SCHAND(Canvas, OBJ_CANVAS, canvas),
	#endif

	#if LV_USE_KEYBOARD
	DECL_OBJ_SCHAND(KBD,      OBJ_KEYBOARD, keyboard),
	DECL_OBJ_SCHAND(KeyBoard, OBJ_KEYBOARD, keyboard),
	#endif

	#if LV_USE_LED
	DECL_OBJ_SCHAND(Led,   OBJ_LED, led),
	DECL_OBJ_SCHAND(Light, OBJ_LED, led),
	DECL_OBJ_SCHAND(Lamp,  OBJ_LED, led),
	#endif

	#if LV_USE_LINE
	DECL_OBJ_SCHAND(Line, OBJ_LINE, line),
	#endif

	#if LV_USE_LIST
	DECL_OBJ_SCHAND(List, OBJ_LIST, list),
	#endif

	#if LV_USE_ROLLER
	DECL_OBJ_SCHAND(Roller, OBJ_ROLLER, roller),
	#endif

	#if LV_USE_SLIDER
	DECL_OBJ_SCHAND(Slider, OBJ_SLIDER, slider),
	#endif

	#if LV_USE_SPINBOX
	DECL_OBJ_SCHAND(SpinBox, OBJ_SPINBOX, spinbox),
	#endif

	#if LV_USE_SWITCH
	DECL_OBJ_SCHAND(Switch, OBJ_SWITCH, switch),
	#endif

	#if LV_USE_SPINNER
	DECL_OBJ_SCHAND(Spinner, OBJ_SPINNER, spinner),
	#endif

	#if LV_USE_CHART
	DECL_OBJ_SCHAND(Chart, OBJ_CHART, chart),
	#endif

	{.valid=false,.name="",.type=OBJ_NONE,.pre_hand=NULL,.post_hand=NULL}
};
#endif
#endif
