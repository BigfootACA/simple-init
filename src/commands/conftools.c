#include<errno.h>
#include<stdio.h>
#include<unistd.h>
#include"confd.h"
#include"output.h"
#include"defines.h"

extern int confctl_do_get(char*key);
int confget_main(int argc,char**argv){
	if(argc!=2)return re_printf(1,"Usage: confget <KEY>\n");
	open_default_confd_socket("confget");
	return confctl_do_get(argv[1]);
}

extern int confctl_do_set(char*key,char*value);
int confset_main(int argc,char**argv){
	if(argc!=3)return re_printf(1,"Usage: confset <KEY> <VALUE>\n");
	open_default_confd_socket("confset");
	return confctl_do_set(argv[1],argv[2]);
}

int confdel_main(int argc,char**argv){
	if(argc!=2)return re_printf(1,"Usage: confdel <KEY>\n");
	open_default_confd_socket("confdel");
	errno=0;
	confd_delete(argv[1]);
	if(errno!=0)perror(_("config daemon delete failed"));
	return errno;
}

int confdump_main(int argc,char**argv __attribute__((unused))){
	if(argc!=1)return re_printf(1,"Usage: confdump\n");
	open_default_confd_socket("confdump");
	errno=0;
	confd_dump();
	if(errno!=0)perror(_("config daemon dump failed"));
	return errno;
}
