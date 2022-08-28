/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef SYSTEM_H
#define SYSTEM_H
#include<stdbool.h>
#include<dirent.h>
#include<signal.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/socket.h>

// mount item
struct mount_item{
	char*source,*target,*type,**options;
	int freq,passno;
};
// src/lib/signal.c: signal name map (eg. 2 = "SIGINT")
extern const char*sigmap[];

// src/lib/mount.c: read all mountpoint from /proc/mounts
extern struct mount_item**read_proc_mounts(void);

// src/lib/mount.c: is 'path' a mountpoint
extern bool is_mountpoint(char*path);

// src/lib/mount.c: free single mount_item
extern void free_mount_item(struct mount_item*m);

// src/lib/mount.c: free mount_item array
extern void free_mounts(struct mount_item**c);

// src/lib/file.c: check file name is "." or ".."
extern bool is_virt_dir(const struct dirent*d);

// src/lib/file.c: get file path of an opened fd
extern ssize_t get_fd_path(int fd,char*buff,size_t size);

// src/lib/file.c: check path is an invalid path
extern bool is_invalid_path(char*path);

// src/lib/file.c: wait for a file exists
extern int wait_exists(char*path,long time,long step);

// src/lib/file.c: recursive mkdir (mkdir -p)
extern int mkdir_res(char*path);

// src/lib/file.c: create a symbolic link
extern int add_link(char*path,char*name,char*dest);

// src/lib/stdio.c: get terminal width
extern int get_term_width(int fd,int def);

// src/lib/credential.c: get username by uid, return uid if fail
extern char*get_username(uid_t uid,char*buff,size_t size);

// src/lib/credential.c: get groupname by gid, return gid if fail
extern char*get_groupname(gid_t gid,char*buff,size_t size);

// src/lib/credential.c: get process comm name by pid, return pid if fail
extern char*get_commname(pid_t pid,char*buff,size_t size,bool with_pid);

#ifndef ENABLE_UEFI
#ifdef _GNU_SOURCE
// src/lib/credential.c: convert ucred to string
extern char*ucred2string(struct ucred*c,char*buff,size_t size,bool with_pid);
#endif

// src/lib/modules.c: lookup and load module by alias
extern int insmod(const char*alias,bool log);
#endif

// src/lib/file.c: remove all sub folders (depth 1)
extern int remove_folders(int dfd,int flags);

// src/lib/file.c: check path is type
extern bool fd_is_type(int fd,int err,mode_t type,const char*path,...);

// src/lib/file.c: read an integer value from a file
extern int fd_read_int(int fd,char*name);

// src/lib/file.c: write an integer value to a file
extern int fd_write_int(int fd,char*name,int value,bool lf);

// src/lib/file.c: get a usable loop device
extern int loop_get_free_num(void);

// src/lib/file.c: get a usable loop device path
extern int loop_get_free(char*buf,size_t len);

enum reboot_cmd{
	REBOOT_NONE=0,
	REBOOT_HALT,
	REBOOT_KEXEC,
	REBOOT_POWEROFF,
	REBOOT_RESTART,
	REBOOT_SUSPEND,
	REBOOT_WARM,
	REBOOT_COLD,
	REBOOT_DATA,
	REBOOT_RECOVERY,
	REBOOT_FASTBOOT,
	REBOOT_EDL,
};

// src/lib/reboot.c: advance reboot (with arg)
extern int adv_reboot(enum reboot_cmd cmd,char*data);

#ifndef ENABLE_UEFI
// src/lib/mount.c: call libmount to mount
extern int xmount(
	bool ex,
	const char*dev,
	const char*dir,
	const char*type,
	const char*data,
	bool warning
);

// src/lib/mount.c: get a unused mountpoint
extern char*auto_mountpoint(char*path,size_t len);

// src/lib/mount.c: lock and unlock mountpoint folder
extern void mountpoint_locker(bool lock);

// src/lib/mount.c: auto mount and reuse
extern int auto_mount(const char*source,const char*type,char*target,size_t len);
#endif

// src/lib/file.c: one line write file
extern int write_file(
	int dir,
	char*file,
	const char*str,
	size_t len,
	mode_t mode,
	bool lf,
	bool create,
	bool trunc
);

// src/lib/file.c: one line simple write file
extern int simple_file_write(char*file,char*content);

// src/lib/file.c: one line simple append file
extern int simple_file_append(char*file,char*content);

// src/lib/file.c: check block or tag is exists
extern int has_block(char*block);

// src/lib/file.c: wait block or tag exists
extern int wait_block(char*block,long time,char*tag);

// src/lib/file.c: one line read file
extern ssize_t read_file(char*buff,size_t len,bool lf,char*path,...);

// src/lib/file.c: one line read file at fd
extern ssize_t fd_read_file(int fd,char*buff,size_t len,bool lf,char*path,...);

#ifndef ENABLE_UEFI
// src/lib/signal.c: remove all signal handlers
extern int reset_signals(void);

// src/lib/signal.c: handle a signal list (sa_handler)
extern void handle_signals(int*sigs,int len,void(*handler)(int));

#ifdef _GNU_SOURCE
// src/lib/signal.c: handle a signal list (sa_sigaction)
extern void action_signals(int*sigs,int len,void(*action)(int, siginfo_t*,void*));
#endif

// src/lib/signal.c: convert sig to string (eg. signame(2) = "SIGINT")
extern const char*signame(int sig);
#endif

// src/lib/signal.c: make sure real sleep n seconds
unsigned int xsleep(unsigned int n);

// src/lib/stdio.c: get the highest possible fd
extern int get_max_fd(void);

// src/lib/stdio.c: close all fds
extern int close_all_fd(const int*exclude,int count);

// src/lib/stdio.c: change foreground virtual terminal
extern int set_active_console(int vt);

// src/lib/stdio.c: get active kernel consoles
extern char**get_active_consoles(void);

// src/lib/stdio.c: full read string
extern ssize_t full_read(int fd,void*data,size_t len);

// src/lib/stdio.c: full write string
extern ssize_t full_write(int fd,void*data,size_t len);

// file type macros
#define is_type(fd,err,type,path...) fd_is_type(AT_FDCWD,err,type,path);
#define fd_is_file(fd,path...) fd_is_type(fd,0,S_IFREG,path)
#define fd_is_link(fd,path...) fd_is_type(fd,0,S_IFLNK,path)
#define fd_is_char(fd,path...) fd_is_type(fd,ENOTTY,S_IFCHR,path)
#define fd_is_block(fd,path...) fd_is_type(fd,ENOTBLK,S_IFBLK,path)
#define fd_is_folder(fd,path...) fd_is_type(fd,ENOTDIR,S_IFDIR,path)
#define is_file(path...) fd_is_file(AT_FDCWD,path)
#define is_link(path...) fd_is_link(AT_FDCWD,path)
#define is_char(path...) fd_is_char(AT_FDCWD,path)
#define is_block(path...) fd_is_block(AT_FDCWD,path)
#define is_folder(path...) fd_is_folder(AT_FDCWD,path)

// call libmount to mount and exit if failed
#define exmount(src,tgt,fs,data) \
	if(xmount(true,src,tgt,fs,data,false)<0)return -errno
#endif
