#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include<blkid/blkid.h>
#include"defines.h"
#include"logger.h"
#include"keyval.h"
#include"cmdline.h"
#include"boot.h"
#define TAG "cmdline"

extern boot_config boot_switchroot;

static int _add_item(char*key,char*value){
	keyval**kv=boot_switchroot.data;
	for(int i=0;i<64;i++){
		if(!kv[i])ERET((kv[i]=kv_new_set_dup(key,value))?0:ENOMEM);
		else if(strcmp(key,kv[i]->key)==0){
			if(kv[i]->value)free(kv[i]->value);
			ERET((kv[i]->value=strdup(value))?0:ENOMEM);
		}
	}
	ERET(ENOSPC);
}

static int _xadd_item(char*k,char*key,char*value){
	return _add_item(key,value)<0?terlog_warn(-1,"set value %s failed",k):0;
}

int cmdline_root(char*k,char*v){
	if(!boot_options.config)boot_options.config=&boot_switchroot;
	char*p=blkid_evaluate_tag(v,NULL,NULL);
	if(!p)tlog_warn("unable to resolve %s",v);
	else tlog_debug("root block path set to %s",p);
	return _xadd_item(k,"path",p);
}

int cmdline_rootflags(char*k,char*v){
	tlog_debug("root block flags set to %s",v);
	return _xadd_item(k,"flags",v);
}

int cmdline_rootfstype(char*k,char*v){
	tlog_debug("root block type set to %s",v);
	return _xadd_item(k,"type",v);
}

int cmdline_rw(char*k,char*v __attribute__((unused))){
	tlog_debug("root block set to read-write");
	return _xadd_item(k,"rw","1");
}

int cmdline_ro(char*k,char*v __attribute__((unused))){
	tlog_debug("root block set to read-only");
	return _xadd_item(k,"rw","0");
}
