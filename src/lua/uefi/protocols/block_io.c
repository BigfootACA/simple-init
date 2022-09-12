/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_BLOCK_IO "UEFI Block IO Protocol"
struct lua_uefi_block_io_proto{EFI_BLOCK_IO_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_block_io_proto,LUA_UEFI_PROTO_BLOCK_IO)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static void load_media(lua_State*L,UINT64 rev,EFI_BLOCK_IO_MEDIA*media){
	lua_createtable(L,0,0);
	lua_pushliteral(L,"MediaId");
	lua_pushinteger(L,media->MediaId);
	lua_settable(L,-3);
	lua_pushliteral(L,"RemovableMedia");
	lua_pushboolean(L,media->RemovableMedia);
	lua_settable(L,-3);
	lua_pushliteral(L,"MediaPresent");
	lua_pushboolean(L,media->MediaPresent);
	lua_settable(L,-3);
	lua_pushliteral(L,"LogicalPartition");
	lua_pushboolean(L,media->LogicalPartition);
	lua_settable(L,-3);
	lua_pushliteral(L,"ReadOnly");
	lua_pushboolean(L,media->ReadOnly);
	lua_settable(L,-3);
	lua_pushliteral(L,"WriteCaching");
	lua_pushboolean(L,media->WriteCaching);
	lua_settable(L,-3);
	lua_pushliteral(L,"BlockSize");
	lua_pushinteger(L,media->BlockSize);
	lua_settable(L,-3);
	lua_pushliteral(L,"IoAlign");
	lua_pushinteger(L,media->IoAlign);
	lua_settable(L,-3);
	lua_pushliteral(L,"LastBlock");
	lua_pushinteger(L,media->LastBlock);
	lua_settable(L,-3);
	lua_pushliteral(L,"TotalSize");
	lua_pushinteger(L,(media->LastBlock-1)*media->BlockSize);
	lua_settable(L,-3);
	lua_pushliteral(L,"LowestAlignedLba");
	#ifdef EFI_BLOCK_IO_PROTOCOL_REVISION2
	if(rev>=EFI_BLOCK_IO_PROTOCOL_REVISION2)
		lua_pushinteger(L,media->LowestAlignedLba);
	else
	#endif
		lua_pushnil(L);
	lua_settable(L,-3);
	lua_pushliteral(L,"LogicalBlocksPerPhysicalBlock");
	#ifdef EFI_BLOCK_IO_PROTOCOL_REVISION2
	if(rev>=EFI_BLOCK_IO_PROTOCOL_REVISION2)
		lua_pushinteger(L,media->LogicalBlocksPerPhysicalBlock);
	else
	#endif
		lua_pushnil(L);
	lua_settable(L,-3);
	lua_pushliteral(L,"OptimalTransferLengthGranularity");
	#ifdef EFI_BLOCK_IO_PROTOCOL_REVISION3
	if(rev>=EFI_BLOCK_IO_PROTOCOL_REVISION3)
		lua_pushinteger(L,media->OptimalTransferLengthGranularity);
	else
	#endif
		lua_pushnil(L);
	lua_settable(L,-3);
}

static int LuaUefiBlockIOProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_BLOCK_IO" %p",proto->proto);
	return 1;
}

static int LuaUefiBlockIOProtocolReset(lua_State*L){
	GET_PROTO(L,1,proto);
	luaL_checktype(L,2,LUA_TBOOLEAN);
	BOOLEAN ext_verify=lua_toboolean(L,2);
	EFI_STATUS status=proto->proto->Reset(proto->proto,ext_verify);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiBlockIOProtocolReadBlocks(lua_State*L){
	VOID*buffer=NULL;
	GET_PROTO(L,1,proto);
	UINT32 media=luaL_optinteger(L,2,proto->proto->Media->MediaId);
	EFI_LBA lba=luaL_checkinteger(L,3);
	UINTN size=luaL_checkinteger(L,4);
	if(!(buffer=AllocateZeroPool(size))){
		uefi_status_to_lua(L,EFI_OUT_OF_RESOURCES);
		lua_pushnil(L);
		return 2;
	}
	EFI_STATUS status=proto->proto->ReadBlocks(
		proto->proto,media,lba,size,buffer
	);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)){
		lua_pushnil(L);
		FreePool(buffer);
	}else uefi_data_to_lua(L,TRUE,buffer,size);
	return 2;
}

static int LuaUefiBlockIOProtocolWriteBlocks(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	GET_PROTO(L,1,proto);
	UINT32 media=luaL_optinteger(L,2,proto->proto->Media->MediaId);
	EFI_LBA lba=luaL_checkinteger(L,3);
	lua_arg_get_data(L,4,false,&data,&ds);
	UINTN size=luaL_optinteger(L,5,ds);
	if(!data)return luaL_argerror(L,4,"empty data");
	if(size<=0)return luaL_argerror(L,5,"empty data");
	EFI_STATUS status=proto->proto->WriteBlocks(
		proto->proto,media,lba,size,data
	);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status))size=0;
	lua_pushinteger(L,size);
	return 2;
}

static int LuaUefiBlockIOProtocolFlushBlocks(lua_State*L){
	GET_PROTO(L,1,proto);
	EFI_STATUS status=proto->proto->FlushBlocks(proto->proto);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiBlockIOProtocolMedia(lua_State*L){
	GET_PROTO(L,1,proto);
	if(!proto->proto->Media)lua_pushnil(L);
	else load_media(L,proto->proto->Revision,proto->proto->Media);
	return 1;
}

static int LuaUefiBlockIOProtocolRevision(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushinteger(L,proto->proto->Revision);
	return 1;
}

EFI_BLOCK_IO_PROTOCOL*uefi_lua_to_block_io_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_block_io_protocol_to_lua(lua_State*L,EFI_BLOCK_IO_PROTOCOL*proto){
	struct lua_uefi_block_io_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_block_io_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_BLOCK_IO);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiBlockIOProtocolMetaTable={
	.name=LUA_UEFI_PROTO_BLOCK_IO,
	.reg=(const luaL_Reg[]){
		{"Revision",    LuaUefiBlockIOProtocolRevision},
		{"Media",       LuaUefiBlockIOProtocolMedia},
		{"Reset",       LuaUefiBlockIOProtocolReset},
		{"ReadBlocks",  LuaUefiBlockIOProtocolReadBlocks},
		{"WriteBlocks", LuaUefiBlockIOProtocolWriteBlocks},
		{"FlushBlocks", LuaUefiBlockIOProtocolFlushBlocks},
		{NULL, NULL}
	},
	.tostring=LuaUefiBlockIOProtocolToString,
	.gc=NULL,
};

#endif
