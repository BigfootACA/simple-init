/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<pwd.h>
#include<errno.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<strings.h>
#include"defines.h"
#include"output.h"
#include"confd.h"
#include"str.h"
#include"getopt.h"
#include"system.h"

enum ctl_oper{
	OPER_NONE,
	OPER_DUMP,
	OPER_LIST,
	OPER_DELETE,
	OPER_QUIT,
	OPER_SAVE,
	OPER_LOAD,
	OPER_SET_DEFAULT,
	OPER_CHOWN,
	OPER_CHGRP,
	OPER_CHMOD,
	OPER_GETOWN,
	OPER_GETGRP,
	OPER_GETMOD,
	OPER_RENAME,
};

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: confctl [OPTION] [KEY [VALUE|OWNER|GROUP|MODE]]\n"
		"Control init config daemon.\n\n"
		"Options:\n"
		"\t-s, --socket <SOCKET>  Use custom control socket (default is %s)\n"
		"\t-d, --delete <KEY>     Delete item\n"
		"\t-l, --list <KEY>       List items\n"
		"\t-O, --chown            Change item owner\n"
		"\t-G, --chgrp            Change item group\n"
		"\t-M, --chmod            Change item mode\n"
		"\t-o, --getown           Get item owner\n"
		"\t-g, --getgrp           Get item group\n"
		"\t-m, --getmod           Get item mode\n"
		"\t-L, --load <PATH>      Load config from a file\n"
		"\t-S, --save <PATH>      Save config to a file\n"
		"\t-x, --path <PATH>      Set default config path\n"
		"\t-r, --rename           Rename config item\n"
		"\t-q, --quit             Terminate confd\n"
		"\t-D, --dump             Dump config store\n"
		"\t-h, --help             Display this help and exit\n",
		DEFAULT_CONFD
	);
}

int confctl_do_ls(char*key){
	errno=0;
	char**r=confd_ls(key);
	if(errno!=0||!r||!r[0])return re_err(1,"list conf key '%s' failed",key);
	for(int i=0;r[i];i++)puts(r[i]);
	free(r[0]);
	free(r);
	return errno;
}

int confctl_do_get(char*key){
	errno=0;
	enum conf_type t=confd_get_type(key);
	if(errno!=0)return re_err(1,"read conf key '%s' failed",key);
	char*x;
	switch(t){
		case TYPE_KEY:fprintf(stderr,_("'%s' is not a value\n"),key);return 1;
		case TYPE_STRING:
			if((x=confd_get_string(key,""))){
				puts(x);
				free(x);
			}
		break;
		case TYPE_INTEGER:printf("%ld\n",confd_get_integer(key,0));break;
		case TYPE_BOOLEAN:printf("%s\n",BOOL2STR(confd_get_boolean(key,false)));break;
	}
	if(errno!=0)return re_err(1,"read conf key '%s' failed",key);
	return errno;
}

int confctl_do_set(char*key,char*value){
	errno=0;
	int r;
	enum conf_type t=confd_get_type(key);
	if(errno!=0)t=0;
	if(t==0||t==TYPE_BOOLEAN){
		errno=0;
		if(strcasecmp(value,"true")==0){
			r=confd_set_boolean(key,true);
			goto done;
		}else if(strcasecmp(value,"false")==0){
			r=confd_set_boolean(key,false);
			goto done;
		}else if(t==TYPE_BOOLEAN)return re_printf(1,"invalid boolean\n");
	}
	if(t==0||t==TYPE_INTEGER){
		char*end;
		int64_t l=(int64_t)strtol(value,&end,10);
		if((!*end&&value!=end&&errno==0)){
			r=confd_set_integer(key,l);
			goto done;
		}else if(t==TYPE_INTEGER)return re_printf(1,"invalid integer\n");
	}
	if(t==0||t==TYPE_STRING){
		r=confd_set_string(key,value);
		goto done;
	}
	r=-1,errno=t==TYPE_KEY?EISDIR:0;
	done:
	if(r!=0)fd_perror(STDERR_FILENO,_("set conf key '%s' failed"),key);
	return r;
}

static int do_get_set(char*key,char*value){
	return value?confctl_do_set(key,value):confctl_do_get(key);
}

static int do_get_perm(enum ctl_oper op,char*key){
	int r=0;
	uid_t u=0;
	gid_t g=0;
	mode_t m=0;
	char user[64]={0},group[64]={0};
	switch(op){
		case OPER_GETOWN:
			r=confd_get_own(key,&u);
			if(r!=0)fd_perror(STDERR_FILENO,_("get conf key '%s' owner failed"),key);
			else printf("UID=%d\nUSER=%s\n",u,get_username(u,user,63));
		break;
		case OPER_GETGRP:
			r=confd_get_grp(key,&g);
			if(r!=0)fd_perror(STDERR_FILENO,_("get conf key '%s' group failed"),key);
			else printf("GID=%d\nGROUP=%s\n",g,get_groupname(g,group,63));
		break;
		case OPER_GETMOD:
			r=confd_get_mod(key,&m);
			if(r!=0)fd_perror(STDERR_FILENO,_("get conf key '%s' mode failed"),key);
			else printf("MODE=%04o\nPERM=%s\n",m,mode_string(m)+1);
		break;
		default:r=2;
	}
	return r;
}

static int do_set_perm(enum ctl_oper op,char*key,char*val){
	int r;
	char*end;
	long l;
	errno=0;
	l=strtol(val,&end,op==OPER_CHMOD?8:10);
	if((*end||val==end||errno!=0))return re_printf(2,"invalid argument: %s\n",val);
	switch(op){
		case OPER_CHOWN:
			r=confd_set_own(key,(uid_t)l);
			if(r!=0)fd_perror(STDERR_FILENO,_("set conf key '%s' owner failed"),key);
		break;
		case OPER_CHGRP:
			r=confd_set_grp(key,(gid_t)l);
			if(r!=0)fd_perror(STDERR_FILENO,_("set conf key '%s' group failed"),key);
		break;
		case OPER_CHMOD:
			r=confd_set_mod(key,(mode_t)l);
			if(r!=0)fd_perror(STDERR_FILENO,_("set conf key '%s' mode failed"),key);
		break;
		default:r=2;
	}
	return r;
}

int confctl_main(int argc,char**argv){
	static const struct option lo[]={
		{"help",    no_argument,       NULL,'h'},
		{"quit",    no_argument,       NULL,'q'},
		{"dump",    no_argument,       NULL,'D'},
		{"save",    required_argument, NULL,'S'},
		{"load",    required_argument, NULL,'L'},
		{"path",    required_argument, NULL,'x'},
		{"list",    required_argument, NULL,'l'},
		{"delete",  required_argument, NULL,'d'},
		{"socket",  required_argument, NULL,'s'},
		{"chown",   no_argument,       NULL,'O'},
		{"chgrp",   no_argument,       NULL,'G'},
		{"chmod",   no_argument,       NULL,'M'},
		{"getown",  no_argument,       NULL,'o'},
		{"getgrp",  no_argument,       NULL,'g'},
		{"getmod",  no_argument,       NULL,'m'},
		{"rename",  no_argument,       NULL,'r'},
		{NULL,0,NULL,0}
	};
	char*socket=NULL,*key=NULL;
	enum ctl_oper op=OPER_NONE;
	int o;
	while((o=b_getlopt(argc,argv,"hqDS:L:p:d:l:s:OGMogmr",lo,NULL))>0)switch(o){
		case 'h':return usage(0);
		case 'q':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_QUIT;
		break;
		case 'D':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_DUMP;
		break;
		case 'd':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_DELETE,key=b_optarg;
		break;
		case 'l':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_LIST,key=b_optarg;
		break;
		case 'S':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_SAVE,key=b_optarg;
		break;
		case 'L':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_LOAD,key=b_optarg;
		break;
		case 'x':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_SET_DEFAULT,key=b_optarg;
		break;
		case 'O':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_CHOWN;
		break;
		case 'G':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_CHGRP;
		break;
		case 'M':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_CHMOD;
		break;
		case 'o':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_GETOWN;
		break;
		case 'g':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_GETGRP;
		break;
		case 'm':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_GETMOD;
		break;
		case 'r':
			if(op!=OPER_NONE)goto conflict;
			op=OPER_RENAME;
		break;
		case 's':
			if(socket)goto conflict;
			socket=b_optarg;
		break;
		default:return 1;
	}
	int ac=argc-b_optind;
	char**av=argv+b_optind;
	if(!socket)socket=DEFAULT_CONFD;
	if(open_confd_socket(false,"confctl",socket)<0)return 2;
	int r;
	switch(op){
		case OPER_QUIT:
			r=confd_quit();
			if(errno>0)perror(_("terminate confd failed"));
		break;
		case OPER_DUMP:
			r=confd_dump(LEVEL_DEBUG);
			if(errno>0)perror(_("dump config store failed"));
		break;
		case OPER_LIST:
			r=confctl_do_ls(key);
			if(errno>0)perror(_("list config items failed"));
		break;
		case OPER_DELETE:
			r=confd_delete(key);
			if(errno>0)perror(_("delete config item failed"));
		break;
		case OPER_SAVE:
			r=confd_save_file(key);
			if(errno>0)perror(_("save config failed"));
		break;
		case OPER_LOAD:
			r=confd_load_file(key);
			if(errno>0)perror(_("load config failed"));
		break;
		case OPER_SET_DEFAULT:
			r=confd_set_default_config(key);
			if(errno>0)perror(_("set default config failed"));
		break;
		case OPER_CHOWN:
		case OPER_CHGRP:
		case OPER_CHMOD:
			if(ac<2)return re_printf(2,"missing arguments\n");
			if(ac>2)return re_printf(2,"too many arguments\n");
			r=do_set_perm(op,av[0],av[1]);
		break;
		case OPER_GETOWN:
		case OPER_GETGRP:
		case OPER_GETMOD:
			if(ac<1)return re_printf(2,"missing arguments\n");
			if(ac>1)return re_printf(2,"too many arguments\n");
			r=do_get_perm(op,av[0]);
		break;
		case OPER_RENAME:
			if(ac<2)return re_printf(2,"missing arguments\n");
			if(ac>2)return re_printf(2,"too many arguments\n");
			r=confd_rename(av[0],av[1]);
			if(errno>0)perror(_("rename config item failed"));
		break;
		case OPER_NONE:{
			if(ac<=0)return usage(1);
			if(ac>2)return re_printf(2,"too many arguments\n");
			r=do_get_set(av[0],ac==2?av[1]:NULL);
		}break;
	}
	return r;
	conflict:return re_printf(2,"too many arguments\n");
}
