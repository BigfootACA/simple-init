#define _GNU_SOURCE
#include<string.h>
#include<sys/types.h>
#include"confd.h"
#include"cmdline.h"
#define TAG "cmdline"

extern boot_config boot_charger;
int cmdline_androidboot(char*k,char*v){
	char*base="cmdline",*conf;
	if(strncmp(k,"androidboot.",12)!=0)return 0;
	k+=12;
	if(strcmp(k,"mode")==0){
		if(strcmp(v,"charger")==0)boot_options.config=&boot_charger;
		conf=k;
	}else if(strcmp(k,"usbcontroller")==0)conf="udc";
	else if(strcmp(k,"serialno")==0)conf="serial";
	else if(strcmp(k,"bootdevice")==0)conf="disk";
	else if(strlen(k)>0)conf=k;
	confd_set_string_base(base,conf,v);
	return 0;
}
