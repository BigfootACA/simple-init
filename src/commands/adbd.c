/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<getopt.h>
#include<fcntl.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<strings.h>
#include<stdbool.h>
#include"defines.h"
#include"output.h"
#include"keyval.h"
#include"adbd.h"
static struct adb_data data;
static int usage(){
	return re_printf(2,
		"Android Debug Bridge Daemon for Embedded Linux\n"
		"\n"
		"Usage: \n"
		"  adbd [--auth] [--daemon] [--shell SHELL] [--banner BANNER] \n"
		"       [--devname DEVICE_NAME] [--devmodel DEVICE_MODEL] \n"
		"       [--devproduct DEVICE_PRODUCT] [--usb|--tcp]\n"
		"\n"
		"Defaults: \n"
		"  BANNER     : \"%s\"\n"
		"  DEVNAME    : \"%s\"\n"
		"  DEVMODEL   : \"%s\"\n"
		"  DEVPRODUCT : \"%s\"\n"
		"  SHELL      : env var 'SHELL' or initshell\n"
		"\n"
		"Values: \n"
		"  BANNER     : \"bootloader\", \"sideload\", \"recovery\", \"device\"\n"
		"  DEVNAME    : string (A-Z, a-z, 0-9, '-', '_')\n"
		"  DEVMODEL   : string (A-Z, a-z, 0-9, '-', '_')\n"
		"  DEVPRODUCT : string (A-Z, a-z, 0-9, '-', '_')\n"
		"  SHELL      : executable file path. (null for initshell)\n"
		"  PROTOCOL   : force protocol. (usb or tcp)\n"
		"\n"
		"Options: \n"
		"\t --help                  , -h            : show this help.\n"
		"\t --protocol PROTOCOL     , -P PROTOCOL   : set protocol.\n"
		"\t --usb                   , -u            : set to USB protocol.\n"
		"\t --tcp                   , -t            : set to TCP protocol.\n"
		"\t --daemon                , -d            : run in daemon mode.\n"
		"\t --auth                  , -a            : turn on adbd need auth.\n"
		"\t --devname DEVNAME       , -n DEVNAME    : device name. (ro.product.name)\n"
		"\t --devmodel DEVMODEL     , -m DEVMODEL   : device model. (ro.product.model)\n"
		"\t --devproduct DEVPRODUCT , -p DEVPRODUCT : device product. (ro.product.device)\n"
		"\t --banner BANNER         , -b BANNER     : device banner show in 'adb devices'.\n"
		"\t --shell SHELL           , -s SHELL      : system shell. (adb shell)\n"
		"\n"
		"Support Protocols: usb, tcp",
		data.banner,
		kvlst_get(data.prop,"ro.product.name","unknown"),
		kvlst_get(data.prop,"ro.product.model","unknown"),
		kvlst_get(data.prop,"ro.product.devices","unknown")
	);
}
int adbd_main(int argc,char**argv){
	int o;
	const char*sopts="utrdahb:n:m:p:s:P:";
	const struct option lopts[]={
		{"daemon",     no_argument,       NULL, 'd'},
		{"auth",       no_argument,       NULL, 'a'},
		{"help",       no_argument,       NULL, 'h'},
		{"tcp",        no_argument,       NULL, 't'},
		{"usb",        no_argument,       NULL, 'u'},
		{"protocol",   required_argument, NULL, 'P'},
		{"banner",     required_argument, NULL, 'b'},
		{"devname",    required_argument, NULL, 'n'},
		{"devmodel",   required_argument, NULL, 'm'},
		{"devproduct", required_argument, NULL, 'p'},
		{"shell",      required_argument, NULL, 's'},
		{NULL, 0, NULL, 0}
	};
	bool daemon=false;
	init_adb_data(&data);
	while((o=b_getlopt(argc,argv,sopts,lopts,NULL))!=-1)switch(o){
		case 'h':return usage();
		case 'd':daemon=true;break;
		case 'a':data.auth_enabled=1;break;
		case 's':
			memset(data.shell,0,sizeof(data.shell));
			strncpy(data.shell,b_optarg,sizeof(data.shell)-1);
		break;
		case 'b':
			if(
				strcasecmp(b_optarg,"device")!=0&&
				strcasecmp(b_optarg,"sideload")!=0&&
				strcasecmp(b_optarg,"recovery")!=0&&
				strcasecmp(b_optarg,"bootloader")!=0
			)return re_printf(2,"Invalid argument '%s' for --%s\n",b_optarg,"banner");
			memset(data.banner,0,sizeof(data.banner));
			strncpy(data.banner,b_optarg,sizeof(data.banner)-1);
		break;
		case 'n':data.prop=kvlst_set(data.prop,"ro.product.name",b_optarg);break;
		case 'm':data.prop=kvlst_set(data.prop,"ro.product.model",b_optarg);break;
		case 'p':data.prop=kvlst_set(data.prop,"ro.product.device",b_optarg);break;
		case 'u':data.proto=PROTO_USB;break;
		case 't':data.proto=PROTO_TCP;break;
		case 'P':
			if(strcasecmp(b_optarg,"tcp")==0)data.proto=PROTO_TCP;
			else if(strcasecmp(b_optarg,"usb")==0)data.proto=PROTO_USB;
			else return re_printf(2,"Invalid argument '%s' for --%s\n",b_optarg,"protocol");
		break;
		default:return re_printf(2,"Unknown argument: %c\n",(char)o);
	}
	if(data.shell[0]&&access(data.shell,X_OK)!=0){
		perror(_("no executable shell found"));
		fprintf(stderr,_("use initshell"));
		memset(data.shell,0,sizeof(data.shell));
	}
	if(daemon){
		pid_t p=fork();
		if(p>0)return ro_printf(0,"daemon run with pid %d.\n",(int)p);
		else if(p==0)setsid();
		else if(p<0)return re_err(1,"fork");
	}
	return adbd_init(&data);
}
