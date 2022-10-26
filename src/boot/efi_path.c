/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<stdbool.h>
#include<stdio.h>
#include"boot.h"

#define ARR(...)((char*[]){__VA_ARGS__,NULL})
#define EFI ".efi"
static char*ms_dir[]=ARR("/EFI/Microsoft/Boot/");
static char*mac_dir[]=ARR("/System/Library/CoreServices/");
static char*ms_boot[]=ARR("bootmgfw"EFI);
static char*mac_boot[]=ARR("boot"EFI);
static char*mac_diag[]=ARR(".diagnostics/diags"EFI);
#define BOOT_DIR(...) ARR("/","/EFI/","/EFI/BOOT/",__VA_ARGS__)
#define CUST_DIR(_cust) BOOT_DIR(("/EFI/"_cust"/"))
#define ARCH_FILE(_name,_in,_arch) (_name _in _arch EFI)
#define ARCH1_FILE(_name,_arch) ARCH_FILE(_name,"",_arch)
#define ARCH2_FILE(_name,_arch) ARCH_FILE(_name,"-",_arch)
#define ARCH3_FILE(_name,_arch) ARCH_FILE(_name,"_",_arch)
#define ARCH_NAME(_name,_func)\
	ARCH1_FILE(_name,""),\
	_func(_name,"ia32"),\
	_func(_name,"ia64"),\
	_func(_name,"x64"),\
	_func(_name,"arm"),\
	_func(_name,"aa64"),\
	_func(_name,"riscv32"),\
	_func(_name,"riscv64")
#define ARCH1_NAME(_name) ARCH_NAME(_name,ARCH1_FILE)
#define ARCH2_NAME(_name) ARCH_NAME(_name,ARCH2_FILE)
#define ARCH3_NAME(_name) ARCH_NAME(_name,ARCH3_FILE)
#define OPT(_cpu,_icon,_title,_dir,_file,...){.enable=true,.cpu=_cpu,.icon=_icon,.title=_title,.dir=_dir,.name=_file,__VA_ARGS__}
#define OPT_ANY(_icon,_title,_dir,_file,...)                OPT(CPU_ANY,_icon,_title,_dir,_file,__VA_ARGS__)
#define OPT_IA32(_icon,_title,_dir,_file,...)               OPT(CPU_IA32,_icon,_title,_dir,_file,__VA_ARGS__)
#define OPT_X64(_icon,_title,_dir,_file,...)                OPT(CPU_X64,_icon,_title,_dir,_file,__VA_ARGS__)
#define OPT_ARM(_icon,_title,_dir,_file,...)                OPT(CPU_ARM,_icon,_title,_dir,_file,__VA_ARGS__)
#define OPT_AARCH64(_icon,_title,_dir,_file,...)            OPT(CPU_AARCH64,_icon,_title,_dir,_file,__VA_ARGS__)
#define OPT_CUST(_cpu,_icon,_title,_cust,...)               OPT(_cpu,_icon,_title,CUST_DIR(_cust),ARR(__VA_ARGS__))
#define OPT_CUST_ARCH1(_cpu,_icon,_title,_cust,_name,_arch) OPT_CUST(_cpu,_icon,_title,_cust,ARCH1_FILE(_name,_arch))
#define OPT_CUST_ARCH2(_cpu,_icon,_title,_cust,_name,_arch) OPT_CUST(_cpu,_icon,_title,_cust,ARCH2_FILE(_name,_arch))
#define OPT_CUST_ARCH3(_cpu,_icon,_title,_cust,_name,_arch) OPT_CUST(_cpu,_icon,_title,_cust,ARCH3_FILE(_name,_arch))
#define _OPT_CUST_ALL(_icon,_title,_cust,_name,_func)\
	OPT_CUST(CPU_ANY,_icon,_title,_cust,ARCH1_FILE(_name,"")),\
	OPT_CUST(CPU_IA32,_icon,_title,_cust,_func(_name,"ia32")),\
	OPT_CUST(CPU_IA64,_icon,_title,_cust,_func(_name,"ia64")),\
	OPT_CUST(CPU_X64,_icon,_title,_cust,_func(_name,"x64")),\
	OPT_CUST(CPU_ARM,_icon,_title,_cust,_func(_name,"arm")),\
	OPT_CUST(CPU_AARCH64,_icon,_title,_cust,_func(_name,"aa64"))
#define OPT_CUST_ALL1(_icon,_title,_cust,_name) _OPT_CUST_ALL(_icon,_title,_cust,_name,ARCH1_FILE)
#define OPT_CUST_ALL2(_icon,_title,_cust,_name) _OPT_CUST_ALL(_icon,_title,_cust,_name,ARCH2_FILE)
#define OPT_CUST_ALL3(_icon,_title,_cust,_name) _OPT_CUST_ALL(_icon,_title,_cust,_name,ARCH3_FILE)
#define OPT_CUST_ALL(_icon,_title,_cust,_name)  OPT_CUST_ALL1(_icon,_title,_cust,_name),OPT_CUST_ALL2(_icon,_title,_cust,_name),OPT_CUST_ALL3(_icon,_title,_cust,_name)
#define OPT_DISTRO(_icon,_title,_name)\
	OPT(CPU_ANY,         _icon,_title,ARR("/EFI/"_name"/"), ARR(ARCH1_FILE("grub",""),        ARCH1_FILE("elilo",""))),\
	OPT(CPU_IA32,        _icon,_title,ARR("/EFI/"_name"/"), ARR(ARCH1_FILE("grub","ia32"),    ARCH1_FILE("elilo","ia32"))),\
	OPT(CPU_IA64,        _icon,_title,ARR("/EFI/"_name"/"), ARR(ARCH1_FILE("grub","ia64"),    ARCH1_FILE("elilo","ia64"))),\
	OPT(CPU_X64,         _icon,_title,ARR("/EFI/"_name"/"), ARR(ARCH1_FILE("grub","x64"),     ARCH1_FILE("elilo","x64"))),\
	OPT(CPU_ARM,         _icon,_title,ARR("/EFI/"_name"/"), ARR(ARCH1_FILE("grub","arm"),     ARCH1_FILE("elilo","arm"))),\
	OPT(CPU_AARCH64,     _icon,_title,ARR("/EFI/"_name"/"), ARR(ARCH1_FILE("grub","aa64"),    ARCH1_FILE("elilo","aa64"))),\
	OPT(CPU_RISCV32,     _icon,_title,ARR("/EFI/"_name"/"), ARR(ARCH1_FILE("grub","riscv32"), ARCH1_FILE("elilo","riscv32"))),\
	OPT(CPU_RISCV64,     _icon,_title,ARR("/EFI/"_name"/"), ARR(ARCH1_FILE("grub","riscv64"), ARCH1_FILE("elilo","riscv64")))
#define LOGO(n) "@distributor-logo-"n
#define LOAD_OPT(_str,_unicode).load_opt=_str,.unicode=_unicode
#define UTF8_OPT(_str) LOAD_OPT(_str,false)
#define UTF16_OPT(_str) LOAD_OPT(_str,true)
struct efi_path boot_efi_paths[]={
	OPT_ANY(LOGO("windows"), "Windows Boot Manager",  ms_dir,  ms_boot),

	OPT_ANY(LOGO("mac"), "Mac OS",                    mac_dir, mac_boot),
	OPT_X64(LOGO("mac"), "Mac OS with i386 kernel",   mac_dir, mac_boot, UTF16_OPT("arch=i386")),
	OPT_X64(LOGO("mac"), "Mac OS with x86_64 kernel", mac_dir, mac_boot, UTF16_OPT("arch=x86_64")),
	OPT_ANY(LOGO("mac"), "Mac OS (verbose mode)",     mac_dir, mac_boot, UTF16_OPT("-v")),
	OPT_ANY(LOGO("mac"), "Mac OS (single user mode)", mac_dir, mac_boot, UTF16_OPT("-v -s")),
	OPT_ANY(LOGO("mac"), "Mac OS (safe mode)",        mac_dir, mac_boot, UTF16_OPT("-x -s")),
	OPT_X64(LOGO("mac"), "Apple Hardware Test ",      mac_dir, mac_diag),

	OPT_CUST(CPU_IA32,"systemd-boot.svg", "Systemd Boot",          "systemd",  ARCH1_FILE("systemd","ia32")),
	OPT_CUST(CPU_X64, "systemd-boot.svg", "Systemd Boot",          "systemd",  ARCH1_FILE("systemd","x64")),
	OPT_CUST(CPU_IA32,"syslinux.svg",     "Syslinux",              "syslinux", ARCH1_FILE("syslinux","ia32")),
	OPT_CUST(CPU_X64, "syslinux.svg",     "Syslinux",              "syslinux", ARCH1_FILE("syslinux","x64")),
	OPT_CUST(CPU_X64, "fwupd.svg",        "Linux Firmware Update", "fwupd",    ARCH1_FILE("fwupd","x64")),
	OPT_CUST(CPU_ANY, "pxe.svg",          "IPXE",                  "tools",    "ipxe"EFI),
	OPT_CUST(CPU_ANY, "terminal.svg",     "UEFI Shell",            "tools",    "shell"EFI),
	OPT_CUST(CPU_ANY, "memtest.svg",      "MemTest",               "tools",    "memtest"EFI),
	OPT_CUST(CPU_ANY, "memtest.svg",      "MemTest86",             "tools",    "memtest86"EFI),
	OPT_CUST(CPU_ANY, "limine.svg",       "Limine",                "limine",   "limine"EFI),
	OPT_CUST_ALL( "linux.svg",            "Linux",                 "Linux",    "linux"),
	OPT_CUST_ALL3("gptsync.svg",          "GPT Sync",              "tools",    "gptsync"),
	OPT_CUST_ALL3("gdisk.svg",            "GDisk",                 "tools",    "gdisk"),
	OPT_CUST_ALL3("refind.svg",           "rEFInd",                "refind",   "refind"),
	OPT_CUST_ALL1("grub.svg",             "GRUB",                  "grub",     "grub"),
	OPT_CUST_ALL1("elilo.svg",            "ELILO",                 "elilo",    "elilo"),
	OPT_CUST_ALL1("shim.svg",             "Shim",                  "shim",     "shim"),

	OPT_DISTRO(LOGO("archlinux"),  "Arch Linux", "arch"),
	OPT_DISTRO(LOGO("manjaro"),    "Manjaro",    "manjaro"),
	OPT_DISTRO(LOGO("gentoo"),     "Gentoo",     "gentoo"),
	OPT_DISTRO(LOGO("debian"),     "Debian",     "debian"),
	OPT_DISTRO(LOGO("ubuntu"),     "Ubuntu",     "ubuntu"),
	OPT_DISTRO(LOGO("deepin"),     "Deepin",     "deepin"),
	OPT_DISTRO(LOGO("deepin"),     "UOS",        "uos"),
	OPT_DISTRO(LOGO("kali-linux"), "Kali Linux", "kali"),
	OPT_DISTRO(LOGO("centos"),     "CentOS",     "centos"),
	OPT_DISTRO(LOGO("fedora"),     "Fedora",     "fedora"),
	OPT_DISTRO("proxmox.png",      "Proxmox VE", "proxmox"),

	OPT_CUST_ALL1("uefi.svg","UEFI OS","","boot"),

	{false,CPU_NONE,NULL,NULL,NULL,NULL,NULL,false}
};
