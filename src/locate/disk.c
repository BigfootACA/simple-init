/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"internal.h"

locate_match_state locate_match_disk_label(locate_dest*loc){
	bool gpt=false,mbr=false;
	char*label=GSTR("by_disk_label",NULL);
	if(!label)return MATCH_SKIP;
	if(!loc||!loc->part_proto)return MATCH_FAILED;
	if(AsciiStriCmp(label,"gpt")==0)gpt=true;
	if(AsciiStriCmp(label,"guid")==0)gpt=true;
	if(AsciiStriCmp(label,"efi")==0)gpt=true;
	if(AsciiStriCmp(label,"mbr")==0)mbr=true;
	if(AsciiStriCmp(label,"dos")==0)mbr=true;
	if(AsciiStriCmp(label,"msdos")==0)mbr=true;
	if(AsciiStriCmp(label,"legacy")==0)mbr=true;
	free(label);
	if(loc->part_proto->Type==PARTITION_TYPE_MBR&&!mbr)return MATCH_FAILED;
	if(loc->part_proto->Type==PARTITION_TYPE_GPT&&!gpt)return MATCH_FAILED;
	return MATCH_SUCCESS;
}

locate_match_state locate_match_esp(locate_dest*loc){
	if(!GBOOL("by_esp",false))return MATCH_SKIP;
	if(!loc||!loc->part_proto)return MATCH_FAILED;
	return loc->part_proto->System==1?MATCH_SUCCESS:MATCH_FAILED;
}

locate_match_state locate_match_device_path(locate_dest*loc){
	CHAR16*dpt;
	char xpx[PATH_MAX],*xpt;
	EFI_DEVICE_PATH_PROTOCOL*dp;
	locate_match_state st=MATCH_FAILED;
	if(!(dp=DevicePathFromHandle(loc->file_hand)))return MATCH_SKIP;
	if(!(xpt=GSTR("by_device_path",NULL)))return MATCH_SKIP;
	if(!(dpt=ConvertDevicePathToText(dp,FALSE,FALSE)))return MATCH_INVALID;
	ZeroMem(xpx,sizeof(xpx));
	UnicodeStrToAsciiStrS(dpt,xpx,sizeof(xpx));
	if(AsciiStriCmp(xpx,xpt)==0)st=MATCH_SUCCESS;
	return st;
}
