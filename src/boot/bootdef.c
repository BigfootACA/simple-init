#include"boot.h"

extern int run_boot_root(boot_config*boot);

boot_config boot_switchroot={
	.mode=BOOT_SWITCHROOT,
	.ident="switchroot",
	.desc="Default SwitchRoot",
	.data={},
	.main=&run_boot_root
};
