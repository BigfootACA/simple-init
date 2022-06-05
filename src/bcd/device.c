/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_HIVEX
#define _GNU_SOURCE
#include<hivex.h>
#include<string.h>
#include"keyval.h"
#include"bcdstore.h"

bcd_type_device_type bcd_device_get_type(bcd_device dev){
	return dev?dev->dev_type:-1;
}

const char*bcd_device_get_type_name(bcd_device dev){
	bcd_type_device_type t=bcd_device_get_type(dev);
	return t<0||t>=BCD_DT_MAX?NULL:bcd_type_device_type_name[t];
}

bcd_type_local_device_type bcd_device_get_local_type(bcd_device dev){
	return dev?dev->local_dev_type:-1;
}

const char*bcd_device_get_local_type_name(bcd_device dev){
	bcd_type_local_device_type t=bcd_device_get_local_type(dev);
	return t<0||t>=BCD_LDT_MAX?NULL:bcd_type_local_device_type_name[t];
}

bool bcd_device_get_disk_uuid(bcd_device dev,uuid_t*uuid){
	if(!dev||!uuid)return false;
	guid2uuid(uuid,dev->disk_guid);
	return true;
}

char*bcd_device_get_disk_uuid_string(bcd_device dev,char*uuid){
	uuid_t u;
	if(!bcd_device_get_disk_uuid(dev,&u))return NULL;
	uuid_unparse(u,uuid);
	return uuid;
}

bool bcd_device_get_part_uuid(bcd_device dev,uuid_t*uuid){
	if(!dev)return false;
	guid2uuid(uuid,dev->part_guid);
	return true;
}

char*bcd_device_get_part_uuid_string(bcd_device dev,char*uuid){
	uuid_t u;
	if(!bcd_device_get_part_uuid(dev,&u))return NULL;
	uuid_unparse(u,uuid);
	return uuid;
}

void bcd_device_free(bcd_device dev){
	if(dev)free(dev);
}

#endif
