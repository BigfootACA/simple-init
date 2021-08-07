#include"init_internal.h"
#include"gadget.h"
#include"shell.h"
#include"boot.h"
#include"lvgl.h"
#include"gui.h"

int(*register_services[])()={
	#ifdef ENABLE_INITSHELL
	&register_console_shell,
	#endif
	&register_default_boot,
	&register_gadget_service,
	&register_guiapp,
	NULL
};

int init_register_all_service(){
	for(int i=0;register_services[i];i++)
		register_services[i]();
	return 0;
}
