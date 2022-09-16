/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fs_internal.h"

list*fs_drivers=NULL;
list*fs_volumes;
list*fs_volume_infos;
mutex_t fsdrv_lock;
mutex_t fsvol_lock;
mutex_t fsvol_info_lock;
fs_initiator_function*fs_initiator[]={
	NULL
};
