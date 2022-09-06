/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _PATHNAMES_H
#define _PATHNAMES_H
#ifdef _PATHS_H
#error "Do not use paths.h"
#endif

#undef	_PATH_DEFPATH
#undef	_PATH_STDPATH
#undef	_PATH_BSHELL
#undef	_PATH_CONSOLE
#undef	_PATH_CSHELL
#undef	_PATH_DEVDB
#undef	_PATH_DEVNULL
#undef	_PATH_DRUM
#undef	_PATH_GSHADOW
#undef	_PATH_KLOG
#undef	_PATH_KMEM
#undef	_PATH_LASTLOG
#undef	_PATH_MAILDIR
#undef	_PATH_MAN
#undef	_PATH_MEM
#undef	_PATH_MNTTAB
#undef	_PATH_MOUNTED
#undef	_PATH_NOLOGIN
#undef	_PATH_PRESERVE
#undef	_PATH_RWHODIR
#undef	_PATH_SENDMAIL
#undef	_PATH_SHADOW
#undef	_PATH_SHELLS
#undef	_PATH_TTY
#undef	_PATH_UNIX
#undef	_PATH_UTMP
#undef	_PATH_VI
#undef	_PATH_WTMP
#undef	_PATH_DEV
#undef	_PATH_TMP
#undef	_PATH_VARDB
#undef	_PATH_VARRUN
#undef	_PATH_VARTMP

#define _PATH_ROOT		"/"
#define _PATH_TMP		_PATH_ROOT"tmp"
#define _PATH_RUN		_PATH_ROOT"run"
#define _PATH_DEV		_PATH_ROOT"dev"
#define _PATH_DEVS		_PATH_DEV"/"
#define _PATH_SYS		_PATH_ROOT"sys"
#define _PATH_PROC		_PATH_ROOT"proc"
#define _PATH_USR		_PATH_ROOT"usr"
#define _PATH_ETC		_PATH_ROOT"etc"
#define _PATH_VAR		_PATH_ROOT"var"
#define _PATH_BIN		_PATH_ROOT"bin"
#define _PATH_SBIN		_PATH_ROOT"sbin"
#define _PATH_LIB		_PATH_ROOT"lib"
#define _PATH_LIB32		_PATH_ROOT"lib32"
#define _PATH_LIB64		_PATH_ROOT"lib64"
#define _PATH_LIBEXEC		_PATH_ROOT"libexec"
#define _PATH_USR_BIN		_PATH_USR"/bin"
#define _PATH_USR_SBIN		_PATH_USR"/sbin"
#define _PATH_USR_LIB		_PATH_USR"/lib"
#define _PATH_USR_LIB32		_PATH_USR"/lib32"
#define _PATH_USR_LIB64		_PATH_USR"/lib64"
#define _PATH_USR_LIBEXEC	_PATH_USR"/libexec"
#define _PATH_USR_LOCAL		_PATH_USR"/local"
#define _PATH_USR_LOCAL_BIN	_PATH_USR_LOCAL"/bin"
#define _PATH_USR_LOCAL_SBIN	_PATH_USR_LOCAL"/sbin"
#define _PATH_USR_LOCAL_LIB	_PATH_USR_LOCAL"/lib"
#define _PATH_USR_LOCAL_LIB32	_PATH_USR_LOCAL"/lib32"
#define _PATH_USR_LOCAL_LIB64	_PATH_USR_LOCAL"/lib64"
#define _PATH_USR_LOCAL_LIBEXEC	_PATH_USR_LOCAL"/libexec"
#define _PATH_VAR_LIB		_PATH_VAR"/lib"
#define _PATH_VAR_LOG		_PATH_VAR"/log"
#define _PATH_FSTAB		_PATH_ETC"/fstab"
#define _PATH_DEV_HUGEPAGES	_PATH_DEV"/hugepages"
#define _PATH_DEV_MQUEUE	_PATH_DEV"/mqueue"
#define _PATH_DEV_PTS		_PATH_DEV"/pts"
#define _PATH_DEV_SHM		_PATH_DEV"/shm"
#define _PATH_DEV_MAPPER	_PATH_DEV"/mapper"
#define _PATH_DEV_LOOP		_PATH_DEV"/loop"
#define _PATH_DEV_LOOPCTL	_PATH_DEV"/loop-control"
#define _PATH_DEV_ZERO		_PATH_DEV"/zero"
#define _PATH_DEV_FULL		_PATH_DEV"/full"
#define _PATH_DEV_NULL		_PATH_DEV"/null"
#define _PATH_DEV_MEM		_PATH_DEV"/mem"
#define _PATH_DEV_KMEM		_PATH_DEV"/kmem"
#define _PATH_DEV_KMSG		_PATH_DEV"/kmsg"
#define _PATH_DEV_RANDOM	_PATH_DEV"/random"
#define _PATH_DEV_URANDOM	_PATH_DEV"/urandom"
#define _PATH_DEV_CONSOLE	_PATH_DEV"/console"
#define _PATH_DEV_DISK		_PATH_DEV"/disk"
#define _PATH_DEV_BYLABEL	_PATH_DEV_DISK"/by-label"
#define _PATH_DEV_BYUUID	_PATH_DEV_DISK"/by-uuid"
#define _PATH_DEV_BYID		_PATH_DEV_DISK"/by-id"
#define _PATH_DEV_BYPATH	_PATH_DEV_DISK"/by-path"
#define _PATH_DEV_BYPARTLABEL	_PATH_DEV_DISK"/by-partlabel"
#define _PATH_DEV_BYPARTUUID	_PATH_DEV_DISK"/by-partuuid"
#define _PATH_SYS_FS		_PATH_SYS"/fs"
#define _PATH_SYS_DEV		_PATH_SYS"/dev"
#define _PATH_SYS_DEVICES	_PATH_SYS"/devices"
#define _PATH_SYS_BUS		_PATH_SYS"/bus"
#define _PATH_SYS_BLOCK		_PATH_SYS"/block"
#define _PATH_SYS_CLASS		_PATH_SYS"/class"
#define _PATH_SYS_SCSI		_PATH_SYS"/scsi"
#define _PATH_SYS_MODULE	_PATH_SYS"/module"
#define _PATH_SYS_KERNEL	_PATH_SYS"/kernel"
#define _PATH_SYS_DEVBLOCK	_PATH_SYS_DEV"/block"
#define _PATH_SYS_DEVCHAR	_PATH_SYS_DEV"/char"
#define _PATH_SYS_BUS_SCSI	_PATH_SYS_BUS"/scsi"
#define _PATH_SECURITYFS	_PATH_SYS_KERNEL"/security"
#define _PATH_DEBUGFS		_PATH_SYS_KERNEL"/debug"
#define _PATH_TRACEFS		_PATH_SYS_KERNEL"/tracing"
#define _PATH_CONFIGFS		_PATH_SYS_KERNEL"/config"
#define _PATH_PSTORE		_PATH_SYS_FS"/pstore"
#define _PATH_BPF		_PATH_SYS_FS"/bpf"
#define _PATH_FUSECTL		_PATH_SYS_FS"/fuse/connections"
#define _PATH_SYS_FIRMWARE	_PATH_SYS"/firmware"
#define _PATH_PROC_PARTITIONS	_PATH_PROC"/partitions"
#define _PATH_PROC_CMDLINE	_PATH_PROC"/cmdline"
#define _PATH_PROC_FILESYSTEMS	_PATH_PROC"/filesystems"
#define _PATH_PROC_DEVICES	_PATH_PROC"/devices"
#define _PATH_PROC_MOUNTS	_PATH_PROC"/mounts"
#define _PATH_PROC_MODULES	_PATH_PROC"/modules"
#define _PATH_PROC_SWAPS	_PATH_PROC"/swaps"
#define _PATH_PROC_SELF		_PATH_PROC"/self"
#define _PATH_PROC_LVM		_PATH_PROC"/lvm"
#define _PATH_PROC_SYS		_PATH_PROC"/sys"
#define _PATH_PROC_SYS_FS	_PATH_PROC_SYS"/fs"
#define _PATH_PROC_MOUNTINFO	_PATH_PROC_SELF"/mountinfo"
#define _PATH_PROC_FDDIR	_PATH_PROC_SELF"/fd"
#define _PATH_BINFMT_MISC	_PATH_PROC_SYS_FS"/binfmt_misc"
#define	_PATH_MNTTAB		_PATH_ETC"/fstab"
#define	_PATH_MOUNTED		_PATH_PROC_SELF"/mounts"
#define _PATH_RUNSTATEDIR	_PATH_RUN
#define _PATH_SYSCONFSTATICDIR	_PATH_ETC
#define _PATH_OS_RELEASE_ETC	_PATH_ETC"/os-release"
#define _PATH_OS_RELEASE_USR	_PATH_USR_LIB"/os-release"
#define _PATH_ISSUE_FILENAME	"issue"
#define _PATH_ISSUE_DIRNAME	_PATH_ISSUE_FILENAME ".d"
#define _PATH_ISSUE		_PATH_ETC"/"_PATH_ISSUE_FILENAME
#define _PATH_ISSUEDIR		_PATH_ETC"/"_PATH_ISSUE_DIRNAME
#define _PATH_NUMLOCK_ON	_PATH_RUN"/numlock-on"
#define _PATH_FILESYSTEMS	_PATH_ETC"/filesystems"
#define _PATH_UTMP		_PATH_RUN"/utmp"
#define _PATH_WTMP		_PATH_VAR_LOG"/wtmp"
#define _PATH_LOGIN		_PATH_SBIN"/login"
#define _PATH_BIN_PATH		_PATH_USR_LOCAL_BIN":"_PATH_USR_BIN":"_PATH_BIN
#define _PATH_SBIN_PATH		_PATH_USR_LOCAL_SBIN":"_PATH_USR_SBIN":"_PATH_SBIN
#define _PATH_USER_PATH		_PATH_BIN_PATH
#define _PATH_ROOT_PATH		_PATH_SBIN_PATH":"_PATH_BIN_PATH
#define _PATH_DEFAULT_PATH	_PATH_BIN_PATH":"_PATH_SBIN_PATH
#define _PATH_LIB_MODULES	_PATH_LIB"/modules"
#define PATH_DELETED_SUFFIX	" (deleted)"

#endif