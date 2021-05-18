#include<stddef.h>
#include"pathnames.h"
#include"cmdline.h"
#define DEF_HANDLER(name)extern int cmdline_##name(char*,char*)
#define DEF_OPTION(_name,_always,_type)&(struct cmdline_option){.name=#_name,.type=(_type),.always=(_always),.handler=&cmdline_##_name}

DEF_HANDLER(rw);
DEF_HANDLER(ro);
DEF_HANDLER(end);
DEF_HANDLER(verbose);
DEF_HANDLER(init);
DEF_HANDLER(root);
DEF_HANDLER(rootflags);
DEF_HANDLER(rootfstype);
DEF_HANDLER(rootwait);
DEF_HANDLER(logfs);
DEF_HANDLER(logfile);

struct cmdline_option*cmdline_options[]={

	// root.c; rootfs for switchroot
	DEF_OPTION(rw,         false, NO_VALUE),
	DEF_OPTION(ro,         false, NO_VALUE),
	DEF_OPTION(init,       false, REQUIRED_VALUE),
	DEF_OPTION(root,       false, REQUIRED_VALUE),
	DEF_OPTION(rootflags,  false, REQUIRED_VALUE),
	DEF_OPTION(rootfstype, false, REQUIRED_VALUE),
	DEF_OPTION(rootwait,   false, OPTIONAL_VALUE),

	// cmdline.c; end parse general options
	DEF_OPTION(end,        false, OPTIONAL_VALUE),

	// cmdline.c; verbose log
	DEF_OPTION(verbose,    false, OPTIONAL_VALUE),

	// logfs.c; logger persistent storage
	DEF_OPTION(logfs,      false, REQUIRED_VALUE),
	DEF_OPTION(logfile,    false, REQUIRED_VALUE),
	NULL
};

struct boot_options boot_options={

};

char*init_list[]={
	// legacy init
	_PATH_SBIN"/init",
	_PATH_BIN"/init",
	_PATH_USR_SBIN"/init",
	_PATH_USR_BIN"/init",
	_PATH_USR_LOCAL_SBIN"/init",
	_PATH_USR_LOCAL_BIN"/init",

	// systemd
	_PATH_LIB"/systemd/systemd",
	_PATH_USR_LIB"/systemd/systemd",
	_PATH_USR_LOCAL_LIB"/systemd/systemd",

	// initramfs
	"/init",
	"/linuxrc",

	// shell
	_PATH_BIN"/bash",
	_PATH_USR_BIN"/bash",
	_PATH_USR_LOCAL_BIN"/bash"
	_PATH_BIN"/sh",
	_PATH_USR_BIN"/sh",
	_PATH_USR_LOCAL_BIN"/sh",

	NULL
};

char*firmware_list[32]={
	"/vendor/firmware_mnt/image",
	"/vendor/bt_firmware/image",
	"/vendor/firmware",
	_PATH_USR_LOCAL_LIB"/firmware",
	_PATH_LIB"/firmware",
	_PATH_USR_LIB"/firmware",
	NULL
};
