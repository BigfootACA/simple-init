/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"xlua.h"
#ifdef ENABLE_UEFI
#include"uefi/lua_uefi.h"
#define free_data(data) (data->uefi?FreePool(data->data):free(data->data))
#define alloc_data(data,size) (void*)(data->uefi?(void*)AllocatePool(size):(void*)malloc(size))
#else
#define free_data(data) free(data->data)
#define alloc_data(data,size) malloc(size)
#endif

#define OPT_DATA(L,n,var) OPT_UDATA(L,n,var,lua_data,LUA_DATA)
#define GET_DATA(L,n,var) OPT_DATA(L,n,var);CHECK_NULL(L,n,var)
static void clean_parent(struct lua_data*data){
	if(!data->parent)return;
	list_obj_del_data(&data->parent->refs,data,NULL);
	data->parent=NULL;
}

static bool convert_allocated(struct lua_data*data){
	if(!data||!data->data||data->size<=0)return false;
	if(data->allocated)return true;
	void*ptr=alloc_data(data,data->size);
	if(!ptr)return false;
	memcpy(ptr,data->data,data->size);
	data->data=ptr,data->allocated=true;
	clean_parent(data);
	return true;
}

static void clean_refs(struct lua_data*data){
	list*l,*n;
	if((l=list_first(data->refs)))do{
		n=l->next;
		LIST_DATA_DECLARE(r,l,struct lua_data*);
		if(r)convert_allocated(r);
	}while((l=n));
	list_free_all_def(data->refs);
	data->refs=NULL;
}

static void clean_data(struct lua_data*data){
	if(data->allocated)free_data(data);
	clean_parent(data);
	clean_refs(data);
	data->allocated=false;
	data->data=NULL,data->size=0;
}

static struct lua_data*create_data(lua_State*L){
	struct lua_data*e;
	e=lua_newuserdata(L,sizeof(struct lua_data));
	luaL_getmetatable(L,LUA_DATA);
	lua_setmetatable(L,-2);
	memset(e,0,sizeof(struct lua_data));
	return e;
}

static bool resize_data(struct lua_data*data,size_t size){
	void*ptr=NULL;
	if(!data->data||data->size<=0)return false;
	if(size==data->size)return true;
	if(data->allocated||data->parent){
		if(data->allocated)clean_refs(data);
		if(!(ptr=alloc_data(data,size)))
			return false;
		if(size>data->size)memset(
			ptr+data->size,
			0,size-data->size
		);
		memcpy(ptr,data->data,MIN(size,data->size));
		if(data->allocated)free_data(data);
		data->data=ptr,data->allocated=true;
		clean_parent(data);
	}
	data->size=size;
	return true;
}

static int lua_data_to_string(lua_State*L){
	GET_DATA(L,1,data);
	lua_pushfstring(
		L,LUA_DATA" %p (%d bytes)",
		data->data,data->size
	);
	return 1;
}

static int lua_data_to_raw_string(lua_State*L){
	GET_DATA(L,1,data);
	if(!data->data){
		lua_pushnil(L);
		return 1;
	}
	size_t start=luaL_optinteger(L,2,0);
	if(data->size>0&&start>=data->size)
		return luaL_argerror(L,2,"start out of data range");
	lua_pushstring(L,data->data+start);
	return 1;
}

#ifdef ENABLE_UEFI
static int lua_data_to_char16(lua_State*L){
	GET_DATA(L,1,data);
	if(!data->data){
		lua_pushnil(L);
		return 1;
	}
	size_t start=luaL_optinteger(L,2,0);
	if(data->size>0&&start>=data->size)
		return luaL_argerror(L,2,"start out of data range");
	uefi_char16_16_to_lua(L,FALSE,data->data+start);
	return 1;
}

static int lua_data_to_file_info(lua_State*L){
	GET_DATA(L,1,data);
	if(!data->data){
		lua_pushnil(L);
		return 1;
	}
	uefi_raw_file_info_to_lua(L,&gEfiFileInfoGuid,data->data);
	return 1;
}

static int lua_data_to_protocol(lua_State*L){
	EFI_GUID guid;
	GET_DATA(L,1,data);
	lua_arg_get_guid(L,2,false,&guid);
	uefi_data_to_protocol(L,&guid,&data->data,true);
	return 1;
}
#endif

static int lua_data_from_string(lua_State*L){
	GET_DATA(L,1,data);
	const char*str=luaL_checkstring(L,2);
	clean_data(data);
	data->size=strlen(str);
	if(!(data->data=alloc_data(data,data->size+1)))goto done;
	memset(data->data,0,data->size+1);
	memcpy(data->data,str,data->size);
	data->allocated=true;
	done:
	lua_pushboolean(L,data->allocated);
	return 1;
}

static int lua_data_set_string(lua_State*L){
	GET_DATA(L,1,data);
	const char*str=luaL_checkstring(L,2);
	size_t off=luaL_optinteger(L,3,0);
	size_t len=luaL_optinteger(L,4,strlen(str));
	lua_Integer i=-1;
	if(len==0||!data->data)goto done;
	if(data->size>0&&len>data->size-off)
		if(!resize_data(data,off+len))goto done;
	strncpy(data->data+off,str,data->size-off);
	i=0;
	done:
	lua_pushinteger(L,i);
	return 1;
}

static int lua_data_address(lua_State*L){
	GET_DATA(L,1,data);
	if(!lua_isnoneornil(L,2)){
		size_t size=data->size;
		lua_Integer addr=luaL_checkinteger(L,2);
		clean_data(data);
		data->data=(void*)(size_t)addr;
		data->size=size;
	}
	lua_pushinteger(L,(lua_Integer)(size_t)data->data);
	return 1;
}

static int lua_data_length(lua_State*L){
	GET_DATA(L,1,data);
	if(!lua_isnoneornil(L,2))
		data->size=luaL_checkinteger(L,2);
	lua_pushinteger(L,data->size);
	return 1;
}

static int lua_data_fill(lua_State*L){
	GET_DATA(L,1,data);
	if(!data->data||data->size<=0){
		lua_pushboolean(L,false);
		return 1;
	}
	int val=luaL_optinteger(L,2,0);
	size_t start=luaL_optinteger(L,2,0);
	size_t size=luaL_optinteger(L,3,data->size-start);
	if(data->size>0){
		if(start>=data->size)
			return luaL_argerror(L,2,"start out of data range");
		if(size>data->size-start)
			return luaL_argerror(L,3,"size out of data range");
	}
	memset(data->data+start,val,MIN(size,data->size));
	return 1;
}

static int lua_data_equals(lua_State*L){
	GET_DATA(L,1,d1);
	GET_DATA(L,2,d2);
	bool eq=false;
	if(d1->size!=d2->size)goto done;
	if(d1->data==d2->data)goto equal;
	if(!d1->data||!d2->data)goto done;
	if(memcpy(
		d1->data,
		d2->data,
		d1->size
	)==0)goto equal;
	done:
	lua_pushboolean(L,eq);
	return 1;
	equal:eq=true;goto done;
}

static int lua_data_compare(lua_State*L){
	GET_DATA(L,1,d1);
	GET_DATA(L,2,d2);
	lua_Integer i=-1;
	bool eq=false;
	size_t size=MIN(d1->size,d2->size);
	if(d1->data==d2->data)goto equal;
	if(!d1->data||!d2->data)goto done;
	i=memcmp(d1->data,d2->data,size);
	if(i==0&&d1->size!=d2->size)i=size;
	done:
	lua_pushinteger(L,eq);
	return 1;
	equal:i=0;goto done;
}

static int lua_data_copy(lua_State*L){
	GET_DATA(L,1,data);
	if(!data->data||data->size<=0){
		lua_pushnil(L);
		return 1;
	}
	size_t start=luaL_optinteger(L,2,0);
	size_t size=luaL_optinteger(L,3,data->size-start);
	if(data->size>0){
		if(start>=data->size)
			return luaL_argerror(L,2,"start out of data range");
		if(size>data->size-start)
			return luaL_argerror(L,3,"size out of data range");
	}
	struct lua_data*e=create_data(L);
	e->data=data->data+start;
	e->size=MIN(size,data->size);
	e->parent=data;
	list_obj_add_new(&e->refs,data);
	return 1;
}

static int lua_data_get_char(lua_State*L){
	GET_DATA(L,1,data);
	if(!data->data){
		lua_pushnil(L);
		return 1;
	}
	uint8_t*d=data->data;
	size_t pos=luaL_optinteger(L,2,0);
	if(data->size>0&&pos>=data->size)
		return luaL_argerror(L,2,"position out of data range");
	lua_pushinteger(L,d[pos]);
	return 1;
}

static int lua_data_set_char(lua_State*L){
	GET_DATA(L,1,data);
	if(!data->data){
		lua_pushboolean(L,false);
		return 1;
	}
	uint8_t c=luaL_checkinteger(L,2);
	uint8_t*d=data->data;
	size_t pos=luaL_optinteger(L,3,0);
	if(data->size>0&&pos>=data->size)
		return luaL_argerror(L,3,"position out of data range");
	d[pos]=c;
	lua_pushboolean(L,true);
	return 1;
}

static int lua_data_resize(lua_State*L){
	GET_DATA(L,1,data);
	size_t size=luaL_checkinteger(L,2);
	lua_pushboolean(L,resize_data(data,size));
	return 1;
}

static int lua_data_gc(lua_State*L){
	GET_DATA(L,1,data);
	clean_data(data);
	return 0;
}

void lua_data_to_lua(lua_State*L,bool allocated,void*data,size_t size){
	if(!data){
		lua_pushnil(L);
		return;
	}
	struct lua_data*e=create_data(L);
	e->allocated=allocated;
	e->data=data;
	e->size=size;
}

void lua_data_dup_to_lua(lua_State*L,void*data,size_t size){
	if(!data){
		lua_pushnil(L);
		return;
	}
	struct lua_data*e=create_data(L);
	if(!(e->data=alloc_data(e,size)))return;
	memcpy(e->data,data,size);
	e->allocated=true;
	e->size=size;
}

#ifdef ENABLE_UEFI
void uefi_data_to_lua(lua_State*L,BOOLEAN allocated,VOID*data,UINTN size){
	if(!data){
		lua_pushnil(L);
		return;
	}
	struct lua_data*e=create_data(L);
	e->allocated=allocated;
	e->data=data;
	e->size=(size_t)size;
	e->uefi=true;
}

void uefi_data_dup_to_lua(lua_State*L,VOID*data,UINTN size){
	if(!data){
		lua_pushnil(L);
		return;
	}
	struct lua_data*e=create_data(L);
	if(!(e->data=AllocateCopyPool(size,data)))return;
	e->allocated=true;
	e->size=(size_t)size;
	e->uefi=true;
}
#endif

void lua_arg_get_data(lua_State*L,int idx,bool nil,void**data,size_t*size){
	switch(lua_type(L,idx)){
		case LUA_TSTRING:{
			const char*a=lua_tostring(L,idx);
			if(data)*data=(void*)a;
			if(size)*size=strlen(a)+1;
		}break;
		case LUA_TUSERDATA:{
			struct lua_data*d1;
			if((d1=luaL_testudata(L,idx,LUA_DATA))){
				if(data)*data=d1->data;
				if(size)*size=d1->size;
				break;
			}
			#ifdef ENABLE_UEFI
			struct lua_uefi_char16_data*d2;
			if((d2=luaL_testudata(L,idx,LUA_UEFI_CHAR16))){
				if(data)*data=d2->string;
				if(size)*size=StrLen(d2->string)+1;
				break;
			}
			#endif
		}break;
		case LUA_TNIL:case LUA_TNONE:
			if(!nil)luaL_argerror(L,idx,"required argument");
		break;
		default:luaL_argerror(L,idx,"argument type unknown");
	}
}

static luaL_Reg data_meta[]={
	{"ToString",          lua_data_to_raw_string},
	{"to_string",         lua_data_to_raw_string},
	{"to_str",            lua_data_to_raw_string},
	#ifdef ENABLE_UEFI
	{"ToChar16",          lua_data_to_char16},
	{"ToFileInfo",        lua_data_to_file_info},
	{"ToProtocol",        lua_data_to_protocol},
	#endif
	{"GetAddress",        lua_data_address},
	{"GetAddr",           lua_data_address},
	{"get_address",       lua_data_address},
	{"get_addr",          lua_data_address},
	{"SetAddress",        lua_data_address},
	{"SetAddr",           lua_data_address},
	{"set_address",       lua_data_address},
	{"set_addr",          lua_data_address},
	{"GetLength",         lua_data_length},
	{"GetLen",            lua_data_length},
	{"get_length",        lua_data_length},
	{"get_len",           lua_data_length},
	{"SetLength",         lua_data_length},
	{"SetLen",            lua_data_length},
	{"set_length",        lua_data_length},
	{"set_len",           lua_data_length},
	{"SetString",         lua_data_set_string},
	{"set_string",        lua_data_set_string},
	{"set_str",           lua_data_set_string},
	{"FromString",        lua_data_from_string},
	{"from_string",       lua_data_from_string},
	{"from_str",          lua_data_from_string},
	{"ConvertAllocated",  lua_data_from_string},
	{"ToAllocated",       lua_data_from_string},
	{"convert_allocated", lua_data_from_string},
	{"to_allocated",      lua_data_from_string},
	{"Equals",            lua_data_equals},
	{"equals",            lua_data_equals},
	{"Compare",           lua_data_compare},
	{"compare",           lua_data_compare},
	{"cmp",               lua_data_compare},
	{"Resize",            lua_data_resize},
	{"resize",            lua_data_resize},
	{"GetChar",           lua_data_get_char},
	{"GetByte",           lua_data_get_char},
	{"get_char",          lua_data_get_char},
	{"get_byte",          lua_data_get_char},
	{"SetChar",           lua_data_set_char},
	{"SetByte",           lua_data_set_char},
	{"set_char",          lua_data_set_char},
	{"set_byte",          lua_data_set_char},
	{"Fill",              lua_data_fill},
	{"fill",              lua_data_fill},
	{"Copy",              lua_data_copy},
	{"copy",              lua_data_copy},
	{"cpy",               lua_data_copy},
	{"Duplicate",         lua_data_copy},
	{"Dup",               lua_data_copy},
	{"duplicate",         lua_data_copy},
	{"dup",               lua_data_copy},
	{NULL, NULL}
};

static int lua_data_new(lua_State*L){
	size_t size=luaL_checkinteger(L,1);
	if(size==0)return luaL_argerror(L,1,"size must not zero");
	void*data=malloc(size);
	if(!data)return luaL_error(L,"alloc %zu bytes failed",size);
	lua_data_to_lua(L,true,data,size);
	return 1;
}

static int lua_data_new_from_string(lua_State*L){
	const char*str=luaL_checkstring(L,1);
	size_t size=strlen(str)+1;
	void*data=malloc(size);
	if(!data)return luaL_error(L,"alloc %zu bytes failed",size);
	strncpy(data,str,size);
	lua_data_to_lua(L,true,data,size);
	return 1;
}

static luaL_Reg data_lib[]={
	{"new",          lua_data_new},
	{"New",          lua_data_new},
	{"Allocate",     lua_data_new},
	{"alloc",        lua_data_new},
	{"FromString",   lua_data_new_from_string},
	{"FromStr",      lua_data_new_from_string},
	{"from_str",     lua_data_new_from_string},
	{"from_string",  lua_data_new_from_string},
	{NULL, NULL}
};

LUAMOD_API int luaopen_data(lua_State*L){
	xlua_create_metatable(
		L,LUA_DATA,
		data_meta,
		lua_data_to_string,
		lua_data_gc
	);
	luaL_newlib(L,data_lib);
	return 1;
}
#endif
