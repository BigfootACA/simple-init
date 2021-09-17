#include"shell_internal.h"
DECLARE_MAIN(adbd);
DECLARE_MAIN(arch);
DECLARE_MAIN(benchmark);
DECLARE_MAIN(cd);
DECLARE_MAIN(cat);
DECLARE_MAIN(chdir);
DECLARE_MAIN(clear);
DECLARE_MAIN(dmesg);
DECLARE_MAIN(dumpargv);
DECLARE_MAIN(logdumpargv);
DECLARE_MAIN(dumpenv);
DECLARE_MAIN(logdumpenv);
DECLARE_MAIN(exit);
DECLARE_MAIN(findfs);
DECLARE_MAIN(guiapp);
DECLARE_MAIN(help);
DECLARE_MAIN(hotplug);
DECLARE_MAIN(init);
DECLARE_MAIN(initctl);
DECLARE_MAIN(initconfd);
DECLARE_MAIN(initdevd);
DECLARE_MAIN(initloggerd);
DECLARE_MAIN(initshell);
DECLARE_MAIN(insmod);
DECLARE_MAIN(loggerctl);
DECLARE_MAIN(ls);
DECLARE_MAIN(lsmod);
DECLARE_MAIN(modprobe);
DECLARE_MAIN(mountpoint);
DECLARE_MAIN(uname);
DECLARE_MAIN(unlink);
DECLARE_MAIN(version);
DECLARE_MAIN(reboot);
DECLARE_MAIN(rmmod);
DECLARE_MAIN(setsid);
DECLARE_MAIN(service);
DECLARE_MAIN(true);
DECLARE_MAIN(false);
DECLARE_MAIN(halt);
DECLARE_MAIN(poweroff);

const struct shell_command*shell_cmds[]={
	#ifdef ENABLE_INITSHELL
	DECLARE_CMD(false, exit,        "Exit shell")
	DECLARE_CMD(true,  initshell,   "Simple init builtin shell")
	#endif
	#ifdef ENABLE_KMOD
	DECLARE_CMD(true,  insmod,      "Insert a module into the Linux Kernel")
	DECLARE_CMD(true,  lsmod,       "Show the status of modules in the Linux Kernel")
	DECLARE_CMD(true,  modprobe,    "Add and remove modules from the Linux Kernel")
	DECLARE_CMD(true,  rmmod,       "Remove a module from the Linux Kernel")
	#endif
	#ifdef ENABLE_BLKID
	DECLARE_CMD(true,  findfs,      "Find a filesystem by label or UUID")
	#endif
	#ifdef ENABLE_GUI
	DECLARE_CMD(true,  benchmark,   "GUI Benchmark")
	DECLARE_CMD(true,  guiapp,      "GUI Application")
	#endif
	DECLARE_CMD(true,  adbd,        "Android Debug Bridge Daemon")
	DECLARE_CMD(true,  arch,        "Print system architecture")
	DECLARE_CMD(true,  cat,         "Concatenate FILE(s) to standard output.")
	DECLARE_CMD(false, cd,          "Change directory")
	DECLARE_CMD(false, chdir,       "Change directory (Direct call)")
	DECLARE_CMD(false, clear,       "Clear screen")
	DECLARE_CMD(true,  dmesg,       "Print or control the kernel ring buffer")
	DECLARE_CMD(true,  dumpargv,    "Dump all arguments to stdout")
	DECLARE_CMD(true,  logdumpargv, "Dump all arguments to initloggerd")
	DECLARE_CMD(true,  loggerctl,   "Control init logger daemon")
	DECLARE_CMD(true,  dumpenv,     "Dump all environments variables to stdout")
	DECLARE_CMD(true,  logdumpenv,  "Dump all environments variables to initloggerd")
	DECLARE_CMD(true,  help,        "Show all shell builtin commands")
	DECLARE_CMD(true,  hotplug,     "Init simple device hotplug notifier")
	DECLARE_CMD(true,  init,        "Simple init")
	DECLARE_CMD(true,  initctl,     "Init controller")
	DECLARE_CMD(true,  initconfd,   "Init Config Daemon")
	DECLARE_CMD(true,  initdevd,    "Init simple device daemon")
	DECLARE_CMD(true,  initloggerd, "Launch simple init logger daemon")
	DECLARE_CMD(true,  ls,          "List directory contents")
	DECLARE_CMD(true,  mountpoint,  "Check whether a directory or file is a mountpoint")
	DECLARE_CMD(true,  uname,       "Print system information")
	DECLARE_CMD(true,  unlink,      "Remove a directory entry (Direct call)")
	DECLARE_CMD(true,  version,     "Show simple-init version")
	DECLARE_CMD(true,  reboot,      "Reboot the system")
	DECLARE_CMD(true,  setsid,      "Run a program in a new session.")
	DECLARE_CMD(true,  service,     "Control service operation (SysV Compatible)")
	DECLARE_CMD(true,  true,        "Always exit with 0 (true)")
	DECLARE_CMD(true,  false,       "Always exit with 1 (false)")
	DECLARE_CMD(true,  halt,        "Halt the system")
	DECLARE_CMD(true,  poweroff,    "Halt and shut off power")
	NULL
};
