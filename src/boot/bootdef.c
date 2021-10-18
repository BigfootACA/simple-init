/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"boot.h"

extern int run_boot_root(boot_config*boot);
extern int run_boot_system(boot_config*boot);
extern int run_boot_charger(boot_config*boot);

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

boot_config boot_charger={
	.mode=BOOT_CHARGER,
	.ident="charger",
	.desc="Default Charger Screen",
	.data={},
	.main=&run_boot_charger
};
