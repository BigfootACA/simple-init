#include<errno.h>
#include<stdlib.h>
#include<string.h>
#ifdef ENABLE_BLKID
#include<blkid/blkid.h>
#endif
#include"defines.h"
#include"logger.h"
#include"cmdline.h"
#define TAG "cmdline"

int cmdline_logfs(char*k __attribute__((unused)),char*v){
	#ifdef ENABLE_BLKID
	char*p=blkid_evaluate_tag(v,NULL,NULL);
	if(!p)tlog_warn("unable to resolve %s",v);
	else{
		tlog_debug("logfs block path set to %s",p);
		strncpy(boot_options.logfs_block,p,63);
		free(p);
	}
	#else
	if(v[0]!='/')tlog_alert("evaluate tag support is disabled");
	else strncpy(boot_options.logfs_block,v,63);
	#endif
	return 0;
}

int cmdline_logfile(char*k __attribute__((unused)),char*v){
	strncpy(boot_options.logfs_file,v,63);
	return 0;
}