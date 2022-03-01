/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"internal.h"

#define IMBR loc->part_proto->Info.Mbr

locate_match_state locate_match_mbr_type(locate_dest*loc){
	int64_t type=GINT("by_mbr_type",0);
	if(type==0)return MATCH_SKIP;
	if(type<=0||type>0xFF){
		tlog_warn("invalid mbr type %02llx",(long long)type);
		return MATCH_INVALID;
	}
	if(!loc||!loc->part_proto)
		return MATCH_SKIP;
	if(loc->part_proto->Type!=PARTITION_TYPE_MBR)
		return MATCH_FAILED;
	if(IMBR.OSIndicator!=type)
		return MATCH_FAILED;
	return MATCH_SUCCESS;
}

locate_match_state locate_match_mbr_active(locate_dest*loc){
	bool mbr_boot=GBOOL("by_mbr_active",false);
	if(!mbr_boot)return MATCH_SKIP;
	if(!loc||!loc->part_proto)
		return MATCH_SKIP;
	if(loc->part_proto->Type!=PARTITION_TYPE_MBR)
		return MATCH_FAILED;
	if(IMBR.BootIndicator!=0x80)
		return MATCH_FAILED;
	return MATCH_SUCCESS;
}
