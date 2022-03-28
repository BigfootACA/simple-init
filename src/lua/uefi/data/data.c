/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"

static void clean_parent(struct lua_uefi_data_data*data){
	if(!data->parent)return;
	list_obj_del_data(&data->parent->refs,data,NULL);
	data->parent=NULL;
}

static BOOLEAN convert_allocated(struct lua_uefi_data_data*data){
	if(!data||!data->data||data->size<=0)return FALSE;
	if(data->allocated)return TRUE;
	VOID*ptr=AllocateCopyPool(data->size,data->data);
	if(!ptr)return FALSE;
	data->data=ptr,data->allocated=TRUE;
	clean_parent(data);
	return TRUE;
}

static void clean_refs(struct lua_uefi_data_data*data){
	list*l,*n;
	if((l=list_first(data->refs)))do{
		n=l->next;
		LIST_DATA_DECLARE(r,l,struct lua_uefi_data_data*);
		if(r)convert_allocated(r);
	}while((l=n));
	list_free_all_def(data->refs);
	data->refs=NULL;
}

static void clean_data(struct lua_uefi_data_data*data){
	if(data->allocated)FreePool(data->data);
	clean_parent(data);
	clean_refs(data);
	data->allocated=FALSE;
	data->data=NULL,data->size=0;
}

static struct lua_uefi_data_data*create_data(lua_State*L){
	struct lua_uefi_data_data*e;
	e=lua_newuserdata(L,sizeof(struct lua_uefi_data_data));
	luaL_getmetatable(L,LUA_UEFI_DATA);
	lua_setmetatable(L,-2);
	ZeroMem(e,sizeof(struct lua_uefi_data_data));
	return e;
}

static BOOLEAN resize_data(struct lua_uefi_data_data*data,UINTN size){
	VOID*ptr=NULL;
	if(!data->data||data->size<=0)return FALSE;
	if(size==data->size)return TRUE;
	if(data->allocated){
		clean_refs(data);
		if(!(ptr=ReallocatePool(
			data->size,
			size,
			data->data
		)))return FALSE;
		data->data=ptr;
	}else if(data->parent){
		if(!(ptr=AllocateZeroPool(size)))return FALSE;
		CopyMem(ptr,data->data,MIN(size,data->size));
		data->data=ptr,data->allocated=TRUE;
		clean_parent(data);
	}
	data->size=size;
	return TRUE;
}

static int LuaUefiDataToString(lua_State*L){
	GET_DATA(L,1,data);
	lua_pushfstring(L,LUA_UEFI_DATA" %p (%d bytes)",data->data,data->size);
	return 1;
}

static int LuaUefiDataToRawString(lua_State*L){
	GET_DATA(L,1,data);
	if(!data->data){
		lua_pushnil(L);
		return 1;
	}
	UINTN start=luaL_optinteger(L,2,0);
	if(data->size>0&&start>=data->size)
		return luaL_argerror(L,2,"start out of data range");
	lua_pushstring(L,data->data+start);
	return 1;
}

static int LuaUefiDataToChar16(lua_State*L){
	GET_DATA(L,1,data);
	if(!data->data){
		lua_pushnil(L);
		return 1;
	}
	UINTN start=luaL_optinteger(L,2,0);
	if(data->size>0&&start>=data->size)
		return luaL_argerror(L,2,"start out of data range");
	uefi_char16_16_to_lua(L,FALSE,data->data+start);
	return 1;
}

static int LuaUefiDataToFileInfo(lua_State*L){
	GET_DATA(L,1,data);
	if(!data->data){
		lua_pushnil(L);
		return 1;
	}
	uefi_raw_file_info_to_lua(L,&gEfiFileInfoGuid,data->data);
	return 1;
}

static int LuaUefiDataToProtocol(lua_State*L){
	GET_DATA(L,1,data);
	GET_GUID(L,2,guid);
	uefi_data_to_protocol(L,&guid->guid,&data->data,TRUE);
	return 1;
}

static int LuaUefiDataFromString(lua_State*L){
	GET_DATA(L,1,data);
	const char*str=luaL_checkstring(L,2);
	clean_data(data);
	data->size=AsciiStrSize(str);
	if(!(data->data=AllocateCopyPool(data->size,str)))goto done;
	data->allocated=TRUE;
	done:
	lua_pushboolean(L,data->allocated);
	return 1;
}

static int LuaUefiDataSetString(lua_State*L){
	GET_DATA(L,1,data);
	const char*str=luaL_checkstring(L,2);
	UINTN off=luaL_optinteger(L,3,0);
	UINTN len=luaL_optinteger(L,4,AsciiStrLen(str));
	lua_Integer i=-1;
	if(len==0||!data->data)goto done;
	if(data->size>0&&len>data->size-off)
		if(!resize_data(data,off+len))goto done;
	i=AsciiStrnCpyS(data->data+off,data->size-off,str,len);
	done:
	lua_pushinteger(L,i);
	return 1;
}

static int LuaUefiDataAddress(lua_State*L){
	GET_DATA(L,1,data);
	if(!lua_isnoneornil(L,2)){
		UINTN size=data->size;
		lua_Integer addr=luaL_checkinteger(L,2);
		clean_data(data);
		data->data=(VOID*)addr;
		data->size=size;
	}
	lua_pushinteger(L,(lua_Integer)data->data);
	return 1;
}

static int LuaUefiDataLength(lua_State*L){
	GET_DATA(L,1,data);
	if(!lua_isnoneornil(L,2))
		data->size=luaL_checkinteger(L,2);
	lua_pushinteger(L,data->size);
	return 1;
}

static int LuaUefiDataFill(lua_State*L){
	GET_DATA(L,1,data);
	if(!data->data||data->size<=0){
		lua_pushboolean(L,FALSE);
		return 1;
	}
	UINT8 val=luaL_optinteger(L,2,0);
	UINTN start=luaL_optinteger(L,2,0);
	UINTN size=luaL_optinteger(L,3,data->size-start);
	if(data->size>0){
		if(start>=data->size)
			return luaL_argerror(L,2,"start out of data range");
		if(size>data->size-start)
			return luaL_argerror(L,3,"size out of data range");
	}
	SetMem(data->data+start,MIN(size,data->size),val);
	return 1;
}

static int LuaUefiDataEquals(lua_State*L){
	GET_DATA(L,1,d1);
	GET_DATA(L,2,d2);
	BOOLEAN eq=FALSE;
	if(d1->size!=d2->size)goto done;
	if(d1->data==d2->data)goto equal;
	if(!d1->data||!d2->data)goto done;
	if(CompareMem(
		d1->data,
		d2->data,
		d1->size
	)==0)goto equal;
	done:
	lua_pushboolean(L,eq);
	return 1;
	equal:eq=TRUE;goto done;
}

static int LuaUefiDataCompare(lua_State*L){
	GET_DATA(L,1,d1);
	GET_DATA(L,2,d2);
	lua_Integer i=-1;
	BOOLEAN eq=FALSE;
	UINTN size=MIN(d1->size,d2->size);
	if(d1->data==d2->data)goto equal;
	if(!d1->data||!d2->data)goto done;
	i=CompareMem(d1->data,d2->data,size);
	if(i==0&&d1->size!=d2->size)i=size;
	done:
	lua_pushinteger(L,eq);
	return 1;
	equal:i=0;goto done;
}

static int LuaUefiDataCopy(lua_State*L){
	GET_DATA(L,1,data);
	if(!data->data||data->size<=0){
		lua_pushnil(L);
		return 1;
	}
	UINTN start=luaL_optinteger(L,2,0);
	UINTN size=luaL_optinteger(L,3,data->size-start);
	if(data->size>0){
		if(start>=data->size)
			return luaL_argerror(L,2,"start out of data range");
		if(size>data->size-start)
			return luaL_argerror(L,3,"size out of data range");
	}
	struct lua_uefi_data_data*e=create_data(L);
	e->data=data->data+start;
	e->size=MIN(size,data->size);
	e->parent=data;
	list_obj_add_new(&e->refs,data);
	return 1;
}

static int LuaUefiDataGetChar(lua_State*L){
	GET_DATA(L,1,data);
	if(!data->data){
		lua_pushnil(L);
		return 1;
	}
	UINT8*d=data->data;
	UINTN pos=luaL_optinteger(L,2,0);
	if(data->size>0&&pos>=data->size)
		return luaL_argerror(L,2,"position out of data range");
	lua_pushinteger(L,d[pos]);
	return 1;
}

static int LuaUefiDataSetChar(lua_State*L){
	GET_DATA(L,1,data);
	if(!data->data){
		lua_pushboolean(L,FALSE);
		return 1;
	}
	UINT8 c=luaL_checkinteger(L,2);
	UINT8*d=data->data;
	UINTN pos=luaL_optinteger(L,3,0);
	if(data->size>0&&pos>=data->size)
		return luaL_argerror(L,3,"position out of data range");
	d[pos]=c;
	lua_pushboolean(L,TRUE);
	return 1;
}

static int LuaUefiDataResize(lua_State*L){
	GET_DATA(L,1,data);
	UINTN size=luaL_checkinteger(L,2);
	lua_pushboolean(L,resize_data(data,size));
	return 1;
}

static int LuaUefiDataGarbageCollect(lua_State*L){
	GET_DATA(L,1,data);
	clean_data(data);
	return 0;
}

void uefi_data_to_lua(lua_State*L,BOOLEAN allocated,VOID*data,UINTN size){
	if(!data){
		lua_pushnil(L);
		return;
	}
	struct lua_uefi_data_data*e=create_data(L);
	e->allocated=allocated;
	e->data=data;
	e->size=size;
}

void uefi_data_dup_to_lua(lua_State*L,VOID*data,UINTN size){
	if(!data){
		lua_pushnil(L);
		return;
	}
	struct lua_uefi_data_data*e=create_data(L);
	if(!(e->data=AllocateCopyPool(size,data)))return;
	e->allocated=TRUE;
	e->size=size;
}

struct lua_uefi_meta_table LuaUefiDataMetaTable={
	.name=LUA_UEFI_DATA,
	.reg=(const luaL_Reg[]){
		{"ToString",         LuaUefiDataToRawString},
		{"ToChar16",         LuaUefiDataToChar16},
		{"ToFileInfo",       LuaUefiDataToFileInfo},
		{"ToProtocol",       LuaUefiDataToProtocol},
		{"GetAddress",       LuaUefiDataAddress},
		{"SetAddress",       LuaUefiDataAddress},
		{"GetLength",        LuaUefiDataLength},
		{"SetLength",        LuaUefiDataLength},
		{"SetString",        LuaUefiDataSetString},
		{"FromString",       LuaUefiDataFromString},
		{"ConvertAllocated", LuaUefiDataFromString},
		{"Equals",           LuaUefiDataEquals},
		{"Compare",          LuaUefiDataCompare},
		{"Resize",           LuaUefiDataResize},
		{"GetChar",          LuaUefiDataGetChar},
		{"SetChar",          LuaUefiDataSetChar},
		{"Fill",             LuaUefiDataFill},
		{"Copy",             LuaUefiDataCopy},
		{"Duplicate",        LuaUefiDataCopy},
		{NULL, NULL}
	},
	.tostring=LuaUefiDataToString,
	.gc=LuaUefiDataGarbageCollect,
};

#endif
