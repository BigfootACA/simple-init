/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_PARTITION_INFO "UEFI Partition Info Protocol"
struct lua_uefi_partition_info_proto{EFI_PARTITION_INFO_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_partition_info_proto,LUA_UEFI_PROTO_PARTITION_INFO)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static void get_mbr(lua_State*L,MBR_PARTITION_RECORD*mbr){
	lua_createtable(L,0,0);
	lua_pushliteral(L,"BootIndicator");
	lua_pushinteger(L,mbr->BootIndicator);
	lua_settable(L,-3);
	lua_pushliteral(L,"StartHead");
	lua_pushinteger(L,mbr->StartHead);
	lua_settable(L,-3);
	lua_pushliteral(L,"StartSector");
	lua_pushinteger(L,mbr->StartSector);
	lua_settable(L,-3);
	lua_pushliteral(L,"StartTrack");
	lua_pushinteger(L,mbr->StartTrack);
	lua_settable(L,-3);
	lua_pushliteral(L,"OSIndicator");
	lua_pushinteger(L,mbr->OSIndicator);
	lua_settable(L,-3);
	lua_pushliteral(L,"EndHead");
	lua_pushinteger(L,mbr->EndHead);
	lua_settable(L,-3);
	lua_pushliteral(L,"EndSector");
	lua_pushinteger(L,mbr->EndSector);
	lua_settable(L,-3);
	lua_pushliteral(L,"EndTrack");
	lua_pushinteger(L,mbr->EndTrack);
	lua_settable(L,-3);
	lua_pushliteral(L,"StartingLBA");
	lua_pushinteger(L,SwapBytes32(*(UINT32*)mbr->StartingLBA));
	lua_settable(L,-3);
	lua_pushliteral(L,"SizeInLBA");
	lua_pushinteger(L,SwapBytes32(*(UINT32*)mbr->SizeInLBA));
	lua_settable(L,-3);
}

static void get_gpt(lua_State*L,EFI_PARTITION_ENTRY*gpt){
	lua_createtable(L,0,0);
	lua_pushliteral(L,"PartitionTypeGUID");
	uefi_guid_to_lua(L,&gpt->PartitionTypeGUID);
	lua_settable(L,-3);
	lua_pushliteral(L,"UniquePartitionGUID");
	uefi_guid_to_lua(L,&gpt->UniquePartitionGUID);
	lua_settable(L,-3);
	lua_pushliteral(L,"StartingLBA");
	lua_pushinteger(L,gpt->StartingLBA);
	lua_settable(L,-3);
	lua_pushliteral(L,"EndingLBA");
	lua_pushinteger(L,gpt->EndingLBA);
	lua_settable(L,-3);
	lua_pushliteral(L,"Attributes");
	lua_pushinteger(L,gpt->Attributes);
	lua_settable(L,-3);
	lua_pushliteral(L,"PartitionName");
	uefi_char16_a16_to_lua(L,gpt->PartitionName);
	lua_settable(L,-3);
}

static int LuaUefiPartitionInfoProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_PARTITION_INFO" %p",proto->proto);
	return 1;
}

static int LuaUefiPartitionInfoProtocolType(lua_State*L){
	GET_PROTO(L,1,proto);
	switch(proto->proto->Type){
		case PARTITION_TYPE_MBR:lua_pushstring(L,"mbr");break;
		case PARTITION_TYPE_GPT:lua_pushstring(L,"gpt");break;
		default:lua_pushnil(L);break;
	}
	return 1;
}

static int LuaUefiPartitionInfoProtocolInfo(lua_State*L){
	GET_PROTO(L,1,proto);
	switch(proto->proto->Type){
		case PARTITION_TYPE_MBR:get_mbr(L,&proto->proto->Info.Mbr);break;
		case PARTITION_TYPE_GPT:get_gpt(L,&proto->proto->Info.Gpt);break;
		default:lua_pushnil(L);break;
	}
	return 1;
}

static int LuaUefiPartitionInfoProtocolSystem(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushinteger(L,proto->proto->System);
	return 1;
}

static int LuaUefiPartitionInfoProtocolIsSystem(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushboolean(L,proto->proto->System==1);
	return 1;
}

static int LuaUefiPartitionInfoProtocolIsGPT(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushboolean(L,proto->proto->Type==PARTITION_TYPE_GPT);
	return 1;
}

static int LuaUefiPartitionInfoProtocolIsMBR(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushboolean(L,proto->proto->Type==PARTITION_TYPE_MBR);
	return 1;
}

static int LuaUefiPartitionInfoProtocolRevision(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushinteger(L,proto->proto->Revision);
	return 1;
}

EFI_PARTITION_INFO_PROTOCOL*uefi_lua_to_partition_info_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_partition_info_protocol_to_lua(lua_State*L,EFI_PARTITION_INFO_PROTOCOL*proto){
	struct lua_uefi_partition_info_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_partition_info_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_PARTITION_INFO);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiPartitionInfoProtocolMetaTable={
	.name=LUA_UEFI_PROTO_PARTITION_INFO,
	.reg=(const luaL_Reg[]){
		{"Revision", LuaUefiPartitionInfoProtocolRevision},
		{"Info",     LuaUefiPartitionInfoProtocolInfo},
		{"Type",     LuaUefiPartitionInfoProtocolType},
		{"System",   LuaUefiPartitionInfoProtocolSystem},
		{"IsSystem", LuaUefiPartitionInfoProtocolIsSystem},
		{"IsGPT",    LuaUefiPartitionInfoProtocolIsGPT},
		{"IsMBR",    LuaUefiPartitionInfoProtocolIsMBR},
		{NULL, NULL}
	},
	.tostring=LuaUefiPartitionInfoProtocolToString,
	.gc=NULL,
};

#endif
