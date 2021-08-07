#include"str.h"
#include"gui.h"
#include"logger.h"
#define TAG "cmdline"

int cmdline_dpi(char*k __attribute__((unused)),char*v){
	int r=parse_int(v,0);
	if(r<0||r>1000)
		return trlog_warn(0,"invalid dpi '%s'",v);
	gui_dpi_def=r;
	return 0;
}

int cmdline_dpi_force(char*k __attribute__((unused)),char*v){
	int r=parse_int(v,0);
	if(r<0||r>1000)
		return trlog_warn(0,"invalid dpi_force '%s'",v);
	gui_dpi_force=r;
	return 0;
}
