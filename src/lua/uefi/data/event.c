/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"

STATIC EFIAPI VOID uefi_event_notify(EFI_EVENT ev,VOID*ctx){
	if(!ctx)return;
	struct lua_uefi_event_extra*ed=ctx;
	if(ed->data->event!=ev||!ed||!ed->st)return;
	lua_rawgeti(ed->st,LUA_REGISTRYINDEX,ed->func_ref);
	if(ed->data_ref<=0)lua_pushnil(ed->st);
	else lua_rawgeti(ed->st,LUA_REGISTRYINDEX,ed->data_ref);
	lua_call(ed->st,1,0);
}

EFI_STATUS uefi_event_clean(lua_State*L,struct lua_uefi_event_data*ev){
	EFI_STATUS st=EFI_SUCCESS;
	if(ev->event)st=ev->bs->CloseEvent(ev->event);
	if(ev->data){
		tlog_debug("removed event %p",ev->event);
		if(ev->data->func_ref>0)
			luaL_unref(L,LUA_REGISTRYINDEX,ev->data->func_ref);
		if(ev->data->data_ref>0)
			luaL_unref(L,LUA_REGISTRYINDEX,ev->data->data_ref);
		ZeroMem(ev->data,sizeof(struct lua_uefi_event_extra));
		FreePool(ev->data);
		ev->data=NULL;
	}
	ev->event=NULL;
	ev->bs=NULL;
	return st;
}

static int LuaUefiEventToString(lua_State*L){
	GET_EVENT(L,1,ev);
	lua_pushfstring(L,LUA_UEFI_EVENT" %p",ev->event);
	return 1;
}

static int LuaUefiEventGetData(lua_State*L){
	GET_EVENT(L,1,ev);
	if(!ev->data||ev->data->data_ref<=0)lua_pushnil(L);
	else lua_rawgeti(L,LUA_REGISTRYINDEX,ev->data->data_ref);
	return 1;
}

static int LuaUefiEventGetFunction(lua_State*L){
	GET_EVENT(L,1,ev);
	if(!ev->data||ev->data->func_ref<=0)lua_pushnil(L);
	else lua_rawgeti(L,LUA_REGISTRYINDEX,ev->data->func_ref);
	return 1;
}

static int LuaUefiEventGarbageCollect(lua_State*L){
	GET_EVENT(L,1,ev);
	uefi_event_clean(L,ev);
	return 0;
}

static struct lua_uefi_event_data*get_event(lua_State*L){
	struct lua_uefi_event_data*e;
	e=lua_newuserdata(L,sizeof(struct lua_uefi_char16_data));
	luaL_getmetatable(L,LUA_UEFI_EVENT);
	lua_setmetatable(L,-2);
	return e;
}

struct lua_uefi_event_data*uefi_create_event(
	lua_State*L,
	EFI_BOOT_SERVICES*bs,
	UINT32 type,
	EFI_TPL tpl,
	int data,
	int func
){
	EFI_EVENT event=NULL;
	struct lua_uefi_event_extra*d=NULL;
	if(!(d=AllocateZeroPool(sizeof(struct lua_uefi_event_extra)))){
		uefi_status_to_lua(L,EFI_OUT_OF_RESOURCES);
		lua_pushnil(L);
		return NULL;
	}
	EFI_STATUS st=bs->CreateEvent(
		type,tpl,
		(EFI_EVENT_NOTIFY)uefi_event_notify,
		d,&event
	);
	uefi_status_to_lua(L,st);
	struct lua_uefi_event_data*e=get_event(L);
	e->event=event;
	e->bs=bs;
	e->data=d;
	d->func_ref=func;
	d->data_ref=data;
	return e;
}

struct lua_uefi_event_data*uefi_create_event_ex(
	lua_State*L,
	EFI_BOOT_SERVICES*bs,
	UINT32 type,
	EFI_TPL tpl,
	EFI_GUID*guid,
	int data,
	int func
){
	EFI_EVENT event=NULL;
	struct lua_uefi_event_extra*d=NULL;
	if(!(d=AllocateZeroPool(sizeof(struct lua_uefi_event_extra)))){
		uefi_status_to_lua(L,EFI_OUT_OF_RESOURCES);
		lua_pushnil(L);
		return NULL;
	}
	EFI_STATUS st=bs->CreateEventEx(type,tpl,uefi_event_notify,d,guid,&event);
	uefi_status_to_lua(L,st);
	struct lua_uefi_event_data*e=get_event(L);
	e->event=event;
	e->bs=bs;
	e->data=d;
	d->func_ref=func;
	d->data_ref=data;
	return e;
}

struct lua_uefi_meta_table LuaUefiEventMetaTable={
	.name=LUA_UEFI_EVENT,
	.reg=(const luaL_Reg[]){
		{"ToString",    LuaUefiEventToString},
		{"GetData",     LuaUefiEventGetData},
		{"GetFunction", LuaUefiEventGetFunction},
		{NULL, NULL}
	},
	.tostring=LuaUefiEventToString,
	.gc=LuaUefiEventGarbageCollect,
};

#endif
