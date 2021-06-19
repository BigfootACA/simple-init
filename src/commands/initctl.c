#include<errno.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include"output.h"
#include"logger.h"
#include"getopt.h"
#include"str.h"
#include"init_internal.h"

static int usage(int e){
	return return_printf(
		e,e==0?STDOUT_FILENO:STDERR_FILENO,
		"Usage: loggerctl [OPTION]...\n"
		"Control init logger daemon.\n\n"
		"Commands:\n"
		"\tpoweroff                  Power-off system\n"
		"\thalt                      Halt system\n"
		"\treboot [STRING]           Reboot system\n"
		"\tswitchroot <ROOT> [INIT]  Switch to new root\n"
		"\tsetenv <KEY> <VALUE>      Add environment variable\n"
		"\naddenv <KEY>=<VALUE>      Add environment variable\n"
		"Options:\n"
		"\t-h, --help  display this help and exit\n"
	);
}

static int cmd_wrapper(struct init_msg*msg,char*name){
	struct init_msg response;
	init_send(msg,&response);
	if(errno!=0)perror("send command");
	if(response.data.ret!=0)fprintf(
		stderr,
		"execute %s: %s\n",
		name,
		strerror(response.data.ret)
	);
	return response.data.ret;
}

static int cmd_poweroff(int argc,char**argv){
	if(argc>1)return re_printf(2,"too many arguments\n");
	struct init_msg msg;
	init_initialize_msg(&msg,ACTION_POWEROFF);
	return cmd_wrapper(&msg,argv[0]);
}

static int cmd_halt(int argc,char**argv){
	if(argc>1)return re_printf(2,"too many arguments\n");
	struct init_msg msg;
	init_initialize_msg(&msg,ACTION_HALT);
	return cmd_wrapper(&msg,argv[0]);
}

static int cmd_reboot(int argc,char**argv){
	if(argc>2)return re_printf(2,"too many arguments\n");
	struct init_msg msg;
	size_t ss=sizeof(msg.data.data);
	init_initialize_msg(&msg,ACTION_REBOOT);
	if(argc==2){
		if(strlen(argv[1])>=ss)return re_printf(2,"argument too long\n");
		strncpy(msg.data.data,argv[1],ss-1);
	}
	return cmd_wrapper(&msg,argv[0]);
}

static int cmd_switchroot(int argc,char**argv){
	if(argc>3)return re_printf(2,"too many arguments\n");
	if(argc<2)return re_printf(2,"missing arguments\n");
	#define root msg.data.newroot.root
	#define init msg.data.newroot.init
	struct init_msg msg;
	size_t s1=sizeof(root),s2=sizeof(init);
	if(
		strlen(argv[1])>=s1||
		(argc==3&&strlen(argv[2])>=s2)
	)return re_printf(2,"argument too long\n");
	init_initialize_msg(&msg,ACTION_SWITCHROOT);
	strncpy(root,argv[1],s1-1);
	if(argc==3)strncpy(init,argv[2],s2-1);
	return cmd_wrapper(&msg,argv[0]);
}

static int cmd_setenv(int argc,char**argv){
	if(argc>3)return re_printf(2,"too many arguments\n");
	if(argc<2)return re_printf(2,"missing arguments\n");
	#define xkey msg.data.env.key
	#define xvalue msg.data.env.value
	char*key,*value;
	struct init_msg msg;
	size_t klen=0,vlen=0;
	size_t s1=sizeof(xkey),s2=sizeof(xvalue);
	switch(argc){
		case 2:
			if(!(value=strchr(argv[1],'=')))
				return re_printf(2,"missing arguments\n");
			klen=value-argv[1],vlen=strlen(++value),key=argv[1];
		break;
		case 3:
			key=argv[1],value=argv[2];
			klen=strlen(key),vlen=strlen(value);
		break;
	}
	if(klen<=0)return re_printf(2,"invalid environ name\n");
	if(klen>=s1||vlen>=s2)return re_printf(2,"arguments too long\n");
	init_initialize_msg(&msg,ACTION_ADDENV);
	strncpy(xkey,key,klen);
	strncpy(xvalue,value,vlen);
	return cmd_wrapper(&msg,argv[0]);
}

int initctl_main(int argc,char**argv){
	static const struct option lo[]={
		{"help",    no_argument,       NULL,'h'},
		{NULL,0,NULL,0}
	};
	char*socket=NULL;
	int o;
	while((o=b_getlopt(argc,argv,"h",lo,NULL))>0)switch(o){
		case 'h':return usage(0);
		case 's':
			if(socket)goto conflict;
			socket=b_optarg;
		break;
		default:return 1;
	}
	if(!socket)socket=DEFAULT_INITD;
	if(open_socket_initfd(socket)<0)return 2;
	if(b_optind>=argc)return re_printf(1,"no command specified\n");
	int ac=argc-b_optind;
	char**av=argv+b_optind,*vn=argv[b_optind];
	if(strcmp(vn,"poweroff")==0)return cmd_poweroff(ac,av);
	else if(strcmp(vn,"halt")==0)return cmd_halt(ac,av);
	else if(strcmp(vn,"reboot")==0)return cmd_reboot(ac,av);
	else if(strcmp(vn,"switchroot")==0)return cmd_switchroot(ac,av);
	else if(strcmp(vn,"setenv")==0)return cmd_setenv(ac,av);
	else if(strcmp(vn,"addenv")==0)return cmd_setenv(ac,av);
	else return re_printf(1,"unknown command: %s\n",vn);
	conflict:return re_printf(2,"too many arguments\n");
}
