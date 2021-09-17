#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<strings.h>
#include"defines.h"
#include"output.h"
#include"confd.h"
#include"str.h"
#include"getopt.h"

enum ctl_oper{
	OPER_NONE,
	OPER_DUMP,
	OPER_DELETE,
	OPER_QUIT,
};

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: confctl [OPTION] [KEY [VALUE]]\n"
		"Control init config daemon.\n\n"
		"Options:\n"
		"\t-s, --socket <SOCKET>  Use custom control socket (default is %s)\n"
		"\t-D, --delete <KEY>     Delete item\n"
		"\t-q, --quit             Terminate confd\n"
		"\t-d, --dump             Dump config store\n"
		"\t-h, --help             Display this help and exit\n",
		DEFAULT_CONFD
	);
}

static int do_get(char*key){
	errno=0;
	enum conf_type t=confd_get_type(key);
	if(errno!=0)return re_err(1,"read conf key %s failed",key);
	switch(t){
		case TYPE_KEY:fprintf(stderr,_("'%s' is not a value\n"),key);return 1;
		case TYPE_STRING:printf("%s\n",confd_get_string(key,""));break;
		case TYPE_INTEGER:printf("%ld\n",confd_get_integer(key,0));break;
		case TYPE_BOOLEAN:printf("%s\n",BOOL2STR(confd_get_boolean(key,false)));break;
	}
	if(errno!=0)return re_err(1,"read conf key '%s' failed",key);
	return 0;
}

static int do_set(char*key,char*value){
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
	return 0;
	done:
	if(r!=0)fd_perror(STDERR_FILENO,_("set conf key '%s' failed"),key);
	return r;
}

static int do_get_set(char*key,char*value){
	return value?do_set(key,value):do_get(key);
}

int confctl_main(int argc,char**argv){
	static const struct option lo[]={
		{"help",    no_argument,       NULL,'h'},
		{"quit",    no_argument,       NULL,'q'},
		{"dump",    no_argument,       NULL,'D'},
		{"delete",  required_argument, NULL,'d'},
		{"socket",  required_argument, NULL,'s'},
		{NULL,0,NULL,0}
	};
	char*socket=NULL,*key=NULL;
	enum ctl_oper op=OPER_NONE;
	int o;
	while((o=b_getlopt(argc,argv,"hqDd:s:",lo,NULL))>0)switch(o){
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
		case 's':
			if(socket)goto conflict;
			socket=b_optarg;
		break;
		default:return 1;
	}
	int ac=argc-b_optind;
	char**av=argv+b_optind;
	if(
		(op==OPER_NONE&&ac<=0)||
		(op!=OPER_NONE&&ac>0)
	)return usage(1);
	if(!socket)socket=DEFAULT_CONFD;
	if(open_confd_socket("confctl",socket)<0)return 2;
	int r;
	switch(op){
		case OPER_QUIT:
			r=confd_quit();
			if(errno>0)perror(_("terminate confd failed"));
		break;
		case OPER_DUMP:
			r=confd_dump();
			if(errno>0)perror(_("dump config store failed"));
			break;
		case OPER_DELETE:
			r=confd_delete(key);
			if(errno>0)perror(_("delete config item failed"));
		break;
		case OPER_NONE:{
			if(ac>2)return re_printf(2,"too many arguments\n");
			r=do_get_set(av[0],ac==2?av[1]:NULL);
		}break;
	}
	return r;
	conflict:return re_printf(2,"too many arguments\n");
}
