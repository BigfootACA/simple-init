/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"

static struct scan_code_map{
	char*name;
	UINT16 code;
}scan_code_map[]={
	{"insert",          0x0007},{"delete",      0x0008},
	{"page-up",         0x0009},{"page-down",   0x000A},
	{"vol-up",          0x0080},{"volume-up",   0x0080},
	{"brightness-up",   0x0100},{"suspend",     0x0102},
	{"brightness-down", 0x0101},{"hibernate",   0x0103},
	{"vol-down",        0x0081},{"volume-down", 0x0081},
	{"recovery",        0x0105},{"eject",       0x0106},
	{"toggle-display",  0x0104},
	{"null",  0x0000},{"nul",   0x0000},{"up",   0x0001},{"down", 0x0002},
	{"right", 0x0003},{"left",  0x0004},{"home", 0x0005},{"end",  0x0006},
	{"ins",   0x0007},{"del",   0x0008},{"pgup", 0x0009},{"pgdn", 0x000A},
	{"f1",    0x000B},{"f2",    0x000C},{"f3",   0x000D},{"f4",   0x000E},
	{"f5",    0x000F},{"f6",    0x0010},{"f7",   0x0011},{"f8",   0x0012},
	{"f9",    0x0013},{"f10",   0x0014},{"f11",  0x0015},{"f12",  0x0016},
	{"esc",   0x0017},{"pause", 0x0048},{"stop", 0x0048},{"f13",  0x0068},
	{"f14",   0x0069},{"f15",   0x006A},{"f16",  0x006B},{"f17",  0x006C},
	{"f18",   0x006D},{"f19",   0x006E},{"f20",  0x006F},{"f21",  0x0070},
	{"f22",   0x0071},{"f23",   0x0072},{"f24",  0x0073},{"mute", 0x007F},
	{"volup", 0x0080},{"voldn", 0x0081},{"brup", 0x0100},{"brdn", 0x0101},
	{NULL,0}
};

static struct unicode_char_map{
	char*name;
	CHAR16 code;
}unicode_char_map[]={
	{"backspace", 0x0008},{"tab",             0x0009},
	{"line-feed", 0x000A},{"carriage-return", 0x000D},
	{"linefeed",  0x000A},{"carriagereturn",  0x000D},
	{"enter",     0x000D},{"delete",          0x007F},

	{"nul", 0x0000},{"soh", 0x0001},{"stx", 0x0002},{"etx", 0x0003},
	{"eot", 0x0004},{"enq", 0x0005},{"ack", 0x0006},{"bel", 0x0007},
	{"bs",  0x0008},{"ht",  0x0009},{"lf",  0x000A},{"vt",  0x000B},
	{"ff",  0x000C},{"cr",  0x000D},{"so",  0x000E},{"si",  0x000F},
	{"dle", 0x0010},{"dc1", 0x0011},{"dc2", 0x0012},{"dc3", 0x0013},
	{"dc4", 0x0014},{"nak", 0x0015},{"syn", 0x0016},{"etb", 0x0017},
	{"can", 0x0018},{"em",  0x0019},{"sub", 0x001A},{"esc", 0x001B},
	{"fs",  0x001C},{"gs",  0x001D},{"rs",  0x001E},{"us",  0x001F},
	{"del", 0x007F},
	{NULL,0}
};
static int LuaUefiInputKeyToString(lua_State*L){
	GET_INPUT_KEY(L,1,key);
	lua_pushfstring(L,LUA_UEFI_INPUT_KEY" %p",key->key);
	return 1;
}

static int LuaUefiInputKeyScanCode(lua_State*L){
	GET_INPUT_KEY(L,1,key);
	switch(lua_type(L,2)){
		case LUA_TNONE:case LUA_TNIL:break;
		case LUA_TNUMBER:key->key.ScanCode=lua_tointeger(L,2);break;
		case LUA_TSTRING:{
			BOOLEAN found=FALSE;
			struct scan_code_map*m;
			const char*name=lua_tostring(L,2);
			for(int i=0;(m=&scan_code_map[i])->name;i++){
				if(AsciiStriCmp(m->name,name)!=0)continue;
				key->key.ScanCode=m->code;
				found=true;
				break;
			}
			if(found)break;
		}//fallthrough
		default:return luaL_argerror(L,2,"invalid scan code");
	}
	lua_pushinteger(L,key->key.ScanCode);
	return 1;
}

static int LuaUefiInputKeyIsScanCode(lua_State*L){
	BOOLEAN res=FALSE;
	GET_INPUT_KEY(L,1,key);
	switch(lua_type(L,2)){
		case LUA_TNUMBER:
			res=key->key.ScanCode==lua_tointeger(L,2);
		break;
		case LUA_TSTRING:{
			struct scan_code_map*m;
			const char*name=lua_tostring(L,2);
			for(int i=0;(m=&scan_code_map[i])->name;i++){
				if(key->key.ScanCode!=m->code)continue;
				if(AsciiStriCmp(m->name,name)==0)res=TRUE;
			}
		}break;
		default:return luaL_argerror(L,2,"invalid scan code");
	}
	lua_pushboolean(L,res);
	return 1;
}

static int LuaUefiInputKeyUnicodeChar(lua_State*L){
	GET_INPUT_KEY(L,1,key);
	switch(lua_type(L,2)){
		case LUA_TNONE:case LUA_TNIL:break;
		case LUA_TNUMBER:key->key.UnicodeChar=lua_tointeger(L,2);break;
		case LUA_TSTRING:{
			BOOLEAN found=FALSE;
			struct unicode_char_map*m;
			const char*name=lua_tostring(L,2);
			UINTN len=AsciiStrLen(name);
			for(int i=0;(m=&unicode_char_map[i])->name;i++){
				if(AsciiStriCmp(m->name,name)!=0)continue;
				key->key.UnicodeChar=m->code;
				found=true;
				break;
			}
			if(found)break;
			if(len<=2){
				CHAR16 c=0;
				for(UINTN i=0;i<len;i++)c<<=8,c|=name[i];
				key->key.UnicodeChar=c;
			}
		}//fallthrough
		default:return luaL_argerror(L,2,"invalid unicode char");
	}
	lua_pushinteger(L,key->key.UnicodeChar);
	return 1;
}

static int LuaUefiInputKeyIsUnicodeChar(lua_State*L){
	BOOLEAN res=FALSE;
	GET_INPUT_KEY(L,1,key);
	switch(lua_type(L,2)){
		case LUA_TNUMBER:
			res=key->key.UnicodeChar==lua_tointeger(L,2);
		break;
		case LUA_TSTRING:{
			struct unicode_char_map*m;
			const char*name=lua_tostring(L,2);
			UINTN len=AsciiStrLen(name);
			for(int i=0;(m=&unicode_char_map[i])->name;i++){
				if(key->key.UnicodeChar!=m->code)continue;
				if(AsciiStriCmp(m->name,name)==0)res=TRUE;
			}
			if(!res&&len<=2){
				CHAR16 c=0;
				for(UINTN i=0;i<len;i++)c<<=8,c|=name[i];
				if(key->key.UnicodeChar==c)res=TRUE;
			}
		}break;
		default:return luaL_argerror(L,2,"invalid scan code");
	}
	lua_pushboolean(L,res);
	return 1;
}

void uefi_input_key_to_lua(lua_State*L,EFI_INPUT_KEY*key){
	struct lua_uefi_input_key_data*e;
	if(!key){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_input_key_data));
	luaL_getmetatable(L,LUA_UEFI_INPUT_KEY);
	lua_setmetatable(L,-2);
	CopyMem(&e->key,key,sizeof(EFI_INPUT_KEY));
}

struct lua_uefi_meta_table LuaUefiInputKeyMetaTable={
	.name=LUA_UEFI_INPUT_KEY,
	.reg=(const luaL_Reg[]){
		{"ScanCode",       LuaUefiInputKeyScanCode},
		{"SetScanCode",    LuaUefiInputKeyScanCode},
		{"IsScanCode",     LuaUefiInputKeyIsScanCode},
		{"UnicodeChar",    LuaUefiInputKeyUnicodeChar},
		{"SetUnicodeChar", LuaUefiInputKeyUnicodeChar},
		{"IsUnicodeChar",  LuaUefiInputKeyIsUnicodeChar},
		{NULL, NULL}
	},
	.tostring=LuaUefiInputKeyToString,
	.gc=NULL,
};

#endif
