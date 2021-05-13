#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include<blkid/blkid.h>
#include"defines.h"
#include"logger.h"
#include"cmdline.h"
#define TAG "cmdline"

int cmdline_logfs(char*k __attribute__((unused)),char*v){
	char*p=blkid_evaluate_tag(v,NULL,NULL);
	if(!p)tlog_warn("unable to resolve %s",v);
	else{
		tlog_debug("logfs block path set to %s",p);
		strncpy(boot_options.logfs_block,p,63);
		free(p);
	}
	return 0;
}

int cmdline_logfile(char*k __attribute__((unused)),char*v){
	strncpy(boot_options.logfs_file,v,63);
	return 0;
}