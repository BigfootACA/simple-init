/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"lua_uefi.h"

static void uefi_file_info_to_lua(lua_State*L,EFI_FILE_INFO*fi){
	lua_createtable(L,0,0);
	lua_pushliteral(L,"FileSize");
	lua_pushinteger(L,fi->FileSize);
	lua_settable(L,-3);
	lua_pushliteral(L,"PhysicalSize");
	lua_pushinteger(L,fi->PhysicalSize);
	lua_settable(L,-3);
	lua_pushliteral(L,"CreateTime");
	uefi_time_to_lua(L,&fi->CreateTime);
	lua_settable(L,-3);
	lua_pushliteral(L,"LastAccessTime");
	uefi_time_to_lua(L,&fi->LastAccessTime);
	lua_settable(L,-3);
	lua_pushliteral(L,"ModificationTime");
	uefi_time_to_lua(L,&fi->ModificationTime);
	lua_settable(L,-3);
	lua_pushliteral(L,"Attribute");
	lua_pushinteger(L,fi->Attribute);
	lua_settable(L,-3);
	lua_pushliteral(L,"FileName");
	uefi_char16_a16_to_lua(L,fi->FileName);
	lua_settable(L,-3);
}

static void uefi_file_system_info_to_lua(lua_State*L,EFI_FILE_SYSTEM_INFO*fi){
	lua_createtable(L,0,0);
	lua_pushliteral(L,"ReadOnly");
	lua_pushboolean(L,fi->ReadOnly);
	lua_settable(L,-3);
	lua_pushliteral(L,"VolumeSize");
	lua_pushinteger(L,fi->VolumeSize);
	lua_settable(L,-3);
	lua_pushliteral(L,"FreeSpace");
	lua_pushinteger(L,fi->FreeSpace);
	lua_settable(L,-3);
	lua_pushliteral(L,"BlockSize");
	lua_pushinteger(L,fi->BlockSize);
	lua_settable(L,-3);
	lua_pushliteral(L,"VolumeLabel");
	uefi_char16_a16_to_lua(L,fi->VolumeLabel);
	lua_settable(L,-3);
}

static void uefi_file_system_volume_label_info_to_lua(lua_State*L,EFI_FILE_SYSTEM_VOLUME_LABEL*fi){
	uefi_char16_a16_to_lua(L,fi->VolumeLabel);
}

static EFI_FILE_INFO*uefi_file_info_from_lua(
	lua_State*L,
	UINTN*size,
	int n
){
	CHAR16*fn=NULL;
	luaL_checktype(L,n,LUA_TTABLE);
	lua_getfield(L,n,"FileSize");
	UINT64 fs=luaL_checkinteger(L,-1);
	lua_pop(L,1);
	lua_getfield(L,n,"PhysicalSize");
	UINT64 ps=luaL_checkinteger(L,-1);
	lua_pop(L,1);
	lua_getfield(L,n,"CreateTime");
	GET_TIME(L,-1,ct);
	lua_pop(L,1);
	lua_getfield(L,n,"LastAccessTime");
	GET_TIME(L,-1,at);
	lua_pop(L,1);
	lua_getfield(L,n,"ModificationTime");
	GET_TIME(L,-1,mt);
	lua_pop(L,1);
	lua_getfield(L,n,"Attribute");
	UINT64 attr=luaL_checkinteger(L,-1);
	lua_pop(L,1);
	lua_getfield(L,n,"FileName");
	lua_arg_get_char16(L,-1,false,&fn);
	lua_pop(L,1);
	*size=sizeof(EFI_FILE_INFO)+StrSize(fn);
	EFI_FILE_INFO*fi=AllocateZeroPool(*size);
	if(fi){
		fi->Size=*size;
		fi->FileSize=fs;
		fi->PhysicalSize=ps;
		CopyMem(&fi->CreateTime,&ct->time,sizeof(EFI_TIME));
		CopyMem(&fi->LastAccessTime,&at->time,sizeof(EFI_TIME));
		CopyMem(&fi->ModificationTime,&mt->time,sizeof(EFI_TIME));
		fi->Attribute=attr;
		StrCpyS(fi->FileName,*size,fn);
	}
	FreePool(fn);
	return fi;
}

static EFI_FILE_SYSTEM_INFO*uefi_file_system_info_from_lua(
	lua_State*L,
	UINTN*size,
	int n
){
	CHAR16*vl=NULL;
	luaL_checktype(L,n,LUA_TTABLE);
	lua_getfield(L,n,"ReadOnly");
	luaL_checktype(L,-1,LUA_TBOOLEAN);
	BOOLEAN ro=lua_toboolean(L,-1);
	lua_pop(L,1);
	lua_getfield(L,n,"VolumeSize");
	UINT64 vs=luaL_checkinteger(L,-1);
	lua_pop(L,1);
	lua_getfield(L,n,"FreeSpace");
	UINT64 fs=luaL_checkinteger(L,-1);
	lua_pop(L,1);
	lua_getfield(L,n,"BlockSize");
	UINT32 bs=luaL_checkinteger(L,-1);
	lua_pop(L,1);
	lua_getfield(L,n,"VolumeLabel");
	lua_arg_get_char16(L,-1,false,&vl);
	lua_pop(L,1);
	*size=sizeof(EFI_FILE_SYSTEM_INFO)+StrSize(vl);
	EFI_FILE_SYSTEM_INFO*fi=AllocateZeroPool(*size);
	if(fi){
		fi->ReadOnly=ro;
		fi->VolumeSize=vs;
		fi->FreeSpace=fs;
		fi->BlockSize=bs;
		StrCpyS(fi->VolumeLabel,*size,vl);
	}
	FreePool(vl);
	return fi;
}

static EFI_FILE_SYSTEM_VOLUME_LABEL*uefi_file_system_volume_label_info_from_lua(
	lua_State*L,
	UINTN*size,
	int n
){
	CHAR16*c16=NULL;
	lua_arg_get_char16(L,n,false,&c16);
	if(!c16)luaL_argerror(L,n,"get argument failed");
	*size=sizeof(EFI_FILE_SYSTEM_VOLUME_LABEL)+StrSize(c16);
	EFI_FILE_SYSTEM_VOLUME_LABEL*fi=AllocateZeroPool(*size);
	if(fi)StrCpyS(fi->VolumeLabel,*size,c16);
	FreePool(c16);
	return fi;
}

void*uefi_raw_file_info_from_lua(lua_State*L,EFI_GUID*guid,UINTN*size,int n){
	if(CompareGuid(guid,&gEfiFileSystemVolumeLabelInfoIdGuid))
		return uefi_file_system_volume_label_info_from_lua(L,size,n);
	else if(CompareGuid(guid,&gEfiFileSystemInfoGuid))
		return uefi_file_system_info_from_lua(L,size,n);
	else if(CompareGuid(guid,&gEfiFileInfoGuid))
		return uefi_file_info_from_lua(L,size,n);
	return NULL;
}

void uefi_raw_file_info_to_lua(lua_State*L,EFI_GUID*guid,void*data){
	if(CompareGuid(guid,&gEfiFileSystemVolumeLabelInfoIdGuid))
		uefi_file_system_volume_label_info_to_lua(L,data);
	else if(CompareGuid(guid,&gEfiFileSystemInfoGuid))
		uefi_file_system_info_to_lua(L,data);
	else if(CompareGuid(guid,&gEfiFileInfoGuid))
		uefi_file_info_to_lua(L,data);
	else lua_pushnil(L);
}

#endif
