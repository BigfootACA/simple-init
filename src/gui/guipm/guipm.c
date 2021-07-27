#include"guipm.h"

void guipm_draw_title(lv_obj_t*screen){
	lv_obj_t*title=lv_label_create(screen,NULL);
	lv_label_set_long_mode(title,LV_LABEL_LONG_BREAK);
	lv_obj_set_size(title,w,h/16);
	lv_obj_set_y(title,16);
	lv_label_set_align(title,LV_LABEL_ALIGN_CENTER);
	lv_label_set_text(title,_("Partition Manager"));
}

static void _draw(lv_obj_t*scr){
	guipm_draw_disk_sel(scr);
}

int guipm_main(int argc __attribute((unused)),char**argv __attribute((unused))){
	open_socket_logfd_default();
	return gui_init(_draw);
}