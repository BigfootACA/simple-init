/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"internal.h"

#define IGPT loc->part_proto->Info.Gpt

locate_match_state locate_match_gpt_name(locate_dest*loc){
	char pn[32];
	char*name=GSTR("by_gpt_name",NULL);
	if(!name)return MATCH_SKIP;
	if(!*name||strlen(name)>=32){
		tlog_warn("invalid gpt name");
		free(name);
		return MATCH_INVALID;
	}
	if(
		!loc||!loc->part_proto||
		loc->part_proto->Type!=PARTITION_TYPE_GPT
	){
		free(name);
		return MATCH_FAILED;
	}
	ZeroMem(pn,sizeof(pn));
	UnicodeStrToAsciiStrS(
		IGPT.PartitionName,
		pn,sizeof(pn)
	);
	locate_match_state st=MATCH_SUCCESS;
	if(AsciiStriCmp(pn,name)!=0)st=MATCH_FAILED;
	free(name);
	return st;
}

locate_match_state locate_match_gpt_guid(locate_dest*loc){
	static GUID guid={0};
	char*str=GSTR("by_gpt_guid",NULL);
	if(!str)return MATCH_SKIP;
	if(!loc||!loc->part_proto)return MATCH_FAILED;
	ZeroMem(&guid,sizeof(GUID));
	RETURN_STATUS st=AsciiStrToGuid(str,&guid);
	if(st!=RETURN_SUCCESS){
		tlog_warn(
			"invalid gpt guid %s: %s",
			str,efi_status_to_string(st)
		);
		free(str);
		return MATCH_INVALID;
	}
	free(str);
	if(loc->part_proto->Type!=PARTITION_TYPE_GPT)
		return MATCH_FAILED;
	if(!CompareGuid(
		&guid,
		&IGPT.UniquePartitionGUID
	))return MATCH_FAILED;
	return MATCH_SUCCESS;
}

locate_match_state locate_match_gpt_type(locate_dest*loc){
	static GUID guid={0};
	char*str=GSTR("by_gpt_type",NULL);
	if(!str)return MATCH_SKIP;
	if(!loc||!loc->part_proto)return MATCH_FAILED;
	ZeroMem(&guid,sizeof(GUID));
	RETURN_STATUS st=AsciiStrToGuid(str,&guid);
	if(st!=RETURN_SUCCESS){
		tlog_warn(
			"invalid gpt type guid %s: %s",
			str,efi_status_to_string(st)
		);
		free(str);
		return MATCH_INVALID;
	}
	free(str);
	if(loc->part_proto->Type!=PARTITION_TYPE_GPT)
		return MATCH_FAILED;
	if(!CompareGuid(
		&guid,
		&IGPT.PartitionTypeGUID
	))return MATCH_FAILED;
	return MATCH_SUCCESS;
}
