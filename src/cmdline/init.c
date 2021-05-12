#include<errno.h>
#include<string.h>
#include"defines.h"
#include"logger.h"
#include"cmdline.h"
#define TAG "cmdline"

int cmdline_init(char*k __attribute__((unused)),char*v){
	if(!v[0]||v[0]!='/')return trlog_error(ENUM(EINVAL),"invalid init %s",v);
	if(!(boot_options.init=strdup(v)))return -errno;
	tlog_debug("init set to %s",v);
	return 0;
}