/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"

#define LUA_UEFI_CHAR16     "UEFI 16-bit String"
#define OPT_CHAR16(L,n,var) OPT_UDATA(L,n,var,lua_uefi_char16_data,LUA_UEFI_CHAR16)
#define GET_CHAR16(L,n,var) OPT_CHAR16(L,n,var);CHECK_NULL(L,n,var)
static void clean_char16(struct lua_uefi_char16_data*c16){
	if(c16->allocated)FreePool(c16->string);
	c16->allocated=FALSE;
	c16->string=NULL;
}

static int LuaUefiChar16ToString(lua_State*L){
	GET_CHAR16(L,1,c16);
	if(!c16->string)goto fail;
	UINTN len=StrLen(c16->string)+1;
	char*buff=AllocateZeroPool(len);
	if(!buff)goto fail;
	UnicodeStrToAsciiStrS(c16->string,buff,len);
	lua_pushstring(L,buff);
	FreePool(buff);
	return 1;
	fail:lua_pushnil(L);
	return 1;
}

static int LuaUefiChar16ToData(lua_State*L){
	GET_CHAR16(L,1,c16);
	if(!c16->string)goto fail;
	uefi_data_to_lua(L,FALSE,c16->string,StrSize(c16->string));
	return 1;
	fail:lua_pushnil(L);
	return 1;
}

static int LuaUefiChar16FromString(lua_State*L){
	GET_CHAR16(L,1,c16);
	const char*str=luaL_checkstring(L,2);
	clean_char16(c16);
	UINTN len=AsciiStrSize(str)*sizeof(CHAR16);
	if(!(c16->string=AllocateZeroPool(len+1)))goto done;
	AsciiStrToUnicodeStrS(str,c16->string,len);
	c16->allocated=TRUE;
	done:lua_pushboolean(L,c16->allocated);
	return 1;
}

static int LuaUefiChar16Copy(lua_State*L){
	GET_CHAR16(L,1,c16);
	uefi_char16_a16_to_lua(L,c16->string);
	return 0;
}

static int LuaUefiChar16Compare(lua_State*L){
	GET_CHAR16(L,1,first);
	GET_CHAR16(L,2,second);
	lua_Integer i=-1;
	if(first->string&&second->string)
		i=StrCmp(first->string,second->string);
	lua_pushinteger(L,i);
	return 1;
}

static int LuaUefiChar16Equals(lua_State*L){
	GET_CHAR16(L,1,first);
	GET_CHAR16(L,2,second);
	BOOLEAN i=FALSE;
	if(first->string&&second->string)
		i=StrCmp(first->string,second->string)==0;
	lua_pushboolean(L,i);
	return 1;
}

static int LuaUefiChar16GarbageCollect(lua_State*L){
	GET_CHAR16(L,1,c16);
	clean_char16(c16);
	return 0;
}

void uefi_char16_16_to_lua(lua_State*L,BOOLEAN allocated,CHAR16*string){
	struct lua_uefi_char16_data*e;
	if(!string){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_char16_data));
	luaL_getmetatable(L,LUA_UEFI_CHAR16);
	lua_setmetatable(L,-2);
	e->allocated=allocated;
	e->string=string;
}

void uefi_char16_an16_to_lua(lua_State*L,CHAR16*string,UINTN buff_len){
	if(!string)goto fail;
	UINTN size=buff_len+sizeof(CHAR16);
	CHAR16*str=AllocateZeroPool(size);
	if(!str)goto fail;
	StrCpyS(str,size,string);
	uefi_char16_16_to_lua(L,TRUE,str);
	return;
	fail:lua_pushnil(L);
}

void uefi_char16_an8_to_lua(lua_State*L,CHAR8*string,UINTN buff_len){
	if(!string)goto fail;
	CHAR16*str=AllocateZeroPool((buff_len+1)*sizeof(CHAR16));
	if(!str)goto fail;
	AsciiStrToUnicodeStrS(string,str,buff_len);
	uefi_char16_16_to_lua(L,TRUE,str);
	return;
	fail:lua_pushnil(L);
}

void uefi_char16_a16_to_lua(lua_State*L,CHAR16*string){
	uefi_char16_an16_to_lua(L,string,StrSize(string));
}

void uefi_char16_a8_to_lua(lua_State*L,CHAR8*string){
	uefi_char16_an8_to_lua(L,string,AsciiStrSize(string));
}

void lua_arg_get_char16(lua_State*L,int idx,bool nil,CHAR16**data){
	switch(lua_type(L,idx)){
		case LUA_TSTRING:{
			const char*a=lua_tostring(L,idx);
			if(data){
				UINTN s=(AsciiStrLen(a)+1)*sizeof(CHAR16);
				if((*data=AllocateZeroPool(s)))
					AsciiStrToUnicodeStrS(a,*data,s);
			}
		}break;
		case LUA_TUSERDATA:{
			struct lua_data*d1;
			if((d1=luaL_testudata(L,idx,LUA_DATA))){
				if(data&&d1->data&&d1->size>0)
					*data=AllocateCopyPool(d1->size,d1->data);
				break;
			}
			struct lua_uefi_char16_data*d2;
			if((d2=luaL_testudata(L,idx,LUA_UEFI_CHAR16))){
				if(data&&d2->string)*data=AllocateCopyPool(
					StrSize(d2->string),
					d2->string
				);
				break;
			}
		}break;
		case LUA_TNIL:case LUA_TNONE:
			if(!nil)luaL_argerror(L,idx,"required argument");
		break;
		default:luaL_argerror(L,idx,"argument type unknown");
	}
}

struct lua_uefi_meta_table LuaUefiChar16MetaTable={
	.name=LUA_UEFI_CHAR16,
	.reg=(const luaL_Reg[]){
		{"ToString",   LuaUefiChar16ToString},
		{"ToData",     LuaUefiChar16ToData},
		{"SetString",  LuaUefiChar16FromString},
		{"FromString", LuaUefiChar16FromString},
		{"Compare",    LuaUefiChar16Compare},
		{"Equals",     LuaUefiChar16Equals},
		{"Copy",       LuaUefiChar16Copy},
		{"Duplicate",  LuaUefiChar16Copy},
		{NULL, NULL}
	},
	.tostring=LuaUefiChar16ToString,
	.gc=LuaUefiChar16GarbageCollect,
};

#endif
