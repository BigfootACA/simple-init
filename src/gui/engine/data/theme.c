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

#define DECL_THEME(_name,_theme){.valid=true,.name=(#_name),.theme=(_theme)}

xml_theme_spec xml_theme_specs[]={
	DECL_THEME(scr,    LV_THEME_SCR),
	DECL_THEME(screen, LV_THEME_SCR),
	DECL_THEME(obj,    LV_THEME_OBJ),
	DECL_THEME(object, LV_THEME_OBJ),

	#if LV_USE_ARC
	DECL_THEME(arc, LV_THEME_ARC),
	#endif

	#if LV_USE_BAR
	DECL_THEME(bar, LV_THEME_BAR),
	#endif

	#if LV_USE_BTN
	DECL_THEME(btn,    LV_THEME_BTN),
	DECL_THEME(button, LV_THEME_BTN),
	#endif

	#if LV_USE_BTNMATRIX
	DECL_THEME(btnmatrix,     LV_THEME_BTNMATRIX),
	DECL_THEME(btn-matrix,    LV_THEME_BTNMATRIX),
	DECL_THEME(buttonmatrix,  LV_THEME_BTNMATRIX),
	DECL_THEME(button-matrix, LV_THEME_BTNMATRIX),
	#endif

	#if LV_USE_CALENDAR
	DECL_THEME(calendar, LV_THEME_CALENDAR),
	#endif

	#if LV_USE_CANVAS
	DECL_THEME(canvas, LV_THEME_CANVAS),
	#endif

	#if LV_USE_CHECKBOX
	DECL_THEME(checkbox, LV_THEME_CHECKBOX),
	#endif

	#if LV_USE_CHART
	DECL_THEME(chart, LV_THEME_CHART),
	#endif

	#if LV_USE_CONT
	DECL_THEME(cont,      LV_THEME_CONT),
	DECL_THEME(container, LV_THEME_CONT),
	#endif

	#if LV_USE_CPICKER
	DECL_THEME(cpicker, LV_THEME_CPICKER),
	#endif

	#if LV_USE_DROPDOWN
	DECL_THEME(dropdown, LV_THEME_DROPDOWN),
	#endif

	#if LV_USE_GAUGE
	DECL_THEME(gauge, LV_THEME_GAUGE),
	#endif

	#if LV_USE_IMG
	DECL_THEME(img,   LV_THEME_IMAGE),
	DECL_THEME(image, LV_THEME_IMAGE),
	#endif

	#if LV_USE_IMGBTN
	DECL_THEME(imgbtn,       LV_THEME_IMGBTN),
	DECL_THEME(img-btn,      LV_THEME_IMGBTN),
	DECL_THEME(imagebtn,     LV_THEME_IMGBTN),
	DECL_THEME(image-btn,    LV_THEME_IMGBTN),
	DECL_THEME(imagebutton,  LV_THEME_IMGBTN),
	DECL_THEME(image-button, LV_THEME_IMGBTN),
	DECL_THEME(imgbutton,    LV_THEME_IMGBTN),
	DECL_THEME(img-button,   LV_THEME_IMGBTN),
	#endif

	#if LV_USE_KEYBOARD
	DECL_THEME(keyboard, LV_THEME_KEYBOARD),
	#endif

	#if LV_USE_LABEL
	DECL_THEME(label, LV_THEME_LABEL),
	#endif

	#if LV_USE_LED
	DECL_THEME(led, LV_THEME_LED),
	#endif

	#if LV_USE_LINE
	DECL_THEME(line, LV_THEME_LINE),
	#endif

	#if LV_USE_LIST
	DECL_THEME(list,        LV_THEME_LIST),
	DECL_THEME(listbtn,     LV_THEME_LIST_BTN),
	DECL_THEME(listbutton,  LV_THEME_LIST_BTN),
	DECL_THEME(list-btn,    LV_THEME_LIST_BTN),
	DECL_THEME(list-button, LV_THEME_LIST_BTN),
	#endif

	#if LV_USE_LINEMETER
	DECL_THEME(linemeter, LV_THEME_LINEMETER),
	#endif

	#if LV_USE_MSGBOX
	DECL_THEME(msgbox,              LV_THEME_MSGBOX),
	DECL_THEME(message-box,         LV_THEME_MSGBOX),
	DECL_THEME(msgbox-btns,         LV_THEME_MSGBOX_BTNS),
	DECL_THEME(message-box-btns,    LV_THEME_MSGBOX_BTNS),
	DECL_THEME(msgbox-buttons,      LV_THEME_MSGBOX_BTNS),
	DECL_THEME(message-box-btns,    LV_THEME_MSGBOX_BTNS),
	DECL_THEME(message-box-buttons, LV_THEME_MSGBOX_BTNS),
	#endif

	#if LV_USE_OBJMASK
	DECL_THEME(objmask,     LV_THEME_OBJMASK),
	DECL_THEME(obj-mask,     LV_THEME_OBJMASK),
	DECL_THEME(objectmask, LV_THEME_OBJMASK),
	DECL_THEME(object-mask, LV_THEME_OBJMASK),
	#endif

	#if LV_USE_PAGE
	DECL_THEME(page, LV_THEME_PAGE),
	#endif

	#if LV_USE_ROLLER
	DECL_THEME(roller, LV_THEME_ROLLER),
	#endif

	#if LV_USE_SLIDER
	DECL_THEME(slider, LV_THEME_SLIDER),
	#endif

	#if LV_USE_SPINBOX
	DECL_THEME(spinbox,        LV_THEME_SPINBOX),
	DECL_THEME(spinbox-btn,    LV_THEME_SPINBOX_BTN),
	DECL_THEME(spinbox-button, LV_THEME_SPINBOX_BTN),
	#endif

	#if LV_USE_SPINNER
	DECL_THEME(spinner, LV_THEME_SPINNER),
	#endif

	#if LV_USE_SWITCH
	DECL_THEME(switch, LV_THEME_SWITCH),
	#endif

	#if LV_USE_TABLE
	DECL_THEME(table, LV_THEME_TABLE),
	#endif

	#if LV_USE_TABVIEW
	DECL_THEME(tabview,      LV_THEME_TABVIEW),
	DECL_THEME(tabview-page, LV_THEME_TABVIEW_PAGE),
	#endif

	#if LV_USE_TEXTAREA
	DECL_THEME(text,     LV_THEME_TEXTAREA),
	DECL_THEME(textarea, LV_THEME_TEXTAREA),
	DECL_THEME(edit,     LV_THEME_TEXTAREA),
	DECL_THEME(edittext, LV_THEME_TEXTAREA),
	#endif

	#if LV_USE_TILEVIEW
	DECL_THEME(tileview, LV_THEME_TILEVIEW),
	#endif

	#if LV_USE_WIN
	DECL_THEME(win,           LV_THEME_WIN),
	DECL_THEME(win-btn,       LV_THEME_WIN_BTN),
	DECL_THEME(win-button,    LV_THEME_WIN_BTN),
	DECL_THEME(window,        LV_THEME_WIN),
	DECL_THEME(window-btn,    LV_THEME_WIN_BTN),
	DECL_THEME(window-button, LV_THEME_WIN_BTN),
	#endif
	{.valid=false,.name="",.theme=0}
};
#endif
#endif
