#include"boot.h"

extern int run_boot_root(boot_config*boot);
extern int run_boot_system(boot_config*boot);

boot_config boot_switchroot={
	.mode=BOOT_SWITCHROOT,
	.ident="switchroot",
	.desc="Default SwitchRoot",
	.data={},
	.main=&run_boot_root
};

boot_config boot_system={
	.mode=BOOT_NONE,
	.ident="system",
	.desc="Default System",
	.data={},
	.main=&run_boot_system
};
