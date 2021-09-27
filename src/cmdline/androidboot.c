#define _GNU_SOURCE
#include<string.h>
#include<sys/types.h>
#include"confd.h"
#include"cmdline.h"
#define TAG "cmdline"

extern boot_config boot_charger;
int cmdline_androidboot_mode(char*k __attribute__((unused)),char*v){
	if(strcmp(v,"charger")==0)boot_options.config=&boot_charger;
	confd_set_string("cmdline.mode",v);
	return 0;
}
