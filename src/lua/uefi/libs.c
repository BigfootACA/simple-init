/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"lua_uefi.h"
#define DECL_LIB(_lib)extern const luaL_Reg LuaUefiLibrary##_lib##Lib[]
#define USE_LIB(_lib){.name=(#_lib),.reg=(LuaUefiLibrary##_lib##Lib)},
DECL_LIB(Io);
DECL_LIB(Uefi);
DECL_LIB(BootLogo);
DECL_LIB(UefiBootManager);
static struct lib_reg{
	const char*name;
	const luaL_Reg*reg;
}regs[]={
	#ifndef NO_IO_LIB
	USE_LIB(Io)
	#endif
	USE_LIB(Uefi)
	USE_LIB(BootLogo)
	USE_LIB(UefiBootManager)
	{NULL,NULL}
};

const luaL_Reg*uefi_get_lib(const char*name){
	char n2[64];
	if(!name)return NULL;
	AsciiSPrint(n2,sizeof(n2),"%aLib",name);
	AsciiStrnCpyS(n2,sizeof(n2),name,AsciiStrLen(name)-3);
	for(int i=0;regs[i].name;i++){
		if(AsciiStriCmp(name,regs[i].name)==0)
			return regs[i].reg;
		if(AsciiStriCmp(n2,regs[i].name)==0)
			return regs[i].reg;
	}
	return NULL;
}

#endif
