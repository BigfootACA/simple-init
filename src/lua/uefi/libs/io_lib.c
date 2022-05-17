/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#ifndef NO_IO_LIB
#include<Library/IoLib.h>
#include"../lua_uefi.h"

static int LuaIoLibIoRead8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	lua_pushinteger(L,IoRead8(port));
	return 1;
}

static int LuaIoLibIoWrite8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT8 val=luaL_checkinteger(L,2);
	lua_pushinteger(L,IoWrite8(port,val));
	return 1;
}

static int LuaIoLibIoReadFifo8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN count=luaL_checkinteger(L,2);
	UINTN size=count*sizeof(UINT8);
	VOID*buf=AllocateZeroPool(size);
	if(!buf)return luaL_error(L,"allocate buffer failed");
	IoReadFifo8(port,count,buf);
	uefi_data_to_lua(L,TRUE,buf,size);
	return 1;
}

static int LuaIoLibIoWriteFifo8(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	UINTN port=luaL_checkinteger(L,1);
	lua_arg_get_data(L,2,false,&data,&ds);
	UINTN count=luaL_optinteger(L,3,ds/sizeof(UINT8));
	IoWriteFifo8(port,count,data);
	return 0;
}

static int LuaIoLibIoOr8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT8 or=luaL_checkinteger(L,2);
	lua_pushinteger(L,IoOr8(port,or));
	return 1;
}

static int LuaIoLibIoAnd8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT8 and=luaL_checkinteger(L,2);
	lua_pushinteger(L,IoAnd8(port,and));
	return 1;
}

static int LuaIoLibIoAndThenOr8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT8 and=luaL_checkinteger(L,2);
	UINT8 or=luaL_checkinteger(L,3);
	lua_pushinteger(L,IoAndThenOr8(port,and,or));
	return 1;
}

static int LuaIoLibIoBitFieldRead8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT8 and=luaL_checkinteger(L,2);
	UINT8 or=luaL_checkinteger(L,3);
	lua_pushinteger(L,IoBitFieldRead8(port,and,or));
	return 1;
}

static int LuaIoLibIoBitFieldWrite8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	lua_pushinteger(L,IoBitFieldRead8(port,start,end));
	return 1;
}

static int LuaIoLibIoBitFieldOr8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT8 or=luaL_checkinteger(L,4);
	lua_pushinteger(L,IoBitFieldOr8(port,start,end,or));
	return 1;
}

static int LuaIoLibIoBitFieldAnd8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT8 and=luaL_checkinteger(L,4);
	lua_pushinteger(L,IoBitFieldAnd8(port,start,end,and));
	return 1;
}

static int LuaIoLibIoBitFieldAndThenOr8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT8 and=luaL_checkinteger(L,4);
	UINT8 or=luaL_checkinteger(L,5);
	lua_pushinteger(L,IoBitFieldAndThenOr8(port,start,end,and,or));
	return 1;
}

static int LuaIoLibIoRead16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	lua_pushinteger(L,IoRead16(port));
	return 1;
}

static int LuaIoLibIoWrite16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT16 val=luaL_checkinteger(L,2);
	lua_pushinteger(L,IoWrite16(port,val));
	return 1;
}

static int LuaIoLibIoReadFifo16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN count=luaL_checkinteger(L,2);
	UINTN size=count*sizeof(UINT16);
	VOID*buf=AllocateZeroPool(size);
	if(!buf)return luaL_error(L,"allocate buffer failed");
	IoReadFifo16(port,count,buf);
	uefi_data_to_lua(L,TRUE,buf,size);
	return 1;
}

static int LuaIoLibIoWriteFifo16(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	UINTN port=luaL_checkinteger(L,1);
	lua_arg_get_data(L,2,false,&data,&ds);
	UINTN count=luaL_optinteger(L,3,ds/sizeof(UINT16));
	IoWriteFifo16(port,count,data);
	return 0;
}

static int LuaIoLibIoOr16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT16 or=luaL_checkinteger(L,2);
	lua_pushinteger(L,IoOr16(port,or));
	return 1;
}

static int LuaIoLibIoAnd16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT16 and=luaL_checkinteger(L,2);
	lua_pushinteger(L,IoAnd16(port,and));
	return 1;
}

static int LuaIoLibIoAndThenOr16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT16 and=luaL_checkinteger(L,2);
	UINT16 or=luaL_checkinteger(L,3);
	lua_pushinteger(L,IoAndThenOr16(port,and,or));
	return 1;
}

static int LuaIoLibIoBitFieldRead16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT16 and=luaL_checkinteger(L,2);
	UINT16 or=luaL_checkinteger(L,3);
	lua_pushinteger(L,IoBitFieldRead16(port,and,or));
	return 1;
}

static int LuaIoLibIoBitFieldWrite16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	lua_pushinteger(L,IoBitFieldRead16(port,start,end));
	return 1;
}

static int LuaIoLibIoBitFieldOr16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT16 or=luaL_checkinteger(L,4);
	lua_pushinteger(L,IoBitFieldOr16(port,start,end,or));
	return 1;
}

static int LuaIoLibIoBitFieldAnd16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT16 and=luaL_checkinteger(L,4);
	lua_pushinteger(L,IoBitFieldAnd16(port,start,end,and));
	return 1;
}

static int LuaIoLibIoBitFieldAndThenOr16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT16 and=luaL_checkinteger(L,4);
	UINT16 or=luaL_checkinteger(L,5);
	lua_pushinteger(L,IoBitFieldAndThenOr16(port,start,end,and,or));
	return 1;
}

static int LuaIoLibIoRead32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	lua_pushinteger(L,IoRead32(port));
	return 1;
}

static int LuaIoLibIoWrite32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT32 val=luaL_checkinteger(L,2);
	lua_pushinteger(L,IoWrite32(port,val));
	return 1;
}

static int LuaIoLibIoReadFifo32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN count=luaL_checkinteger(L,2);
	UINTN size=count*sizeof(UINT32);
	VOID*buf=AllocateZeroPool(size);
	if(!buf)return luaL_error(L,"allocate buffer failed");
	IoReadFifo32(port,count,buf);
	uefi_data_to_lua(L,TRUE,buf,size);
	return 1;
}

static int LuaIoLibIoWriteFifo32(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	UINTN port=luaL_checkinteger(L,1);
	lua_arg_get_data(L,2,false,&data,&ds);
	UINTN count=luaL_optinteger(L,3,ds/sizeof(UINT32));
	IoWriteFifo32(port,count,data);
	return 0;
}

static int LuaIoLibIoOr32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT32 or=luaL_checkinteger(L,2);
	lua_pushinteger(L,IoOr32(port,or));
	return 1;
}

static int LuaIoLibIoAnd32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT32 and=luaL_checkinteger(L,2);
	lua_pushinteger(L,IoAnd32(port,and));
	return 1;
}

static int LuaIoLibIoAndThenOr32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT32 and=luaL_checkinteger(L,2);
	UINT32 or=luaL_checkinteger(L,3);
	lua_pushinteger(L,IoAndThenOr32(port,and,or));
	return 1;
}

static int LuaIoLibIoBitFieldRead32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT32 and=luaL_checkinteger(L,2);
	UINT32 or=luaL_checkinteger(L,3);
	lua_pushinteger(L,IoBitFieldRead32(port,and,or));
	return 1;
}

static int LuaIoLibIoBitFieldWrite32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	lua_pushinteger(L,IoBitFieldRead32(port,start,end));
	return 1;
}

static int LuaIoLibIoBitFieldOr32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT32 or=luaL_checkinteger(L,4);
	lua_pushinteger(L,IoBitFieldOr32(port,start,end,or));
	return 1;
}

static int LuaIoLibIoBitFieldAnd32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT32 and=luaL_checkinteger(L,4);
	lua_pushinteger(L,IoBitFieldAnd32(port,start,end,and));
	return 1;
}

static int LuaIoLibIoBitFieldAndThenOr32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT32 and=luaL_checkinteger(L,4);
	UINT32 or=luaL_checkinteger(L,5);
	lua_pushinteger(L,IoBitFieldAndThenOr32(port,start,end,and,or));
	return 1;
}

static int LuaIoLibIoRead64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	lua_pushinteger(L,IoRead64(port));
	return 1;
}

static int LuaIoLibIoWrite64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT64 val=luaL_checkinteger(L,2);
	lua_pushinteger(L,IoWrite64(port,val));
	return 1;
}

static int LuaIoLibIoOr64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT64 or=luaL_checkinteger(L,2);
	lua_pushinteger(L,IoOr64(port,or));
	return 1;
}

static int LuaIoLibIoAnd64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT64 and=luaL_checkinteger(L,2);
	lua_pushinteger(L,IoAnd64(port,and));
	return 1;
}

static int LuaIoLibIoAndThenOr64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT64 and=luaL_checkinteger(L,2);
	UINT64 or=luaL_checkinteger(L,3);
	lua_pushinteger(L,IoAndThenOr64(port,and,or));
	return 1;
}

static int LuaIoLibIoBitFieldRead64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT64 and=luaL_checkinteger(L,2);
	UINT64 or=luaL_checkinteger(L,3);
	lua_pushinteger(L,IoBitFieldRead64(port,and,or));
	return 1;
}

static int LuaIoLibIoBitFieldWrite64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	lua_pushinteger(L,IoBitFieldRead64(port,start,end));
	return 1;
}

static int LuaIoLibIoBitFieldOr64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT64 or=luaL_checkinteger(L,4);
	lua_pushinteger(L,IoBitFieldOr64(port,start,end,or));
	return 1;
}

static int LuaIoLibIoBitFieldAnd64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT64 and=luaL_checkinteger(L,4);
	lua_pushinteger(L,IoBitFieldAnd64(port,start,end,and));
	return 1;
}

static int LuaIoLibIoBitFieldAndThenOr64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT64 and=luaL_checkinteger(L,4);
	UINT64 or=luaL_checkinteger(L,5);
	lua_pushinteger(L,IoBitFieldAndThenOr64(port,start,end,and,or));
	return 1;
}

static int LuaIoLibMmioRead8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	lua_pushinteger(L,MmioRead8(port));
	return 1;
}

static int LuaIoLibMmioWrite8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT8 val=luaL_checkinteger(L,2);
	lua_pushinteger(L,MmioWrite8(port,val));
	return 1;
}

static int LuaIoLibMmioOr8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT8 or=luaL_checkinteger(L,2);
	lua_pushinteger(L,MmioOr8(port,or));
	return 1;
}

static int LuaIoLibMmioAnd8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT8 and=luaL_checkinteger(L,2);
	lua_pushinteger(L,MmioAnd8(port,and));
	return 1;
}

static int LuaIoLibMmioAndThenOr8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT8 and=luaL_checkinteger(L,2);
	UINT8 or=luaL_checkinteger(L,3);
	lua_pushinteger(L,MmioAndThenOr8(port,and,or));
	return 1;
}

static int LuaIoLibMmioBitFieldRead8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT8 and=luaL_checkinteger(L,2);
	UINT8 or=luaL_checkinteger(L,3);
	lua_pushinteger(L,MmioBitFieldRead8(port,and,or));
	return 1;
}

static int LuaIoLibMmioBitFieldWrite8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	lua_pushinteger(L,MmioBitFieldRead8(port,start,end));
	return 1;
}

static int LuaIoLibMmioBitFieldOr8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT8 or=luaL_checkinteger(L,4);
	lua_pushinteger(L,MmioBitFieldOr8(port,start,end,or));
	return 1;
}

static int LuaIoLibMmioBitFieldAnd8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT8 and=luaL_checkinteger(L,4);
	lua_pushinteger(L,MmioBitFieldAnd8(port,start,end,and));
	return 1;
}

static int LuaIoLibMmioBitFieldAndThenOr8(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT8 and=luaL_checkinteger(L,4);
	UINT8 or=luaL_checkinteger(L,5);
	lua_pushinteger(L,MmioBitFieldAndThenOr8(port,start,end,and,or));
	return 1;
}

static int LuaIoLibMmioRead16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	lua_pushinteger(L,MmioRead16(port));
	return 1;
}

static int LuaIoLibMmioWrite16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT16 val=luaL_checkinteger(L,2);
	lua_pushinteger(L,MmioWrite16(port,val));
	return 1;
}

static int LuaIoLibMmioOr16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT16 or=luaL_checkinteger(L,2);
	lua_pushinteger(L,MmioOr16(port,or));
	return 1;
}

static int LuaIoLibMmioAnd16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT16 and=luaL_checkinteger(L,2);
	lua_pushinteger(L,MmioAnd16(port,and));
	return 1;
}

static int LuaIoLibMmioAndThenOr16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT16 and=luaL_checkinteger(L,2);
	UINT16 or=luaL_checkinteger(L,3);
	lua_pushinteger(L,MmioAndThenOr16(port,and,or));
	return 1;
}

static int LuaIoLibMmioBitFieldRead16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT16 and=luaL_checkinteger(L,2);
	UINT16 or=luaL_checkinteger(L,3);
	lua_pushinteger(L,MmioBitFieldRead16(port,and,or));
	return 1;
}

static int LuaIoLibMmioBitFieldWrite16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	lua_pushinteger(L,MmioBitFieldRead16(port,start,end));
	return 1;
}

static int LuaIoLibMmioBitFieldOr16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT16 or=luaL_checkinteger(L,4);
	lua_pushinteger(L,MmioBitFieldOr16(port,start,end,or));
	return 1;
}

static int LuaIoLibMmioBitFieldAnd16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT16 and=luaL_checkinteger(L,4);
	lua_pushinteger(L,MmioBitFieldAnd16(port,start,end,and));
	return 1;
}

static int LuaIoLibMmioBitFieldAndThenOr16(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT16 and=luaL_checkinteger(L,4);
	UINT16 or=luaL_checkinteger(L,5);
	lua_pushinteger(L,MmioBitFieldAndThenOr16(port,start,end,and,or));
	return 1;
}

static int LuaIoLibMmioRead32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	lua_pushinteger(L,MmioRead32(port));
	return 1;
}

static int LuaIoLibMmioWrite32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT32 val=luaL_checkinteger(L,2);
	lua_pushinteger(L,MmioWrite32(port,val));
	return 1;
}

static int LuaIoLibMmioOr32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT32 or=luaL_checkinteger(L,2);
	lua_pushinteger(L,MmioOr32(port,or));
	return 1;
}

static int LuaIoLibMmioAnd32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT32 and=luaL_checkinteger(L,2);
	lua_pushinteger(L,MmioAnd32(port,and));
	return 1;
}

static int LuaIoLibMmioAndThenOr32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT32 and=luaL_checkinteger(L,2);
	UINT32 or=luaL_checkinteger(L,3);
	lua_pushinteger(L,MmioAndThenOr32(port,and,or));
	return 1;
}

static int LuaIoLibMmioBitFieldRead32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT32 and=luaL_checkinteger(L,2);
	UINT32 or=luaL_checkinteger(L,3);
	lua_pushinteger(L,MmioBitFieldRead32(port,and,or));
	return 1;
}

static int LuaIoLibMmioBitFieldWrite32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	lua_pushinteger(L,MmioBitFieldRead32(port,start,end));
	return 1;
}

static int LuaIoLibMmioBitFieldOr32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT32 or=luaL_checkinteger(L,4);
	lua_pushinteger(L,MmioBitFieldOr32(port,start,end,or));
	return 1;
}

static int LuaIoLibMmioBitFieldAnd32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT32 and=luaL_checkinteger(L,4);
	lua_pushinteger(L,MmioBitFieldAnd32(port,start,end,and));
	return 1;
}

static int LuaIoLibMmioBitFieldAndThenOr32(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT32 and=luaL_checkinteger(L,4);
	UINT32 or=luaL_checkinteger(L,5);
	lua_pushinteger(L,MmioBitFieldAndThenOr32(port,start,end,and,or));
	return 1;
}

static int LuaIoLibMmioRead64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	lua_pushinteger(L,MmioRead64(port));
	return 1;
}

static int LuaIoLibMmioWrite64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT64 val=luaL_checkinteger(L,2);
	lua_pushinteger(L,MmioWrite64(port,val));
	return 1;
}

static int LuaIoLibMmioOr64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT64 or=luaL_checkinteger(L,2);
	lua_pushinteger(L,MmioOr64(port,or));
	return 1;
}

static int LuaIoLibMmioAnd64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT64 and=luaL_checkinteger(L,2);
	lua_pushinteger(L,MmioAnd64(port,and));
	return 1;
}

static int LuaIoLibMmioAndThenOr64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT64 and=luaL_checkinteger(L,2);
	UINT64 or=luaL_checkinteger(L,3);
	lua_pushinteger(L,MmioAndThenOr64(port,and,or));
	return 1;
}

static int LuaIoLibMmioBitFieldRead64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINT64 and=luaL_checkinteger(L,2);
	UINT64 or=luaL_checkinteger(L,3);
	lua_pushinteger(L,MmioBitFieldRead64(port,and,or));
	return 1;
}

static int LuaIoLibMmioBitFieldWrite64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	lua_pushinteger(L,MmioBitFieldRead64(port,start,end));
	return 1;
}

static int LuaIoLibMmioBitFieldOr64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT64 or=luaL_checkinteger(L,4);
	lua_pushinteger(L,MmioBitFieldOr64(port,start,end,or));
	return 1;
}

static int LuaIoLibMmioBitFieldAnd64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT64 and=luaL_checkinteger(L,4);
	lua_pushinteger(L,MmioBitFieldAnd64(port,start,end,and));
	return 1;
}

static int LuaIoLibMmioBitFieldAndThenOr64(lua_State*L){
	UINTN port=luaL_checkinteger(L,1);
	UINTN start=luaL_checkinteger(L,2);
	UINTN end=luaL_checkinteger(L,3);
	UINT64 and=luaL_checkinteger(L,4);
	UINT64 or=luaL_checkinteger(L,5);
	lua_pushinteger(L,MmioBitFieldAndThenOr64(port,start,end,and,or));
	return 1;
}

static int LuaIoLibMmioReadBuffer8(lua_State*L){
	UINTN addr=luaL_checkinteger(L,1);
	UINTN len=luaL_checkinteger(L,2);
	VOID*buf=AllocateZeroPool(len);
	if(!buf)return luaL_error(L,"allocate buffer failed");
	MmioReadBuffer8(addr,len,buf);
	uefi_data_to_lua(L,TRUE,buf,len);
	return 1;
}

static int LuaIoLibMmioReadBuffer16(lua_State*L){
	UINTN addr=luaL_checkinteger(L,1);
	UINTN len=luaL_checkinteger(L,2);
	VOID*buf=AllocateZeroPool(len);
	if(!buf)return luaL_error(L,"allocate buffer failed");
	MmioReadBuffer16(addr,len,buf);
	uefi_data_to_lua(L,TRUE,buf,len);
	return 1;
}

static int LuaIoLibMmioReadBuffer32(lua_State*L){
	UINTN addr=luaL_checkinteger(L,1);
	UINTN len=luaL_checkinteger(L,2);
	VOID*buf=AllocateZeroPool(len);
	if(!buf)return luaL_error(L,"allocate buffer failed");
	MmioReadBuffer32(addr,len,buf);
	uefi_data_to_lua(L,TRUE,buf,len);
	return 1;
}

static int LuaIoLibMmioReadBuffer64(lua_State*L){
	UINTN addr=luaL_checkinteger(L,1);
	UINTN len=luaL_checkinteger(L,2);
	VOID*buf=AllocateZeroPool(len);
	if(!buf)return luaL_error(L,"allocate buffer failed");
	MmioReadBuffer64(addr,len,buf);
	uefi_data_to_lua(L,TRUE,buf,len);
	return 1;
}

static int LuaIoLibMmioWriteBuffer8(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	UINTN addr=luaL_checkinteger(L,1);
	lua_arg_get_data(L,2,false,&data,&ds);
	UINTN len=luaL_optinteger(L,3,ds);
	MmioWriteBuffer8(addr,len,data);
	return 0;
}

static int LuaIoLibMmioWriteBuffer16(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	UINTN addr=luaL_checkinteger(L,1);
	lua_arg_get_data(L,2,false,&data,&ds);
	UINTN len=luaL_optinteger(L,3,ds);
	MmioWriteBuffer16(addr,len,data);
	return 0;
}

static int LuaIoLibMmioWriteBuffer32(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	UINTN addr=luaL_checkinteger(L,1);
	lua_arg_get_data(L,2,false,&data,&ds);
	UINTN len=luaL_optinteger(L,3,ds);
	MmioWriteBuffer32(addr,len,data);
	return 0;
}

static int LuaIoLibMmioWriteBuffer64(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	UINTN addr=luaL_checkinteger(L,1);
	lua_arg_get_data(L,2,false,&data,&ds);
	UINTN len=luaL_optinteger(L,3,ds);
	MmioWriteBuffer64(addr,len,data);
	return 0;
}

const luaL_Reg LuaUefiLibraryIoLib[]={
	{"IoRead8",                 LuaIoLibIoRead8},
	{"IoWrite8",                LuaIoLibIoWrite8},
	{"IoReadFifo8",             LuaIoLibIoReadFifo8},
	{"IoWriteFifo8",            LuaIoLibIoWriteFifo8},
	{"IoOr8",                   LuaIoLibIoOr8},
	{"IoAnd8",                  LuaIoLibIoAnd8},
	{"IoAndThenOr8",            LuaIoLibIoAndThenOr8},
	{"IoBitFieldRead8",         LuaIoLibIoBitFieldRead8},
	{"IoBitFieldWrite8",        LuaIoLibIoBitFieldWrite8},
	{"IoBitFieldOr8",           LuaIoLibIoBitFieldOr8},
	{"IoBitFieldAnd8",          LuaIoLibIoBitFieldAnd8},
	{"IoBitFieldAndThenOr8",    LuaIoLibIoBitFieldAndThenOr8},
	{"IoRead16",                LuaIoLibIoRead16},
	{"IoWrite16",               LuaIoLibIoWrite16},
	{"IoReadFifo16",            LuaIoLibIoReadFifo16},
	{"IoWriteFifo16",           LuaIoLibIoWriteFifo16},
	{"IoOr16",                  LuaIoLibIoOr16},
	{"IoAnd16",                 LuaIoLibIoAnd16},
	{"IoAndThenOr16",           LuaIoLibIoAndThenOr16},
	{"IoBitFieldRead16",        LuaIoLibIoBitFieldRead16},
	{"IoBitFieldWrite16",       LuaIoLibIoBitFieldWrite16},
	{"IoBitFieldOr16",          LuaIoLibIoBitFieldOr16},
	{"IoBitFieldAnd16",         LuaIoLibIoBitFieldAnd16},
	{"IoBitFieldAndThenOr16",   LuaIoLibIoBitFieldAndThenOr16},
	{"IoRead32",                LuaIoLibIoRead32},
	{"IoWrite32",               LuaIoLibIoWrite32},
	{"IoReadFifo32",            LuaIoLibIoReadFifo32},
	{"IoWriteFifo32",           LuaIoLibIoWriteFifo32},
	{"IoOr32",                  LuaIoLibIoOr32},
	{"IoAnd32",                 LuaIoLibIoAnd32},
	{"IoAndThenOr32",           LuaIoLibIoAndThenOr32},
	{"IoBitFieldRead32",        LuaIoLibIoBitFieldRead32},
	{"IoBitFieldWrite32",       LuaIoLibIoBitFieldWrite32},
	{"IoBitFieldOr32",          LuaIoLibIoBitFieldOr32},
	{"IoBitFieldAnd32",         LuaIoLibIoBitFieldAnd32},
	{"IoBitFieldAndThenOr32",   LuaIoLibIoBitFieldAndThenOr32},
	{"IoRead64",                LuaIoLibIoRead64},
	{"IoWrite64",               LuaIoLibIoWrite64},
	{"IoOr64",                  LuaIoLibIoOr64},
	{"IoAnd64",                 LuaIoLibIoAnd64},
	{"IoAndThenOr64",           LuaIoLibIoAndThenOr64},
	{"IoBitFieldRead64",        LuaIoLibIoBitFieldRead64},
	{"IoBitFieldWrite64",       LuaIoLibIoBitFieldWrite64},
	{"IoBitFieldOr64",          LuaIoLibIoBitFieldOr64},
	{"IoBitFieldAnd64",         LuaIoLibIoBitFieldAnd64},
	{"IoBitFieldAndThenOr64",   LuaIoLibIoBitFieldAndThenOr64},
	{"MmioRead8",               LuaIoLibMmioRead8},
	{"MmioWrite8",              LuaIoLibMmioWrite8},
	{"MmioOr8",                 LuaIoLibMmioOr8},
	{"MmioAnd8",                LuaIoLibMmioAnd8},
	{"MmioAndThenOr8",          LuaIoLibMmioAndThenOr8},
	{"MmioBitFieldRead8",       LuaIoLibMmioBitFieldRead8},
	{"MmioBitFieldWrite8",      LuaIoLibMmioBitFieldWrite8},
	{"MmioBitFieldOr8",         LuaIoLibMmioBitFieldOr8},
	{"MmioBitFieldAnd8",        LuaIoLibMmioBitFieldAnd8},
	{"MmioBitFieldAndThenOr8",  LuaIoLibMmioBitFieldAndThenOr8},
	{"MmioRead16",              LuaIoLibMmioRead16},
	{"MmioWrite16",             LuaIoLibMmioWrite16},
	{"MmioOr16",                LuaIoLibMmioOr16},
	{"MmioAnd16",               LuaIoLibMmioAnd16},
	{"MmioAndThenOr16",         LuaIoLibMmioAndThenOr16},
	{"MmioBitFieldRead16",      LuaIoLibMmioBitFieldRead16},
	{"MmioBitFieldWrite16",     LuaIoLibMmioBitFieldWrite16},
	{"MmioBitFieldOr16",        LuaIoLibMmioBitFieldOr16},
	{"MmioBitFieldAnd16",       LuaIoLibMmioBitFieldAnd16},
	{"MmioBitFieldAndThenOr16", LuaIoLibMmioBitFieldAndThenOr16},
	{"MmioRead32",              LuaIoLibMmioRead32},
	{"MmioWrite32",             LuaIoLibMmioWrite32},
	{"MmioOr32",                LuaIoLibMmioOr32},
	{"MmioAnd32",               LuaIoLibMmioAnd32},
	{"MmioAndThenOr32",         LuaIoLibMmioAndThenOr32},
	{"MmioBitFieldRead32",      LuaIoLibMmioBitFieldRead32},
	{"MmioBitFieldWrite32",     LuaIoLibMmioBitFieldWrite32},
	{"MmioBitFieldOr32",        LuaIoLibMmioBitFieldOr32},
	{"MmioBitFieldAnd32",       LuaIoLibMmioBitFieldAnd32},
	{"MmioBitFieldAndThenOr32", LuaIoLibMmioBitFieldAndThenOr32},
	{"MmioRead64",              LuaIoLibMmioRead64},
	{"MmioWrite64",             LuaIoLibMmioWrite64},
	{"MmioOr64",                LuaIoLibMmioOr64},
	{"MmioAnd64",               LuaIoLibMmioAnd64},
	{"MmioAndThenOr64",         LuaIoLibMmioAndThenOr64},
	{"MmioBitFieldRead64",      LuaIoLibMmioBitFieldRead64},
	{"MmioBitFieldWrite64",     LuaIoLibMmioBitFieldWrite64},
	{"MmioBitFieldOr64",        LuaIoLibMmioBitFieldOr64},
	{"MmioBitFieldAnd64",       LuaIoLibMmioBitFieldAnd64},
	{"MmioBitFieldAndThenOr64", LuaIoLibMmioBitFieldAndThenOr64},
	{"MmioReadBuffer8",         LuaIoLibMmioReadBuffer8},
	{"MmioReadBuffer16",        LuaIoLibMmioReadBuffer16},
	{"MmioReadBuffer32",        LuaIoLibMmioReadBuffer32},
	{"MmioReadBuffer64",        LuaIoLibMmioReadBuffer64},
	{"MmioWriteBuffer8",        LuaIoLibMmioWriteBuffer8},
	{"MmioWriteBuffer16",       LuaIoLibMmioWriteBuffer16},
	{"MmioWriteBuffer32",       LuaIoLibMmioWriteBuffer32},
	{"MmioWriteBuffer64",       LuaIoLibMmioWriteBuffer64},
	{NULL,NULL}
};
#endif
#endif
