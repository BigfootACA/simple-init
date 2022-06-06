/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<fcntl.h>
#include<errno.h>
#include<unistd.h>
#include<string.h>
#include<sys/stat.h>
#include"cmdline.h"
#include"assets.h"
#include"confd.h"
#include"devd.h"
#include"system.h"
#include"init.h"
#include"logger.h"
#include"shell.h"
#include"pathnames.h"
#include"language.h"
#include"hardware.h"
#define TAG "preinit"

static bool need_extract_rootfs(){
	if(access(_PATH_ETC,F_OK)!=0&&errno==ENOENT)return true;

	struct mount_item**ms=read_proc_mounts(),*m;
	if(!ms)return false;
	bool need=false;
	for(int i=0;(m=ms[i]);i++){
		if(strcmp(m->target,"/")!=0)continue;
		tlog_debug("root filesystem is %s",m->type);
		if(strcmp(m->type,"rootfs")==0)need=true;
		if(strcmp(m->type,"tmpfs")==0)need=true;
		break;
	}
	free_mounts(ms);
	return need;
}

int preinit(){

	// ensure important folder exists
	mkdir(_PATH_PROC,755);
	mkdir(_PATH_SYS,755);
	mkdir(_PATH_DEV,755);
	mkdir(_PATH_RUN,755);
	mkdir(_PATH_TMP,1777);

	tlog_debug("preinit mountpoints");
	if(is_folder(_PATH_SYS_DEV))tlog_warn("%s already mounted",_PATH_SYS);
	else exmount("sys",_PATH_SYS,"sysfs","rw,nosuid,noexec,nodev");
	if(is_folder(_PATH_PROC"/1"))tlog_warn("%s already mounted",_PATH_PROC);
	else exmount("proc",_PATH_PROC,"proc","rw,nosuid,noexec,nodev");
	xmount(false,"run",_PATH_RUN,"tmpfs","rw,nosuid,nodev,mode=755",true);
	xmount(false,"tmp",_PATH_TMP,"tmpfs","rw,nosuid,nodev,mode=1777",true);
	if(access(_PATH_PROC_CMDLINE,R_OK)!=0)
		return terlog_error(2,"failed to find proc mountpoint");

	// vibrate device
	vibrate(100);

	// init empty rootfs
	if(need_extract_rootfs()){
		int dfd;
		if((dfd=open(_PATH_ROOT,O_DIR))>0){
			create_assets_dir(dfd,&assets_rootfs,false);
			tlog_debug("extract assets done");
			lang_init_locale();
			close(dfd);
		}
		if((dfd=open(_PATH_USR_BIN,O_DIR))>0){
			install_cmds(dfd);
			tlog_debug("install commands done");
			close(dfd);
		}
	}

	// tel loggerd to listen socket
	logger_listen_default();
	close_logfd();
	open_socket_logfd_default();
	chmod(DEFAULT_LOGGER,0600);
	chown(DEFAULT_LOGGER,0,0);

	// start config daemon
	if(start_confd(TAG,NULL)<0){
		tlog_emerg("start config daemon failed");
		abort();
	}

	// create config runtime root key
	confd_add_key("runtime");
	confd_set_save("runtime",false);

	// create initial boot configs
	boot_init_configs();

	// load cmdline
	load_cmdline();

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

	mkdir(_PATH_DEV_PTS,0755);
	xmount(false,"devpts",_PATH_DEV_PTS,"devpts","rw,nosuid,noexec,mode=620,ptmxmode=000",true);
	mkdir(_PATH_DEV_SHM,1777);
	xmount(false,"shm",_PATH_DEV_SHM,"tmpfs","rw,nosuid,nodev",true);
	mkdir(_PATH_DEV_MQUEUE,1777);
	xmount(false,"mqueue",_PATH_DEV_MQUEUE,"mqueue","rw,nosuid,nodev,noexec",false);
	mkdir(_PATH_DEV_HUGEPAGES,1777);
	xmount(false,"hugetlbfs",_PATH_DEV_HUGEPAGES,"hugetlbfs","rw",false);
	mkdir(_PATH_DEV"/binderfs",0755);
	xmount(false,"binder",_PATH_DEV"/binderfs","binder","rw",false);
	xmount(false,"bpf",_PATH_SYS_FS"/bpf","bpf","nosuid,nodev,noexec",false);
	xmount(false,"pstore",_PATH_SYS_FS"/pstore","pstore","nosuid,nodev,noexec",false);
	xmount(false,"resctrl",_PATH_SYS_FS"/resctrl","resctrl","nosuid,nodev,noexec",false);
	xmount(false,"cgroup2",_PATH_SYS_FS"/cgroup","cgroup2","nosuid,nodev,noexec",false);
	xmount(false,"fusectl",_PATH_SYS_FS"/fuse/connections","fusectl","nosuid,nodev,noexec",false);
	xmount(false,"configfs",_PATH_SYS_KERNEL"/config","configfs","nosuid,nodev,noexec",false);
	xmount(false,"debugfs",_PATH_SYS_KERNEL"/debug","debugfs","nosuid,nodev,noexec",false);
	xmount(false,"tracefs",_PATH_SYS_KERNEL"/tracing","tracefs","nosuid,nodev,noexec",false);
	xmount(false,"securityfs",_PATH_SYS_KERNEL"/security","securityfs","nosuid,nodev,noexec",false);
	xmount(false,"efivarfs",_PATH_SYS_FIRMWARE"/efi/efivars","efivarfs","nosuid,nodev,noexec",false);
	xmount(false,"binfmt_misc",_PATH_PROC"/fs/binfmt_misc","binfmt_misc","nosuid,nodev,noexec",false);
	xmount(false,"nfsd",_PATH_PROC"/fs/nfsd","nfsd",NULL,false);

	// start devd daemon
	start_devd(TAG,NULL);

	// create all device nodes
	devd_call_init();

	// open active consoles
	logger_open_console();

	// read kmsg to logger
	logger_klog();

	// start syslog forwarder
	logger_syslog();

	// open /dev/logger.log
	char*dev_logger=_PATH_DEV"/logger.log";
	logger_open(dev_logger);
	chmod(dev_logger,0600);
	chown(dev_logger,0,0);

	// load modules from list config
	devd_call_modload();

	// load all modules
	devd_call_modalias();

	// setup logfs and save log
	setup_logfs();

	// setup conffs and load conf
	setup_conffs();

	return 0;
}
