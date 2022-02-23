/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<stdlib.h>
#include<Library/UefiBootServicesTableLib.h>
#include"boot.h"

int run_boot_exit(boot_config*boot){
	gBS->Exit(gImageHandle,EFI_ABORTED,0,NULL);
	return -1;
}
