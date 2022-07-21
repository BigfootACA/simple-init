/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"gui/activity.h"
extern struct gui_register guireg_linux_simple_mass_storage;
extern struct gui_register guireg_guipm_disk_select;
extern struct gui_register guireg_guipm_partitions;
extern struct gui_register guireg_picture_viewer;
extern struct gui_register guireg_uefi_bootmenu;
extern struct gui_register guireg_uefi_dxe_load;
extern struct gui_register guireg_pointer_test;
extern struct gui_register guireg_acpi_manager;
extern struct gui_register guireg_add_ramdisk;
extern struct gui_register guireg_screen_test;
extern struct gui_register guireg_serial_open;
extern struct gui_register guireg_boot_linux;
extern struct gui_register guireg_uefi_start;
extern struct gui_register guireg_uefi_shell;
extern struct gui_register guireg_mouse_menu;
extern struct gui_register guireg_theme_menu;
extern struct gui_register guireg_acpi_load;
extern struct gui_register guireg_text_edit;
extern struct gui_register guireg_conf_save;
extern struct gui_register guireg_conf_load;
extern struct gui_register guireg_benchmark;
extern struct gui_register guireg_backlight;
extern struct gui_register guireg_logviewer;
extern struct gui_register guireg_clipboard;
extern struct gui_register guireg_add_mount;
extern struct gui_register guireg_add_mass;
extern struct gui_register guireg_fdt_load;
extern struct gui_register guireg_vibrator;
extern struct gui_register guireg_language;
extern struct gui_register guireg_conftool;
extern struct gui_register guireg_calendar;
extern struct gui_register guireg_abootimg;
extern struct gui_register guireg_terminal;
extern struct gui_register guireg_bootmgr;
extern struct gui_register guireg_filemgr;
extern struct gui_register guireg_regedit;
extern struct gui_register guireg_gadget;
extern struct gui_register guireg_guiapp;
extern struct gui_register guireg_mount;
struct gui_register*guiact_register[]={
	#ifdef ENABLE_UEFI
	#ifdef ENABLE_LINUX_SIMPLE_MASS_STORAGE
	&guireg_linux_simple_mass_storage,
	#endif
	&guireg_uefi_bootmenu,
	&guireg_uefi_dxe_load,
	&guireg_acpi_manager,
	&guireg_add_ramdisk,
	&guireg_uefi_start,
	&guireg_uefi_shell,
	&guireg_mouse_menu,
	&guireg_boot_linux,
	&guireg_acpi_load,
	&guireg_fdt_load,
	#else
	&guireg_guipm_disk_select,
	&guireg_guipm_partitions,
	#ifdef ENABLE_LIBTSM
	&guireg_terminal,
	#endif
	&guireg_backlight,
	&guireg_add_mount,
	&guireg_vibrator,
	&guireg_add_mass,
	&guireg_gadget,
	&guireg_mount,
	#endif
	#ifdef ENABLE_HIVEX
	&guireg_regedit,
	#endif
	#ifdef ENABLE_LIBTSM
	&guireg_serial_open,
	#endif
	&guireg_picture_viewer,
	&guireg_pointer_test,
	&guireg_screen_test,
	&guireg_theme_menu,
	&guireg_benchmark,
	&guireg_text_edit,
	&guireg_conf_save,
	&guireg_conf_load,
	&guireg_clipboard,
	&guireg_logviewer,
	&guireg_language,
	&guireg_calendar,
	&guireg_abootimg,
	&guireg_conftool,
	&guireg_bootmgr,
	&guireg_filemgr,
	&guireg_guiapp,
	NULL
};
#endif
