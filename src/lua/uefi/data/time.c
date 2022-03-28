/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"

static int LuaUefiTimeToString(lua_State*L){
	GET_TIME(L,1,tm);
	CHAR8 buff[32];
	AsciiSPrint(buff,sizeof(buff),
		"%04d-%02d-%02d %02d:%02d:%02d",
		tm->time.Year,
		tm->time.Month,
		tm->time.Day,
		tm->time.Hour,
		tm->time.Minute,
		tm->time.Second
	);
	lua_pushstring(L,buff);
	return 1;
}

static int LuaUefiTimeYear(lua_State*L){
	GET_TIME(L,1,tm);
	if(!lua_isnoneornil(L,2))
		tm->time.Year=luaL_checkinteger(L,2);
	lua_pushinteger(L,tm->time.Year);
	return 1;
}

static int LuaUefiTimeMonth(lua_State*L){
	GET_TIME(L,1,tm);
	if(!lua_isnoneornil(L,2))
		tm->time.Month=luaL_checkinteger(L,2);
	lua_pushinteger(L,tm->time.Month);
	return 1;
}

static int LuaUefiTimeDay(lua_State*L){
	GET_TIME(L,1,tm);
	if(!lua_isnoneornil(L,2))
		tm->time.Day=luaL_checkinteger(L,2);
	lua_pushinteger(L,tm->time.Day);
	return 1;
}

static int LuaUefiTimeHour(lua_State*L){
	GET_TIME(L,1,tm);
	if(!lua_isnoneornil(L,2))
		tm->time.Hour=luaL_checkinteger(L,2);
	lua_pushinteger(L,tm->time.Hour);
	return 1;
}

static int LuaUefiTimeMinute(lua_State*L){
	GET_TIME(L,1,tm);
	if(!lua_isnoneornil(L,2))
		tm->time.Minute=luaL_checkinteger(L,2);
	lua_pushinteger(L,tm->time.Minute);
	return 1;
}

static int LuaUefiTimeSecond(lua_State*L){
	GET_TIME(L,1,tm);
	if(!lua_isnoneornil(L,2))
		tm->time.Second=luaL_checkinteger(L,2);
	lua_pushinteger(L,tm->time.Second);
	return 1;
}

static int LuaUefiTimeNanosecond(lua_State*L){
	GET_TIME(L,1,tm);
	if(!lua_isnoneornil(L,2))
		tm->time.Nanosecond=luaL_checkinteger(L,2);
	lua_pushinteger(L,tm->time.Nanosecond);
	return 1;
}

static int LuaUefiTimeTimeZone(lua_State*L){
	GET_TIME(L,1,tm);
	if(!lua_isnoneornil(L,2))
		tm->time.TimeZone=luaL_checkinteger(L,2);
	lua_pushinteger(L,tm->time.TimeZone);
	return 1;
}

static int LuaUefiTimeDaylight(lua_State*L){
	GET_TIME(L,1,tm);
	if(!lua_isnoneornil(L,2))
		tm->time.Daylight=luaL_checkinteger(L,2);
	lua_pushinteger(L,tm->time.Daylight);
	return 1;
}

void uefi_time_to_lua(lua_State*L,CONST EFI_TIME*time){
	struct lua_uefi_time_data*e;
	if(!time){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_time_data));
	luaL_getmetatable(L,LUA_UEFI_HANDLE);
	lua_setmetatable(L,-2);
	CopyMem(&e->time,time,sizeof(EFI_TIME));
}

struct lua_uefi_meta_table LuaUefiTimeMetaTable={
	.name=LUA_UEFI_TIME,
	.reg=(const luaL_Reg[]){
		{"ToString",      LuaUefiTimeToString},
		{"GetYear",       LuaUefiTimeYear},
		{"GetMonth",      LuaUefiTimeMonth},
		{"GetDay",        LuaUefiTimeDay},
		{"GetHour",       LuaUefiTimeHour},
		{"GetMinute",     LuaUefiTimeMinute},
		{"GetSecond",     LuaUefiTimeSecond},
		{"GetNanosecond", LuaUefiTimeNanosecond},
		{"GetTimeZone",   LuaUefiTimeTimeZone},
		{"GetDaylight",   LuaUefiTimeDaylight},
		{"SetYear",       LuaUefiTimeYear},
		{"SetMonth",      LuaUefiTimeMonth},
		{"SetDay",        LuaUefiTimeDay},
		{"SetHour",       LuaUefiTimeHour},
		{"SetMinute",     LuaUefiTimeMinute},
		{"SetSecond",     LuaUefiTimeSecond},
		{"SetNanosecond", LuaUefiTimeNanosecond},
		{"SetTimeZone",   LuaUefiTimeTimeZone},
		{"SetDaylight",   LuaUefiTimeDaylight},
		{NULL, NULL}
	},
	.tostring=LuaUefiTimeToString,
	.gc=NULL,
};

#endif
