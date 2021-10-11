#ifdef ENABLE_GUI
#include"gui/activity.h"
extern struct gui_register guireg_guipm_disk_select;
extern struct gui_register guireg_guipm_partitions;
extern struct gui_register guireg_gadget_base_info;
extern struct gui_register guireg_uefi_bootmenu;
extern struct gui_register guireg_uefi_shell;
extern struct gui_register guireg_mouse_menu;
extern struct gui_register guireg_theme_menu;
extern struct gui_register guireg_filepicker;
extern struct gui_register guireg_text_edit;
extern struct gui_register guireg_benchmark;
extern struct gui_register guireg_backlight;
extern struct gui_register guireg_logviewer;
extern struct gui_register guireg_vibrator;
extern struct gui_register guireg_language;
extern struct gui_register guireg_inputbox;
extern struct gui_register guireg_conftool;
extern struct gui_register guireg_filemgr;
extern struct gui_register guireg_gadget;
extern struct gui_register guireg_msgbox;
extern struct gui_register guireg_reboot;
extern struct gui_register guireg_guiapp;
struct gui_register*guiact_register[]={
	#ifdef ENABLE_UEFI
	&guireg_uefi_bootmenu,
	&guireg_uefi_shell,
	&guireg_mouse_menu,
	#else
	#ifdef ENABLE_FDISK
	&guireg_guipm_disk_select,
	&guireg_guipm_partitions,
	#endif
	&guireg_backlight,
	&guireg_logviewer,
	&guireg_vibrator,
	&guireg_gadget,
	&guireg_gadget_base_info,
	#endif
	&guireg_theme_menu,
	&guireg_filepicker,
	&guireg_benchmark,
	&guireg_text_edit,
	&guireg_language,
	&guireg_conftool,
	&guireg_inputbox,
	&guireg_filemgr,
	&guireg_msgbox,
	&guireg_reboot,
	&guireg_guiapp,
	NULL
};
#endif
