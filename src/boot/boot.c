#include<errno.h>
#include"boot.h"
#include"logger.h"
#include"defines.h"
#define TAG "boot"

int boot(boot_config*boot){
	if(!boot||!boot->main)ERET(EINVAL);
	tlog_info("try to execute boot config %s(%s)",boot->ident,boot->desc);
	return boot->main(boot);
}