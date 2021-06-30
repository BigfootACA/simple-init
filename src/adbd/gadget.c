#include<unistd.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/mount.h>
#include<sys/prctl.h>
#include"adbd_internal.h"
#include"proctitle.h"
#include"system.h"
#include"gadget.h"
#include"logger.h"
#define TAG "adbd"

int gadget_add_func_adbd(gadget*gadget,char*name,char*path){
	struct adb_data data;
	gadget_func func;
	init_adb_data(&data);
	memset(&func,0,sizeof(func));
	func.function="ffs";
	func.name=name;
	if(gadget_add_function(gadget,&func)<0)
		return terlog_warn(-1,"add adbd gadget function failed");
	if(mkdir(path,0755)<0&&errno!=EEXIST)
		return terlog_warn(-1,"adbd gadget ffs mkdir %s failed",path);
	if(mount(name,path,"functionfs",0,NULL)<0)
		return terlog_warn(-1,"adbd gadget ffs mount %s failed",path);
	data.proto=PROTO_USB;
	strcpy(data.ffs,path);
	pid_t p=fork();
	if(p<0)return terlog_warn(-1,"adbd fork failed");
	else if(p==0){
		close_all_fd(NULL,0);
		open_socket_logfd_default();
		prctl(PR_SET_NAME,"ADB Daemon");
		setproctitle("initadbd");
		_exit(adbd_init(&data));
	}
	kvlst_free(data.prop);
	tlog_info("adbd gadget initialized");
	return 0;
}
