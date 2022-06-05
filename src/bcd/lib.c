/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_HIVEX
#define _GNU_SOURCE
#include<string.h>
#include<endian.h>
#include<uuid/uuid.h>
#include"bcdstore.h"

uuid_t*guid2uuid(uuid_t*uuid,guid_t guid){
	if(!uuid)return NULL;
	guid_t new;
	new.u1=be32toh(guid.u1);
	new.u2=be16toh(guid.u2);
	new.u3=be16toh(guid.u3);
	new.u4=guid.u4;
	memcpy(uuid,&new,16);
	return uuid;
}

guid_t*uuid2guid(guid_t*guid,uuid_t uuid){
	if(!guid)return NULL;
	memcpy(guid,uuid,16);
	guid->u1=htobe32(guid->u1);
	guid->u2=htobe16(guid->u2);
	guid->u3=htobe16(guid->u3);
	return guid;
}

bool bcd_get_guid_by_name(const char*name,uuid_t uuid){
	if(!name||!uuid)return NULL;
	for(size_t s=0;BcdGuidTable[s].name;s++){
		if(strcmp(BcdGuidTable[s].name,name)!=0)continue;
		uuid_copy(uuid,BcdGuidTable[s].uuid);
		return true;
	}
	return false;
}

const char*bcd_get_name_by_guid(uuid_t uuid){
	if(!uuid)return NULL;
	for(size_t s=0;BcdGuidTable[s].name;s++)
		if(uuid_compare(BcdGuidTable[s].uuid,uuid)==0)
			return BcdGuidTable[s].name;
	return NULL;
}

#endif
