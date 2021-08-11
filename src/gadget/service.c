#define _GNU_SOURCE
#include<string.h>
#include"service.h"
#include"version.h"
#include"gadget.h"
#include"logger.h"
#include"adbd.h"
#define TAG "gadget"

static struct gadget g={
	.name    = "gadget",
	.vendor  = 0x05F9,
	.product = 0x5016,
	.strings = GADGET_STRARRAY{GADGET_STR(0x409,"Linux",NAME,"1234567890"),NULL},
	.configs = GADGET_CFGARRAY{
		&(gadget_cfg){
			.strings=GADGET_CFGSTRARRAY{GADGET_CFGSTR(0x409,NAME),NULL},
			.max_power=500,
			.type="a",
			.id=1
		},NULL
	}
};

static int gadget_startup(struct service*svc __attribute__((unused))){
	tlog_info("register gadget");
	if(gadget_register(&g)<0)
		return trlog_error(-1,"failed to register gadget");
	if(gadget_add_function(&g,&(gadget_func){
		.name="usb0",
		.function="rndis"
	})<0)tlog_warn("failed to add rndis function");
	if(gadget_add_function(&g,&(gadget_func){
		.name="ttyGS0",
		.function="acm"
	})<0)tlog_warn("failed to add serial function");
	if(gadget_add_func_adbd(&g,"adb",_PATH_RUN"/adb")<0)
		tlog_warn("failed to add adbd function");
	if(gadget_start_auto(&g)<0)
		return trlog_error(-1,"failed to start gadget");
	tlog_info("usb gadget initialized");
	return 0;
}

int register_gadget_service(){
	struct service*gadget=svc_create_service("usb-gadget",WORK_ONCE);
	if(gadget){
		svc_set_desc(gadget,"USB Gadget Startup");
		svc_set_start_function(gadget,gadget_startup);
		svc_add_depend(svc_system,gadget);
	}
	return 0;
}
