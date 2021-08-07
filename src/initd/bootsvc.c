#include"init_internal.h"
#include"gadget.h"
#include"shell.h"
#include"boot.h"
#ifdef ENABLE_GUI
#include"gui.h"
#endif

int(*register_services[])()={
	#ifdef ENABLE_INITSHELL
	&register_console_shell,
	#endif
	&register_default_boot,
	&register_gadget_service,
	#ifdef ENABLE_GUI
	&register_guiapp,
	#endif
	NULL
};

int init_register_all_service(){
	for(int i=0;register_services[i];i++)
		register_services[i]();
	return 0;
}
