#include<string.h>
#include"output.h"
#include"defines.h"
#include"str.h"
#include"shell_internal.h"

int help_main(int argc,char**argv __attribute__((unused))){
	if(argc!=1)return re_printf(1,"Usage: help\n");
	size_t max=0;
	const struct shell_command*cmd;
	for(size_t s=0;(cmd=shell_cmds[s]);s++)
		if(cmd->enabled)max=MAX(max,strlen(cmd->name));
	for(size_t s=0;(cmd=shell_cmds[s]);s++){
		if(!cmd->enabled||!cmd->name[0])continue;
		dprintf(STDOUT_FILENO,"%s",cmd->name);
		repeat(STDOUT_FILENO,' ',max-strlen(cmd->name));
		dprintf(STDOUT_FILENO,"  - %s\n",cmd->help);
	}
	return 0;
}