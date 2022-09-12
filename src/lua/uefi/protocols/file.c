/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_FILE "UEFI File Protocol"
struct lua_uefi_file_proto{EFI_FILE_PROTOCOL*proto;EFI_FILE_INFO*info;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_file_proto,LUA_UEFI_PROTO_FILE)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static int LuaUefiFileProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_FILE" %p",proto->proto);
	return 1;
}

static int LuaUefiFileProtocolGarbageCollect(lua_State*L){
	EFI_STATUS st;
	GET_PROTO(L,1,proto);
	if(proto->proto){
		st=proto->proto->Close(proto->proto);
		if(!EFI_ERROR(st))proto->proto=NULL;
	}
	if(proto->info){
		FreePool(proto->info);
		proto->info=NULL;
	}
	return 0;
}

static EFI_STATUS get_file_info(struct lua_uefi_file_proto*e,EFI_FILE_INFO**info){
	EFI_STATUS st;
	if(e->info){
		*info=e->info;
		return EFI_SUCCESS;
	}
	st=efi_file_get_file_info(e->proto,NULL,info);
	if(!EFI_ERROR(st)){
		if(!*info)return EFI_OUT_OF_RESOURCES;
		e->info=*info;
	}
	return st;
}

static BOOLEAN str_to_file_attr(const char*str,UINT64*res){
	if(!str||!res)return FALSE;
	if(AsciiStriCmp(str,"read-only")==0)*res=EFI_FILE_READ_ONLY;
	else if(AsciiStriCmp(str,"hidden")==0)*res=EFI_FILE_HIDDEN;
	else if(AsciiStriCmp(str,"system")==0)*res=EFI_FILE_SYSTEM;
	else if(AsciiStriCmp(str,"reserved")==0)*res=EFI_FILE_RESERVED;
	else if(AsciiStriCmp(str,"directory")==0)*res=EFI_FILE_DIRECTORY;
	else if(AsciiStriCmp(str,"archive")==0)*res=EFI_FILE_ARCHIVE;
	else if(AsciiStriCmp(str,"valid-attr")==0)*res=EFI_FILE_VALID_ATTR;
	else return FALSE;
	return TRUE;
}

void uefi_file_protocol_get_mode(lua_State*L,int n,UINT64*mode,BOOLEAN nil){
	switch(lua_type(L,n)){
		case LUA_TNUMBER:
			*mode=lua_tointeger(L,n);
		break;
		case LUA_TSTRING:{
			const char*mod=lua_tostring(L,n);
			if(strchr(mod,'r'))*mode|=EFI_FILE_MODE_READ;
			if(strchr(mod,'w'))*mode|=EFI_FILE_MODE_WRITE;
			if(strchr(mod,'c'))*mode|=EFI_FILE_MODE_CREATE;
			if(strchr(mod,'+'))*mode|=EFI_FILE_MODE_CREATE;
		};break;
		case LUA_TNONE:case LUA_TNIL:if(nil)break;//fallthrough
		default:luaL_argerror(L,n,"invalid file mode");
	}
}

void uefi_file_protocol_get_attr(lua_State*L,int n,UINT64*attr,BOOLEAN nil){
	switch(lua_type(L,n)){
		case LUA_TNUMBER:*attr=luaL_checkinteger(L,n);break;
		case LUA_TSTRING:
			if(!str_to_file_attr(luaL_checkstring(L,n),attr))
				luaL_argerror(L,n,"invalid attribute string");
		break;
		case LUA_TTABLE:
			for(int i=1;i<=luaL_len(L,n);i++){
				UINT64 x=0;
				lua_rawgeti(L,n,i);
				const char*str=luaL_checkstring(L,lua_gettop(L));
				if(!str_to_file_attr(str,&x))
					luaL_argerror(L,n,"invalid attribute string");
				(*attr)|=x;
				lua_pop(L,1);
			}
		break;
		case LUA_TNONE:case LUA_TNIL:if(nil)break;//fallthrough
		default:luaL_argerror(L,n,"invalid attribute");
	}
}

static int LuaUefiFileProtocolOpen(lua_State*L){
	CHAR16*file=NULL;
	UINT64 mode=0,attr=0;
	EFI_FILE_PROTOCOL*fp=NULL;
	GET_PROTO(L,1,proto);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	lua_arg_get_char16(L,2,false,&file);
	if(!file)return luaL_argerror(L,2,"get argument failed");
	uefi_file_protocol_get_mode(L,3,&mode,FALSE);
	uefi_file_protocol_get_attr(L,4,&attr,TRUE);
	EFI_STATUS status=proto->proto->Open(
		proto->proto,&fp,file,mode,attr
	);
	uefi_status_to_lua(L,status);
	uefi_file_protocol_to_lua(L,fp);
	FreePool(file);
	return 2;
}

static int LuaUefiFileProtocolOpenDir(lua_State*L){
	CHAR16*file=NULL;
	EFI_FILE_PROTOCOL*fp=NULL;
	GET_PROTO(L,1,proto);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	lua_arg_get_char16(L,2,false,&file);
	if(!file)return luaL_argerror(L,2,"get argument failed");
	EFI_STATUS status=proto->proto->Open(
		proto->proto,&fp,file,
		EFI_FILE_MODE_READ,
		EFI_FILE_DIRECTORY
	);
	uefi_status_to_lua(L,status);
	uefi_file_protocol_to_lua(L,fp);
	FreePool(file);
	return 2;
}

static int LuaUefiFileProtocolCreate(lua_State*L){
	UINT64 attr=0;
	CHAR16*file=NULL;
	EFI_FILE_PROTOCOL*fp=NULL;
	GET_PROTO(L,1,proto);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	lua_arg_get_char16(L,2,false,&file);
	if(!file)return luaL_argerror(L,2,"get argument failed");
	uefi_file_protocol_get_attr(L,3,&attr,TRUE);
	EFI_STATUS status=proto->proto->Open(
		proto->proto,&fp,file,
		EFI_FILE_MODE_READ|
		EFI_FILE_MODE_WRITE|
		EFI_FILE_MODE_CREATE,
		attr
	);
	uefi_status_to_lua(L,status);
	uefi_file_protocol_to_lua(L,fp);
	FreePool(file);
	return 2;
}

static int LuaUefiFileProtocolCreateDir(lua_State*L){
	CHAR16*file=NULL;
	EFI_FILE_PROTOCOL*fp=NULL;
	GET_PROTO(L,1,proto);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	lua_arg_get_char16(L,2,false,&file);
	if(!file)return luaL_argerror(L,2,"get argument failed");
	EFI_STATUS status=proto->proto->Open(
		proto->proto,&fp,file,
		EFI_FILE_MODE_READ|
		EFI_FILE_MODE_WRITE|
		EFI_FILE_MODE_CREATE,
		EFI_FILE_DIRECTORY
	);
	uefi_status_to_lua(L,status);
	uefi_file_protocol_to_lua(L,fp);
	FreePool(file);
	return 2;
}

static int LuaUefiFileProtocolClose(lua_State*L){
	GET_PROTO(L,1,proto);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	EFI_STATUS status=proto->proto->Close(proto->proto);
	uefi_status_to_lua(L,status);
	if(!EFI_ERROR(status))proto->proto=NULL;
	return 1;
}

static int LuaUefiFileProtocolDelete(lua_State*L){
	GET_PROTO(L,1,proto);
	if(!proto->proto)return luaL_argerror(L,2,"file closed");
	EFI_STATUS status=proto->proto->Delete(proto->proto);
	uefi_status_to_lua(L,status);
	if(!EFI_ERROR(status))proto->proto=NULL;
	return 1;
}

static int LuaUefiFileProtocolFileToData(lua_State*L){
	UINTN size=0;
	VOID*buff=NULL;
	CHAR16*file=NULL;
	EFI_STATUS status;
	EFI_FILE_INFO*info=NULL;
	EFI_FILE_PROTOCOL*fp=NULL;
	GET_PROTO(L,1,proto);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	lua_arg_get_char16(L,2,false,&file);
	if(!file)return luaL_argerror(L,2,"get argument failed");
	status=proto->proto->Open(
		proto->proto,&fp,file,
		EFI_FILE_READ_ONLY,0
	);
	if(EFI_ERROR(status)||!fp)goto done;
	status=efi_file_get_file_info(fp,NULL,&info);
	if(EFI_ERROR(status)||!info)goto done;
	status=efi_file_read(fp,info->FileSize,&buff,&size);
	if(EFI_ERROR(status)||!buff)goto done;
	if(size!=info->FileSize)status=EFI_LOAD_ERROR;
	done:
	if(fp)fp->Close(fp);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)||!buff)lua_pushnil(L);
	else uefi_data_to_lua(L,TRUE,buff,size);
	FreePool(file);
	return 2;
}

static int LuaUefiFileProtocolRead(lua_State*L){
	GET_PROTO(L,1,proto);
	UINTN size=luaL_optinteger(L,2,0);
	EFI_FILE_INFO*info=NULL;
	EFI_STATUS status;
	VOID*buff=NULL;
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	if(size<=0){
		status=get_file_info(proto,&info);
		if(!EFI_ERROR(status))size=info->FileSize;
	}
	if(size<=0)return luaL_argerror(L,2,"read buffer zero");
	else status=efi_file_read(proto->proto,size,&buff,&size);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)||!buff)lua_pushnil(L);
	else uefi_data_to_lua(L,TRUE,buff,size);
	return 2;
}

static int LuaUefiFileProtocolReadDir(lua_State*L){
	GET_PROTO(L,1,proto);
	EFI_STATUS status;
	EFI_FILE_INFO*file=NULL;
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	status=efi_file_read_dir(proto->proto,&file);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)||!file)lua_pushnil(L);
	else uefi_raw_file_info_to_lua(L,&gEfiFileInfoGuid,file);
	if(file)FreePool(file);
	return 2;
}

static int LuaUefiFileProtocolWrite(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	GET_PROTO(L,1,proto);
	lua_arg_get_data(L,2,false,&data,&ds);
	UINTN size=luaL_optinteger(L,3,ds);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	if(!data||size<=0)return luaL_argerror(L,2,"empty data");
	EFI_STATUS status=proto->proto->Write(
		proto->proto,&size,data
	);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status))size=0;
	lua_pushinteger(L,size);
	return 2;
}

static int LuaUefiFileProtocolGetPosition(lua_State*L){
	GET_PROTO(L,1,proto);
	UINT64 pos=0;
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	EFI_STATUS status=proto->proto->GetPosition(proto->proto,&pos);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status))pos=0;
	lua_pushinteger(L,pos);
	return 2;
}

static int LuaUefiFileProtocolSetPosition(lua_State*L){
	GET_PROTO(L,1,proto);
	UINT64 pos=luaL_checkinteger(L,2);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	EFI_STATUS status=proto->proto->SetPosition(proto->proto,pos);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiFileProtocolGetInfo(lua_State*L){
	EFI_GUID guid;
	VOID*data=NULL;
	EFI_STATUS status;
	GET_PROTO(L,1,proto);
	lua_arg_get_guid(L,2,false,&guid);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	status=efi_file_get_info(proto->proto,&guid,NULL,&data);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)||!data)lua_pushnil(L);
	else uefi_raw_file_info_to_lua(L,&guid,data);
	if(data){
		if(CompareGuid(&guid,&gEfiFileInfoGuid)){
			if(proto->info)FreePool(proto->info);
			proto->info=data;
		}
		FreePool(data);
	}
	return 2;
}

static int LuaUefiFileProtocolGetFileInfo(lua_State*L){
	EFI_STATUS status;
	EFI_FILE_INFO*info=NULL;
	GET_PROTO(L,1,proto);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	status=get_file_info(proto,&info);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)||!info)lua_pushnil(L);
	else uefi_raw_file_info_to_lua(L,&gEfiFileInfoGuid,info);
	return 2;
}

static int LuaUefiFileProtocolIsDir(lua_State*L){
	EFI_STATUS status;
	EFI_FILE_INFO*info=NULL;
	GET_PROTO(L,1,proto);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	status=get_file_info(proto,&info);
	if(EFI_ERROR(status)||!info)lua_pushnil(L);
	else lua_pushboolean(L,info->Attribute&EFI_FILE_DIRECTORY);
	return 1;
}

static int LuaUefiFileProtocolIsReadOnly(lua_State*L){
	EFI_STATUS status;
	EFI_FILE_INFO*info=NULL;
	GET_PROTO(L,1,proto);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	status=get_file_info(proto,&info);
	if(EFI_ERROR(status)||!info)lua_pushnil(L);
	else lua_pushboolean(L,info->Attribute&EFI_FILE_READ_ONLY);
	return 1;
}

static int LuaUefiFileProtocolIsSystem(lua_State*L){
	EFI_STATUS status;
	EFI_FILE_INFO*info=NULL;
	GET_PROTO(L,1,proto);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	status=get_file_info(proto,&info);
	if(EFI_ERROR(status)||!info)lua_pushnil(L);
	else lua_pushboolean(L,info->Attribute&EFI_FILE_SYSTEM);
	return 1;
}

static int LuaUefiFileProtocolIsHidden(lua_State*L){
	EFI_STATUS status;
	EFI_FILE_INFO*info=NULL;
	GET_PROTO(L,1,proto);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	status=get_file_info(proto,&info);
	if(EFI_ERROR(status)||!info)lua_pushnil(L);
	else lua_pushboolean(L,info->Attribute&EFI_FILE_HIDDEN);
	return 1;
}

static int LuaUefiFileProtocolIsArchive(lua_State*L){
	EFI_STATUS status;
	EFI_FILE_INFO*info=NULL;
	GET_PROTO(L,1,proto);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	status=get_file_info(proto,&info);
	if(EFI_ERROR(status)||!info)lua_pushnil(L);
	else lua_pushboolean(L,info->Attribute&EFI_FILE_ARCHIVE);
	return 1;
}

static int LuaUefiFileProtocolGetName(lua_State*L){
	EFI_STATUS status;
	EFI_FILE_INFO*info=NULL;
	GET_PROTO(L,1,proto);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	status=get_file_info(proto,&info);
	if(EFI_ERROR(status)||!info)lua_pushnil(L);
	else uefi_char16_a16_to_lua(L,info->FileName);
	return 1;
}

static int LuaUefiFileProtocolGetLength(lua_State*L){
	EFI_STATUS status;
	EFI_FILE_INFO*info=NULL;
	GET_PROTO(L,1,proto);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	status=get_file_info(proto,&info);
	if(EFI_ERROR(status)||!info)lua_pushnil(L);
	else lua_pushinteger(L,info->FileSize);
	return 1;
}

static int LuaUefiFileProtocolSetInfo(lua_State*L){
	EFI_GUID guid;
	UINTN size=0;
	VOID*data=NULL;
	GET_PROTO(L,1,proto);
	lua_arg_get_guid(L,2,false,&guid);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	if(!(data=uefi_raw_file_info_from_lua(L,&guid,&size,3)))
		return luaL_argerror(L,3,"invalid file info");
	EFI_STATUS status=proto->proto->SetInfo(
		proto->proto,&guid,size,data
	);
	if(CompareGuid(&guid,&gEfiFileInfoGuid)){
		if(proto->info)FreePool(proto->info);
		proto->info=data;
	}
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiFileProtocolSetFileInfo(lua_State*L){
	GET_PROTO(L,1,proto);
	UINTN size=0;
	VOID*data=NULL;
	EFI_GUID*guid=&gEfiFileInfoGuid;
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	if(!(data=uefi_raw_file_info_from_lua(L,guid,&size,2)))
		return luaL_argerror(L,3,"invalid file info");
	EFI_STATUS status=proto->proto->SetInfo(
		proto->proto,guid,size,data
	);
	if(proto->info)FreePool(proto->info);
	proto->info=data;
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiFileProtocolSetLength(lua_State*L){
	UINTN size=0;
	EFI_STATUS status;
	EFI_FILE_INFO*info=NULL;
	GET_PROTO(L,1,proto);
	UINTN filesize=luaL_checkinteger(L,2);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	status=get_file_info(proto,&info);
	if(EFI_ERROR(status)||!info)goto fail;
	info->FileSize=filesize;
	status=proto->proto->SetInfo(
		proto->proto,&gEfiFileInfoGuid,size,info
	);
	fail:uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiFileProtocolSetName(lua_State*L){
	VOID*ptr;
	CHAR16*filename=NULL;
	EFI_STATUS status;
	UINTN size=0,ns,fs;
	EFI_FILE_INFO*info=NULL;
	GET_PROTO(L,1,proto);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	lua_arg_get_char16(L,2,false,&filename);
	if(!filename)return luaL_argerror(L,2,"get argument failed");
	status=get_file_info(proto,&info);
	if(EFI_ERROR(status)||!info)goto fail;
	ns=sizeof(EFI_FILE_INFO),fs=StrSize(filename)+ns;
	if(fs>size){
		status=EFI_OUT_OF_RESOURCES;
		if(!(ptr=ReallocatePool(size,fs,info)))goto fail;
		info=ptr,size=fs,proto->info=info;
	}
	fs=size-ns+sizeof(info->FileName);
	StrCpyS(info->FileName,fs,filename);
	status=proto->proto->SetInfo(
		proto->proto,&gEfiFileInfoGuid,size,info
	);
	fail:uefi_status_to_lua(L,status);
	FreePool(filename);
	return 1;
}

static int LuaUefiFileProtocolFlush(lua_State*L){
	GET_PROTO(L,1,proto);
	if(!proto->proto)return luaL_argerror(L,1,"file closed");
	EFI_STATUS status=proto->proto->Flush(proto->proto);
	uefi_status_to_lua(L,status);
	return 1;
}

EFI_FILE_PROTOCOL*uefi_lua_to_file_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_file_protocol_to_lua(lua_State*L,EFI_FILE_PROTOCOL*proto){
	struct lua_uefi_file_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_file_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_FILE);
	lua_setmetatable(L,-2);
	e->proto=proto;
	e->info=NULL;
}

struct lua_uefi_meta_table LuaUefiFileProtocolMetaTable={
	.name=LUA_UEFI_PROTO_FILE,
	.reg=(const luaL_Reg[]){
		{"Open",         LuaUefiFileProtocolOpen},
		{"Close",        LuaUefiFileProtocolClose},
		{"Delete",       LuaUefiFileProtocolDelete},
		{"Read",         LuaUefiFileProtocolRead},
		{"ReadDir",      LuaUefiFileProtocolReadDir},
		{"ReadFolder",   LuaUefiFileProtocolReadDir},
		{"Write",        LuaUefiFileProtocolWrite},
		{"GetPosition",  LuaUefiFileProtocolGetPosition},
		{"SetPosition",  LuaUefiFileProtocolSetPosition},
		{"GetInfo",      LuaUefiFileProtocolGetInfo},
		{"SetInfo",      LuaUefiFileProtocolSetInfo},
		{"Flush",        LuaUefiFileProtocolFlush},

		// extensions functions, non UEFI
		{"OpenDir",      LuaUefiFileProtocolOpenDir},
		{"OpenFolder",   LuaUefiFileProtocolOpenDir},
		{"Create",       LuaUefiFileProtocolCreate},
		{"CreateDir",    LuaUefiFileProtocolCreateDir},
		{"CreateFolder", LuaUefiFileProtocolCreateDir},
		{"GetFileInfo",  LuaUefiFileProtocolGetFileInfo},
		{"SetFileInfo",  LuaUefiFileProtocolSetFileInfo},
		{"IsDir",        LuaUefiFileProtocolIsDir},
		{"IsFolder",     LuaUefiFileProtocolIsDir},
		{"IsSystem",     LuaUefiFileProtocolIsSystem},
		{"IsHidden",     LuaUefiFileProtocolIsHidden},
		{"IsArchive",    LuaUefiFileProtocolIsArchive},
		{"IsReadOnly",   LuaUefiFileProtocolIsReadOnly},
		{"GetName",      LuaUefiFileProtocolGetName},
		{"GetLength",    LuaUefiFileProtocolGetLength},
		{"SetName",      LuaUefiFileProtocolSetName},
		{"ReName",       LuaUefiFileProtocolSetName},
		{"SetLength",    LuaUefiFileProtocolSetLength},
		{"Truncate",     LuaUefiFileProtocolSetLength},
		{"FileToData",   LuaUefiFileProtocolFileToData},
		{NULL, NULL}
	},
	.tostring=LuaUefiFileProtocolToString,
	.gc=LuaUefiFileProtocolGarbageCollect,
};

#endif
