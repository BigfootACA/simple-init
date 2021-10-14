#include"confd.h"

int cmdline_conffs(char*k __attribute__((unused)),char*v){
	confd_set_string("runtime.cmdline.conffs",v);
	return 0;
}

int cmdline_conffile(char*k __attribute__((unused)),char*v){
	confd_set_string("runtime.cmdline.conffile",v);
	return 0;
}
