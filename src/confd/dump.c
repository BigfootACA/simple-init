#define _GNU_SOURCE
#include<string.h>
#include"str.h"
#include"list.h"
#include"logger.h"
#include"confd_internal.h"
#define TAG "confd"

static int dump(struct conf*key,char*name){
	char path[PATH_MAX]={0};
	if(key->name[0]){
		if(!name[0])strcpy(path,key->name);
		else snprintf(path,PATH_MAX-1,"%s.%s",name,key->name);
	}
	if(key->type==TYPE_KEY){
		list*p=list_first(key->keys);
		if(!p)return 0;
		do{dump(LIST_DATA(p,struct conf*),path);}while((p=p->next));
	}else switch(key->type){
		case TYPE_STRING:  tlog_debug("  %s = \"%s\" (string)\n", path,VALUE_STRING(key));break;
		case TYPE_INTEGER: tlog_debug("  %s = %ld (integer)\n",   path,VALUE_INTEGER(key));break;
		case TYPE_BOOLEAN: tlog_debug("  %s = %s (boolean)\n",    path,BOOL2STR(VALUE_BOOLEAN(key)));break;
		default:           tlog_debug("  %s = (Unknown)\n",       path);break;
	}
	return 0;
}

int conf_dump_store(){
	tlog_debug("dump configuration store:");
	return dump(conf_get_store(),"");
}
