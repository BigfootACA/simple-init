/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<unistd.h>
#include<strings.h>
#include<sys/stat.h>
#include"str.h"
#include"aboot.h"
#include"output.h"
#include"getopt.h"

enum oper{
	OPER_NONE,
	OPER_CREATE,
	OPER_EXTRACT,
	OPER_UPDATE,
};
static char*name=NULL,*cmdline=NULL,*page_size=NULL;
static char*kernel=NULL,*ramdisk=NULL,*second=NULL,*image=NULL;
static int page=4096;
static bool force=false,overwrite=false;

static int usage(int e){
	return r_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: abootimg [OPTIONS]...\n"
		"Android boot image tools.\n\n"
		"Options: \n"
		"\t-c, --create <IMG>      create new image\n"
		"\t-x, --extract <IMG>     extract image\n"
		"\t-u, --update <IMG>      update image\n"
		"\t-k, --kernel <KERNEL>   kernel path\n"
		"\t-r, --ramdisk <RAMDISK> ramdisk path (initramfs)\n"
		"\t-s, --second <SECOND>   second stage bootloader path\n"
		"\t-b, --cmdline <CMDLINE> set boot cmdline (bootargs max 511 chars)\n"
		"\t-n, --name <NAME>       set image name (max 15 chars)\n"
		"\t-p, --page <PAGE>       set page size (default 4096)\n"
		"\t-f, --force             force use stdin or stdout when tty\n"
		"\t-o, --overwrite         do not backup anything\n"
		"\t-h, --help              show this help\n"
	);
}

static void check_use_stdin(){
	static bool stdin_used=false;
	if(stdin_used)ee_printf(1,"reuse of stdin is not allowed\n");
	else stdin_used=true;
}

static void check_use_stdout(){
	static bool stdout_used=false;
	if(stdout_used)ee_printf(1,"reuse of stdout is not allowed\n");
	else stdout_used=true;
}

static void check_tty(){
	if(force)return;
	if(!isatty(STDOUT_FILENO))return;
	ee_printf(1,_("stdout is a tty, use -f to force output\n"));
	check_use_stdout();
}

static void check_backup(char*path){
	struct stat st;
	if(overwrite)return;
	if(stat(path,&st)!=0)return;
	if(!S_ISREG(st.st_mode))return;
	char new_name[PATH_MAX]={0};
	snprintf(new_name,sizeof(new_name)-1,"%s.bak",path);
	if(rename(path,new_name)==0)fprintf(
		stderr,_("backup '%s' to '%s'\n"),
		path,new_name
	);
}

static aboot_image*load_image(){
	bool use_pipe=strcmp(image,"-")==0;
	if(use_pipe){
		check_use_stdin();
		fprintf(stderr,_("read image from stdin...\n"));
	}else fprintf(stderr,_("load image from '%s'...\n"),image);
	errno=0;
	aboot_image*img=use_pipe?
		abootimg_load_from_fd(STDIN_FILENO):
		abootimg_load_from_file(AT_FDCWD,image);
	if(!img)exit_perror(1,"load image failed");
	return img;
}

static void save_image(aboot_image*img){
	bool use_pipe=strcmp(image,"-")==0;
	if(use_pipe){
		check_tty();
		fprintf(stderr,_("write image to stdout...\n"));
	}else{
		check_backup(image);
		fprintf(stderr,_("save image to '%s'...\n"),image);
	}
	errno=0;
	if(!(use_pipe?
		abootimg_save_to_fd(img,STDOUT_FILENO):
		abootimg_save_to_file(img,AT_FDCWD,image)
	)){
		stderr_perror(_("save image failed"));
		abootimg_free(img);
		exit(1);
	}
}

static void check_image(aboot_image*img){
	if(abootimg_is_empty(img))fprintf(stderr,_("warning: looks like an empty image\n"));
	else if(abootimg_is_invalid(img))fprintf(stderr,_("warning: looks like an invalid image\n"));
}

#define DO_SAVE(type) if(abootimg_have_##type(img)){ \
	bool use_pipe=strcmp(type,"-")==0;\
	if(use_pipe){\
		check_tty();\
        	fprintf(stderr,_("write "#type" to stdout...\n"));\
        }else{\
                check_backup(type);\
                fprintf(stderr,_("save "#type" to '%s'...\n"),type);\
        }\
	errno=0;\
	if(!(use_pipe?\
		abootimg_save_##type##_to_fd(img,STDOUT_FILENO):\
		abootimg_save_##type##_to_file(img,AT_FDCWD,type)\
	)){\
		stderr_perror(_("save "#type" failed"));\
		abootimg_free(img);\
		exit(1);\
	}\
}

#define DO_LOAD(type) if(type){\
	bool use_pipe=strcmp(type,"-")==0;\
	if(use_pipe){\
                check_use_stdin();\
                fprintf(stderr,_("read "#type" from stdin...\n"));\
        }else fprintf(stderr,_("load "#type" from '%s'...\n"),type);\
	errno=0;\
	if(!(use_pipe?\
		abootimg_load_##type##_from_fd(img,STDIN_FILENO):\
		abootimg_load_##type##_from_file(img,AT_FDCWD,type)\
	)){\
		stderr_perror(_("load "#type" failed"));\
		abootimg_free(img);\
		exit(1);\
	}\
}

static void do_load(aboot_image*img){
	DO_LOAD(kernel)
	DO_LOAD(ramdisk)
	DO_LOAD(second)
	if(page_size)abootimg_set_page_size(img,page);
	if(cmdline)abootimg_set_cmdline(img,cmdline);
	if(name)abootimg_set_name(img,name);
	save_image(img);
}

static void do_save(aboot_image*img){
	if(!kernel)kernel="zImage";
	if(!ramdisk)ramdisk="initrd.img";
	if(!second)second="stage2.img";
	DO_SAVE(kernel)
	DO_SAVE(ramdisk)
	DO_SAVE(second)
}

static void do_create(){
	fprintf(stderr,_("create new image...\n"));
	errno=0;
	aboot_image*img=abootimg_new_image();
	if(!img)exit_perror(1,"create image failed");
	if(!kernel)ee_printf(1,"no kernel specified\n");
	do_load(img);
	abootimg_free(img);
}

static void do_extract(){
	aboot_image*img=load_image();
	check_image(img);
	do_save(img);
	abootimg_free(img);
}

static void do_update(){
	aboot_image*img=load_image();
	check_image(img);
	do_load(img);
	abootimg_free(img);
}

static inline void check_var(char**var){
	if(var&&*var&&!**var)*var=NULL;
}

int abootimg_main(int argc,char**argv){
	int r=0,o;
	enum oper oper=OPER_NONE;
	static const struct option lo[]={
		{"create",    required_argument, NULL,'c'},
		{"extract",   required_argument, NULL,'x'},
		{"update",    required_argument, NULL,'u'},
		{"kernel",    required_argument, NULL,'k'},
		{"ramdisk",   required_argument, NULL,'r'},
		{"second",    required_argument, NULL,'s'},
		{"cmdline",   required_argument, NULL,'b'},
		{"name",      required_argument, NULL,'n'},
		{"page",      required_argument, NULL,'p'},
		{"force",     no_argument,       NULL,'f'},
		{"overwrite", no_argument,       NULL,'o'},
		{"help",      no_argument,       NULL,'h'},
		{NULL,0,NULL,0}
	};
	while((o=b_getlopt(argc,argv,"c:x:u:k:r:s:b:n:p:foh",lo,NULL))>0)switch(o){
		case 'c':if(oper!=OPER_NONE)goto conflict;oper=OPER_CREATE;image=b_optarg;break;
		case 'x':if(oper!=OPER_NONE)goto conflict;oper=OPER_EXTRACT;image=b_optarg;break;
		case 'u':if(oper!=OPER_NONE)goto conflict;oper=OPER_UPDATE;image=b_optarg;break;
		case 'k':if(kernel)goto conflict;kernel=b_optarg;break;
		case 'r':if(ramdisk)goto conflict;ramdisk=b_optarg;break;
		case 's':if(second)goto conflict;second=b_optarg;break;
		case 'b':if(cmdline)goto conflict;cmdline=b_optarg;break;
		case 'n':if(name)goto conflict;name=b_optarg;break;
		case 'p':if(page_size)goto conflict;page_size=b_optarg;break;
		case 'f':force=true;break;
		case 'o':overwrite=true;break;
		case 'h':return usage(0);
		default:return 1;
	}
	if(b_optind!=argc)goto conflict;
	check_var(&kernel);
	check_var(&ramdisk);
	check_var(&second);
	check_var(&cmdline);
	check_var(&page_size);
	if(!image)return usage(1);
	if(page_size&&(page=parse_int(page_size,0))<=0)goto invalid;
	if(!abootimg_check_page(page))goto invalid;
	switch(oper){
		case OPER_NONE:return usage(1);
		case OPER_CREATE:do_create();break;
		case OPER_EXTRACT:do_extract();break;
		case OPER_UPDATE:do_update();break;
	}
	return r;
	conflict:return re_printf(2,"too many arguments\n");
	invalid:return re_printf(2,"invalid argument\n");
}
