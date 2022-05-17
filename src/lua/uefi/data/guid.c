/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"

#define LUA_UEFI_GUID     "UEFI GUID"
#define OPT_GUID(L,n,var) OPT_UDATA(L,n,var,lua_uefi_guid_data,LUA_UEFI_GUID)
#define GET_GUID(L,n,var) OPT_GUID(L,n,var);CHECK_NULL(L,n,var)

static int LuaUefiGuidToRawString(lua_State*L){
	char buff[64];
	GET_GUID(L,1,guid);
	ZeroMem(buff,sizeof(buff));
	AsciiSPrint(buff,sizeof(buff),"%g",&guid->guid);
	lua_pushstring(L,buff);
	return 1;
}

static int LuaUefiGuidToString(lua_State*L){
	char buff[64];
	GET_GUID(L,1,guid);
	ZeroMem(buff,sizeof(buff));
	AsciiSPrint(buff,sizeof(buff),"%g",&guid->guid);
	lua_pushstring(L,buff);
	return 1;
}

static int LuaUefiGuidFromString(lua_State*L){
	GET_GUID(L,1,guid);
	const char*str=luaL_checkstring(L,2);
	ZeroMem(&guid->guid,sizeof(EFI_GUID));
	uefi_status_to_lua(L,AsciiStrToGuid(str,&guid->guid));
	return 1;
}

static int LuaUefiGuidToProtocol(lua_State*L){
	size_t ds=0;
	void*reg=NULL;
	GET_GUID(L,1,guid);
	OPT_BS(L,2,bs);
	lua_arg_get_data(L,3,true,&reg,&ds);
	VOID*d=NULL;
	EFI_BOOT_SERVICES*b=bs?bs->bs:gBS;
	EFI_STATUS status=b->LocateProtocol(
		&guid->guid,reg,&d
	);
	uefi_status_to_lua(L,status);
	uefi_data_to_protocol(L,&guid->guid,d,TRUE);
	return 2;
}

void uefi_guid_to_lua(lua_State*L,CONST EFI_GUID*guid){
	struct lua_uefi_guid_data*e;
	if(!guid){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_guid_data));
	luaL_getmetatable(L,LUA_UEFI_GUID);
	lua_setmetatable(L,-2);
	CopyGuid(&e->guid,guid);
}

void uefi_str_guid_to_lua(lua_State*L,CHAR8*str){
	EFI_GUID guid;
	AsciiStrToGuid(str,&guid);
	uefi_guid_to_lua(L,&guid);
}

void uefi_char16_guid_to_lua(lua_State*L,CHAR16*str){
	EFI_GUID guid;
	StrToGuid(str,&guid);
	uefi_guid_to_lua(L,&guid);
}

bool lua_arg_get_guid(lua_State*L,int idx,bool nil,EFI_GUID*guid){
	switch(lua_type(L,idx)){
		case LUA_TSTRING:{
			const char*a=lua_tostring(L,idx);
			if(guid)uefi_str_to_guid(a,guid);
			else return false;
		}break;
		case LUA_TUSERDATA:{
			struct lua_data*d1;
			if((d1=luaL_testudata(L,idx,LUA_DATA))){
				if(guid)uefi_str_to_guid((char*)d1->data,guid);
				else return false;
				break;
			}
			struct lua_uefi_guid_data*d2;
			if((d2=luaL_testudata(L,idx,LUA_UEFI_GUID))){
				if(guid)CopyGuid(guid,&d2->guid);
				else return false;
				break;
			}
		}break;
		case LUA_TNIL:case LUA_TNONE:
			if(!nil)luaL_argerror(L,idx,"required argument");
			else return false;
		break;
		default:luaL_argerror(L,idx,"argument type unknown");
	}
	return true;
}

struct lua_uefi_meta_table LuaUefiGuidMetaTable={
	.name=LUA_UEFI_GUID,
	.reg=(const luaL_Reg[]){
		{"ToString",   LuaUefiGuidToRawString},
		{"ToProtocol", LuaUefiGuidToProtocol},
		{"SetString",  LuaUefiGuidFromString},
		{"FromString", LuaUefiGuidFromString},
		{NULL, NULL}
	},
	.tostring=LuaUefiGuidToString,
	.gc=NULL,
};

#endif
