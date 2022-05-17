/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"

static int LuaUefiRTGetTime(lua_State*L){
	EFI_TIME tm;
	EFI_TIME_CAPABILITIES cap;
	GET_RT(L,1,rt);
	EFI_STATUS status=rt->rt->GetTime(&tm,&cap);
	uefi_status_to_lua(L,status);
	uefi_time_to_lua(L,&tm);
	lua_createtable(L,0,0);
	lua_pushliteral(L,"Resolution");
	lua_pushinteger(L,cap.Resolution);
	lua_settable(L,-3);
	lua_pushliteral(L,"Accuracy");
	lua_pushinteger(L,cap.Accuracy);
	lua_settable(L,-3);
	lua_pushliteral(L,"SetsToZero");
	lua_pushboolean(L,cap.SetsToZero);
	lua_settable(L,-3);
	return 3;
}

static int LuaUefiRTSetTime(lua_State*L){
	GET_RT(L,1,rt);
	GET_TIME(L,2,tm);
	EFI_STATUS status=rt->rt->SetTime(&tm->time);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiRTGetWakeupTime(lua_State*L){
	EFI_TIME tm;
	BOOLEAN enable=FALSE,pending=FALSE;
	GET_RT(L,1,rt);
	EFI_STATUS status=rt->rt->GetWakeupTime(
		&enable,&pending,&tm
	);
	uefi_status_to_lua(L,status);
	lua_pushboolean(L,enable);
	lua_pushboolean(L,pending);
	uefi_time_to_lua(L,&tm);
	return 4;
}

static int LuaUefiRTSetWakeupTime(lua_State*L){
	GET_RT(L,1,rt);
	luaL_checktype(L,2,LUA_TBOOLEAN);
	BOOLEAN enable=lua_toboolean(L,2);
	GET_TIME(L,3,tm);
	EFI_STATUS status=rt->rt->SetWakeupTime(enable,&tm->time);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiRTSetVirtualAddressMap(lua_State*L){
	//GET_RT(L,1,rt);
	// TODO: implement it
	return luaL_error(L,"not implemented");
}

static int LuaUefiRTGetVariable(lua_State*L){
	EFI_GUID guid;
	CHAR16*name=NULL;
	GET_RT(L,1,rt);
	lua_arg_get_char16(L,2,false,&name);
	lua_arg_get_guid(L,3,false,&guid);
	if(!name)return luaL_argerror(L,2,"get argument failed");
	UINT32 attr=0;
	UINTN size=0;
	VOID*data=NULL;
	EFI_STATUS status=rt->rt->GetVariable(
		name,&guid,
		&attr,&size,&data
	);
	if(status==EFI_BUFFER_TOO_SMALL){
		if(!(data=AllocateZeroPool(size)))
			return luaL_error(L,"allocate pool failed");
		status=rt->rt->GetVariable(
			name,&guid,
			&attr,&size,&data
		);
	}
	uefi_status_to_lua(L,status);
	lua_pushinteger(L,attr);
	uefi_data_to_lua(L,FALSE,data,size);
	FreePool(name);
	return 3;
}

static int LuaUefiRTGetNextVariableName(lua_State*L){
	GET_RT(L,1,rt);
	UINTN size=0;
	CHAR16*name=NULL;
	EFI_GUID guid;
	EFI_STATUS status=rt->rt->GetNextVariableName(
		&size,name,&guid
	);
	if(status==EFI_BUFFER_TOO_SMALL){
		if(!(name=AllocateZeroPool(size)))
			return luaL_error(L,"allocate pool failed");
		status=rt->rt->GetNextVariableName(
			&size,name,&guid
		);
	}
	uefi_status_to_lua(L,status);
	uefi_char16_16_to_lua(L,TRUE,name);
	uefi_guid_to_lua(L,&guid);
	return 3;
}

static int LuaUefiRTSetVariable(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	EFI_GUID guid;
	CHAR16*name=NULL;
	GET_RT(L,1,rt);
	lua_arg_get_char16(L,2,false,&name);
	lua_arg_get_guid(L,3,false,&guid);
	if(!name)return luaL_argerror(L,2,"get argument failed");
	lua_Integer attr=luaL_checkinteger(L,4);
	lua_arg_get_data(L,5,false,&data,&ds);
	if(!data)return luaL_argerror(L,5,"get argument failed");
	EFI_STATUS status=rt->rt->SetVariable(
		name,&guid,attr,ds,data
	);
	uefi_status_to_lua(L,status);
	FreePool(name);
	return 1;
}

static int LuaUefiRTGetNextHighMonotonicCount(lua_State*L){
	GET_RT(L,1,rt);
	UINT32 count=0;
	EFI_STATUS status=rt->rt->GetNextHighMonotonicCount(&count);
	uefi_status_to_lua(L,status);
	lua_pushinteger(L,count);
	return 2;
}

static int LuaUefiRTResetSystem(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	GET_RT(L,1,rt);
	const char*_reset=luaL_checkstring(L,2);
	GET_STATUS(L,3,status);
	lua_arg_get_data(L,4,true,&data,&ds);
	EFI_RESET_TYPE reset=0;
	if(!uefi_str_to_reset_type(_reset,&reset))
		return luaL_error(L,"invalid reset type %s",_reset);
	rt->rt->ResetSystem(
		reset,status->st,
		data?ds:0,data
	);
	return 0;
}

static int LuaUefiRTUpdateCapsule(lua_State*L){
	//GET_RT(L,1,rt);
	// TODO: implement it
	return luaL_error(L,"not implemented");
}

static int LuaUefiRTQueryCapsuleCapabilities(lua_State*L){
	//GET_RT(L,1,rt);
	// TODO: implement it
	return luaL_error(L,"not implemented");
}

static int LuaUefiRTQueryVariableInfo(lua_State*L){
	GET_RT(L,1,rt);
	lua_Integer attr=luaL_checkinteger(L,2);
	UINT64 max_var_stor_size=0;
	UINT64 rem_var_stor_size=0;
	UINT64 max_var_size=0;
	EFI_STATUS status=rt->rt->QueryVariableInfo(
		(UINT32)attr,
		&max_var_stor_size,
		&rem_var_stor_size,
		&max_var_size
	);
	uefi_status_to_lua(L,status);
	lua_pushinteger(L,max_var_stor_size);
	lua_pushinteger(L,rem_var_stor_size);
	lua_pushinteger(L,max_var_size);
	return 4;
}

static int LuaUefiRTToString(lua_State*L){
	GET_RT(L,1,rt);
	lua_pushfstring(L,LUA_UEFI_RT" %p",rt->rt);
	return 1;
}

void uefi_rt_to_lua(lua_State*L,EFI_RUNTIME_SERVICES*rt){
	struct lua_uefi_rt_data*e;
	if(!rt){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_rt_data));
	luaL_getmetatable(L,LUA_UEFI_RT);
	lua_setmetatable(L,-2);
	e->rt=rt;
}

struct lua_uefi_meta_table LuaUefiRTMetaTable={
	.name=LUA_UEFI_RT,
	.reg=(const luaL_Reg[]){
		{"GetTime",                   LuaUefiRTGetTime},
		{"SetTime",                   LuaUefiRTSetTime},
		{"GetWakeupTime",             LuaUefiRTGetWakeupTime},
		{"SetWakeupTime",             LuaUefiRTSetWakeupTime},
		{"SetVirtualAddressMap",      LuaUefiRTSetVirtualAddressMap},
		{"GetVariable",               LuaUefiRTGetVariable},
		{"GetNextVariableName",       LuaUefiRTGetNextVariableName},
		{"SetVariable",               LuaUefiRTSetVariable},
		{"GetNextHighMonotonicCount", LuaUefiRTGetNextHighMonotonicCount},
		{"ResetSystem",               LuaUefiRTResetSystem},
		{"UpdateCapsule",             LuaUefiRTUpdateCapsule},
		{"QueryCapsuleCapabilities",  LuaUefiRTQueryCapsuleCapabilities},
		{"QueryVariableInfo",         LuaUefiRTQueryVariableInfo},
		{NULL, NULL}
	},
	.tostring=LuaUefiRTToString,
	.gc=NULL,
};

#endif
