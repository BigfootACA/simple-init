#include"confd.h"

int cmdline_logfs(char*k __attribute__((unused)),char*v){
	confd_set_string("runtime.cmdline.logfs",v);
	return 0;
}

int cmdline_logfile(char*k __attribute__((unused)),char*v){
	confd_set_string("runtime.cmdline.logfile",v);
	return 0;
}
