#include<fcntl.h>
#include<errno.h>
#include<unistd.h>
#include<sys/stat.h>
#include"cmdline.h"
#include"assets.h"
#include"devd.h"
#include"system.h"
#include"init.h"
#include"logger.h"
#include"shell.h"
#include"pathnames.h"
#define TAG "preinit"

int preinit(){

	// ensure important folder exists
	mkdir(_PATH_PROC,755);
	mkdir(_PATH_SYS,755);
	mkdir(_PATH_DEV,755);
	mkdir(_PATH_RUN,755);
	mkdir(_PATH_TMP,1777);

	tlog_debug("preinit mountpoints");
	exmount("sys",_PATH_SYS,"sysfs","rw,nosuid,noexec,nodev");
	exmount("proc",_PATH_PROC,"proc","rw,nosuid,noexec,nodev");
	exmount("run",_PATH_RUN,"tmpfs","rw,nosuid,nodev,mode=755");

	// init empty rootfs
	if(access(_PATH_ETC,F_OK)!=0){
		int dfd;
		if(errno==ENOENT){
			if((dfd=open(_PATH_ROOT,O_DIRECTORY|O_RDONLY))>0){
				create_assets_dir(dfd,&assets_rootfs,false);
				tlog_debug("extract assets done");
				close(dfd);
			}
			if((dfd=open(_PATH_USR_BIN,O_DIRECTORY|O_RDONLY))>0){
				install_cmds(dfd);
				tlog_debug("install commands done");
				close(dfd);
			}
		}else return terlog_emerg(-errno,"cannot access "_PATH_ETC);
	}

	// tel loggerd to listen socket
	logger_listen_default();
	close_logfd();
	open_socket_logfd_default();
	chmod(DEFAULT_LOGGER,0600);
	chown(DEFAULT_LOGGER,0,0);

	// setup hotplug helper
	simple_file_write(_PATH_PROC_SYS"/kernel/hotplug",_PATH_USR_BIN"/initdevd");

	// init /dev
	if(xmount(false,"dev",_PATH_DEV,"devtmpfs","rw,nosuid,noexec,mode=755",false)!=0)switch(errno){
		case EBUSY:tlog_info("devtmpfs already mounted.");break;
		case ENODEV:
			tlog_warn("your kernel seems not support devtmpfs.");
			if(xmount(true,"dev",_PATH_DEV,"tmpfs","rw,nosuid,noexec,mode=755",false)!=0)
				return terlog_emerg(errno,"failed to mount tmpfs on "_PATH_DEV);
		break;
		default:return terlog_warn(errno,"failed to mount devtmpfs on "_PATH_DEV);
	}

	// start devd daemon
	start_devd(TAG,NULL);

	// create all device nodes
	devd_call_init();

	// open /dev/logger.log
	char*dev_logger=_PATH_DEV"/logger.log";
	logger_open(dev_logger);
	chmod(dev_logger,0600);
	chown(dev_logger,0,0);

	// read kmsg to logger
	logger_klog();

	#ifdef ENABLE_KMOD
	// load all modules
	devd_call_modalias();
	#endif

	// load cmdline
	if(access(_PATH_PROC_CMDLINE,R_OK)!=0)return terlog_error(2,"failed to find proc mountpoint");
	load_cmdline();

	// setup logfs and save log
	setup_logfs();

	return 0;
}