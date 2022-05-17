/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"

static int LuaUefiBSRaiseTPL(lua_State*L){
	GET_BS(L,1,bs);
	EFI_TPL tpl=0;
	const char*_tpl=luaL_checkstring(L,2);
	if(!uefi_str_to_tpl(_tpl,&tpl))
		return luaL_argerror(L,2,"invalid tpl");
	EFI_STATUS status=bs->bs->RaiseTPL(tpl);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiBSRestoreTPL(lua_State*L){
	GET_BS(L,1,bs);
	EFI_TPL tpl=0;
	const char*_tpl=luaL_checkstring(L,2);
	if(!uefi_str_to_tpl(_tpl,&tpl))
		return luaL_argerror(L,2,"invalid tpl");
	bs->bs->RestoreTPL(tpl);
	return 0;
}

static int LuaUefiBSGetMemoryMap(lua_State*L){
	UINT32 dv=0;
	EFI_STATUS st;
	UINTN ms=0,mk=0,ds=0,i;
	EFI_MEMORY_DESCRIPTOR*mm=NULL,*md;
	st=gBS->GetMemoryMap(&ms,mm,&mk,&ds,&dv);
	if(st==EFI_BUFFER_TOO_SMALL){
		mm=AllocatePool(ms+(2*ds));
		if(mm)st=gBS->GetMemoryMap(&ms,mm,&mk,&ds,&dv);
	}
	uefi_status_to_lua(L,st);
	if(mm){
		if(!EFI_ERROR(st)){
			lua_createtable(L,0,0);
			for(
				md=mm,i=0;
				(void*)md<(void*)mm+ms;
				md=(void*)md+ds,i++
			){
				lua_createtable(L,0,0);
				lua_pushliteral(L,"Type");
				lua_pushstring(L,uefi_memory_type_to_str(md->Type));
				lua_settable(L,-3);
				lua_pushliteral(L,"PhysicalStart");
				lua_pushinteger(L,md->PhysicalStart);
				lua_settable(L,-3);
				lua_pushliteral(L,"VirtualStart");
				lua_pushinteger(L,md->VirtualStart);
				lua_settable(L,-3);
				lua_pushliteral(L,"NumberOfPages");
				lua_pushinteger(L,md->NumberOfPages);
				lua_settable(L,-3);
				lua_pushliteral(L,"Attribute");
				lua_pushinteger(L,md->Attribute);
				lua_settable(L,-3);
				lua_rawseti(L,-2,i+1);
			}
		}else lua_pushnil(L);
		FreePool(mm);
	}else lua_pushnil(L);
	lua_pushinteger(L,mk);
	lua_pushinteger(L,ds);
	lua_pushinteger(L,dv);
	return 5;
}

static int LuaUefiBSCreateEvent(lua_State*L){
	GET_BS(L,1,bs);
	UINT32 type=0;
	EFI_TPL tpl=0;
	const char*_type=luaL_checkstring(L,2);
	const char*_tpl=luaL_checkstring(L,3);
	if(!uefi_str_to_event_type(_type,&type))
		return luaL_argerror(L,2,"invalid event type");
	if(!uefi_str_to_tpl(_tpl,&tpl))
		return luaL_argerror(L,3,"invalid tpl");
	int data=0,func=0;
	if(!lua_isnoneornil(L,5)){
		data=luaL_ref(L,LUA_REGISTRYINDEX);
	}
	if(!lua_isnoneornil(L,4)){
		luaL_checktype(L,4,LUA_TFUNCTION);
		func=luaL_ref(L,LUA_REGISTRYINDEX);
	}
	uefi_create_event(L,bs->bs,type,tpl,data,func);
	return 2;
}

static int LuaUefiBSSetTimer(lua_State*L){
	GET_BS(L,1,bs);
	GET_EVENT(L,2,event);
	const char*_type=luaL_checkstring(L,3);
	lua_Integer trig_time=luaL_checkinteger(L,4);
	EFI_TIMER_DELAY type=0;
	if(AsciiStriCmp(_type,"relative")==0)type=TimerRelative;
	else if(AsciiStriCmp(_type,"periodic")==0)type=TimerPeriodic;
	else if(AsciiStriCmp(_type,"cancel")==0)type=TimerCancel;
	else return luaL_argerror(L,3,"invalid timer delay type");
	EFI_STATUS status=bs->bs->SetTimer(event->event,type,(UINT64)trig_time);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiBSWaitForEvent(lua_State*L){
	GET_BS(L,1,bs);
	UINTN res=0;
	EFI_EVENT*evs=NULL;
	int cnt=luaL_len(L,2);
	struct lua_uefi_event_data*d;
	if(!(evs=AllocateZeroPool((cnt+1)*sizeof(EFI_EVENT))))
		return luaL_error(L,"allocate buffer failed");
	luaL_checktype(L,2,LUA_TTABLE);
	for(int i=1;i<=cnt;i++){
		lua_rawgeti(L,2,i);
		d=luaL_checkudata(L,lua_gettop(L),LUA_UEFI_EVENT);
		evs[i-1]=d->event;
		lua_pop(L,1);
	}
	EFI_STATUS status=bs->bs->WaitForEvent((UINTN)cnt,evs,&res);
	FreePool(evs);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status))lua_pushnil(L);
	else lua_rawgeti(L,2,(int)res+1);
	return 2;
}

static int LuaUefiBSSignalEvent(lua_State*L){
	GET_BS(L,1,bs);
	GET_EVENT(L,2,event);
	EFI_STATUS status=bs->bs->SignalEvent(event->event);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiBSCloseEvent(lua_State*L){
	GET_BS(L,1,bs);
	GET_EVENT(L,2,event);
	(void)bs;
	EFI_STATUS status=uefi_event_clean(L,event);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiBSCheckEvent(lua_State*L){
	GET_BS(L,1,bs);
	GET_EVENT(L,2,event);
	EFI_STATUS status=bs->bs->CheckEvent(event->event);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiBSHandleProtocol(lua_State*L){
	EFI_GUID guid;
	GET_BS(L,1,bs);
	GET_HANDLE(L,2,hand);
	lua_arg_get_guid(L,3,false,&guid);
	VOID*proto=NULL;
	EFI_STATUS status=bs->bs->HandleProtocol(
		hand->hand,&guid,&proto
	);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)||!proto)lua_pushnil(L);
	else uefi_data_to_protocol(L,&guid,proto,FALSE);
	return 2;
}

static int LuaUefiBSRegisterProtocolNotify(lua_State*L){
	EFI_GUID guid;
	GET_BS(L,1,bs);
	lua_arg_get_guid(L,2,false,&guid);
	GET_EVENT(L,3,event);
	VOID*data=NULL;
	EFI_STATUS status=bs->bs->RegisterProtocolNotify(
		&guid,event->event,&data
	);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status))lua_pushnil(L);
	else uefi_data_to_lua(L,FALSE,data,0);
	return 2;
}

static int LuaUefiBSLocateDevicePath(lua_State*L){
	EFI_GUID guid;
	GET_BS(L,1,bs);
	lua_arg_get_guid(L,2,false,&guid);
	GET_DEVICE_PATH(L,3,dp);
	EFI_DEVICE_PATH_PROTOCOL*xdp=dp->dp;
	EFI_HANDLE hand=NULL;
	EFI_STATUS status=bs->bs->LocateDevicePath(
		&guid,&xdp,&hand
	);
	uefi_status_to_lua(L,status);
	uefi_device_path_to_lua(L,xdp);
	uefi_handle_to_lua(L,hand);
	return 3;
}

static int LuaUefiBSInstallConfigurationTable(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	EFI_GUID guid;
	GET_BS(L,1,bs);
	lua_arg_get_guid(L,2,false,&guid);
	lua_arg_get_data(L,3,false,&data,&ds);
	EFI_STATUS status=bs->bs->InstallConfigurationTable(
		&guid,data
	);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiBSLoadImage(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	GET_BS(L,1,bs);
	luaL_checktype(L,2,LUA_TBOOLEAN);
	BOOLEAN boot_policy=lua_toboolean(L,2);
	GET_HANDLE(L,3,parent);
	GET_DEVICE_PATH(L,4,dp);
	lua_arg_get_data(L,5,false,&data,&ds);
	EFI_HANDLE hand=NULL;
	EFI_STATUS status=bs->bs->LoadImage(
		boot_policy,parent,dp->dp,
		data,data?ds:0,&hand
	);
	uefi_status_to_lua(L,status);
	uefi_handle_to_lua(L,hand);
	return 2;
}

static int LuaUefiBSStartImage(lua_State*L){
	GET_BS(L,1,bs);
	GET_HANDLE(L,2,hand);
	UINTN data_size=0;
	CHAR16*data=NULL;
	EFI_STATUS status=bs->bs->StartImage(
		hand,&data_size,&data
	);
	uefi_status_to_lua(L,status);
	uefi_char16_an16_to_lua(L,data,data_size);
	return 2;
}

static int LuaUefiBSExit(lua_State*L){
	CHAR16*data=NULL;
	GET_BS(L,1,bs);
	GET_HANDLE(L,2,hand);
	GET_STATUS(L,3,exit_st);
	lua_arg_get_char16(L,4,true,&data);
	UINTN data_size=data?StrSize(data):0;
	EFI_STATUS status=bs->bs->Exit(
		hand,exit_st->st,data_size,
		data?data:NULL
	);
	uefi_status_to_lua(L,status);
	if(data)FreePool(data);
	return 1;
}

static int LuaUefiBSUnloadImage(lua_State*L){
	GET_BS(L,1,bs);
	GET_HANDLE(L,2,hand);
	EFI_STATUS status=bs->bs->UnloadImage(hand->hand);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiBSExitBootServices(lua_State*L){
	GET_BS(L,1,bs);
	GET_HANDLE(L,2,hand);
	lua_Integer mapkey=luaL_checkinteger(L,3);
	EFI_STATUS status=bs->bs->ExitBootServices(hand->hand,mapkey);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiBSGetNextMonotonicCount(lua_State*L){
	GET_BS(L,1,bs);
	UINT64 count=0;
	EFI_STATUS status=bs->bs->GetNextMonotonicCount(&count);
	uefi_status_to_lua(L,status);
	lua_pushinteger(L,count);
	return 2;
}

static int LuaUefiBSStall(lua_State*L){
	GET_BS(L,1,bs);
	lua_Integer microseconds=luaL_checkinteger(L,2);
	EFI_STATUS status=bs->bs->Stall((UINTN)microseconds);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiBSSetWatchdogTimer(lua_State*L){
	CHAR16*data=NULL;
	GET_BS(L,1,bs);
	UINTN timeout=(UINTN)luaL_checkinteger(L,2);
	UINT16 wdog_code=(UINTN)luaL_checkinteger(L,3);
	lua_arg_get_char16(L,4,true,&data);
	UINTN data_size=data?StrSize(data):0;
	EFI_STATUS status=bs->bs->SetWatchdogTimer(
		timeout,wdog_code,data_size,
		data?data:NULL
	);
	uefi_status_to_lua(L,status);
	if(data)FreePool(data);
	return 1;
}

static int LuaUefiBSConnectController(lua_State*L){
	GET_BS(L,1,bs);
	GET_HANDLE(L,2,ctrl_hand);
	OPT_HANDLE(L,3,drv_img_hand);
	OPT_DEVICE_PATH(L,4,remaining_dp);
	BOOLEAN recursive=FALSE;
	if(!lua_isnoneornil(L,5)){
		luaL_checktype(L,5,LUA_TBOOLEAN);
		recursive=lua_toboolean(L,5);
	}
	EFI_STATUS status=bs->bs->ConnectController(
		ctrl_hand->hand,
		drv_img_hand?drv_img_hand->hand:NULL,
		remaining_dp?remaining_dp->dp:NULL,
		recursive
	);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiBSDisconnectController(lua_State*L){
	GET_BS(L,1,bs);
	GET_HANDLE(L,2,ctrl_hand);
	OPT_HANDLE(L,3,drv_img_hand);
	OPT_HANDLE(L,4,child_hand);
	EFI_STATUS status=bs->bs->DisconnectController(
		ctrl_hand->hand,
		drv_img_hand?drv_img_hand->hand:NULL,
		child_hand?drv_img_hand->hand:NULL
	);
	uefi_status_to_lua(L,status);
	return 1;
}

static BOOLEAN str_to_open_protocol(const char*str,UINT32*res){
	if(!str||!res)return FALSE;
	if(AsciiStriCmp(str,"by-handle-protocol")==0)*res=EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL;
	else if(AsciiStriCmp(str,"by-child-controller")==0)*res=EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER;
	else if(AsciiStriCmp(str,"by-driver")==0)*res=EFI_OPEN_PROTOCOL_BY_DRIVER;
	else if(AsciiStriCmp(str,"test-protocol")==0)*res=EFI_OPEN_PROTOCOL_TEST_PROTOCOL;
	else if(AsciiStriCmp(str,"exclusive")==0)*res=EFI_OPEN_PROTOCOL_EXCLUSIVE;
	else if(AsciiStriCmp(str,"get-protocol")==0)*res=EFI_OPEN_PROTOCOL_GET_PROTOCOL;
	else return FALSE;
	return TRUE;
}

static int LuaUefiBSOpenProtocol(lua_State*L){
	EFI_GUID guid;
	UINT32 attr=0;
	VOID*proto=NULL;
	GET_BS(L,1,bs);
	GET_HANDLE(L,2,hand);
	lua_arg_get_guid(L,3,false,&guid);
	OPT_HANDLE(L,4,agent);
	OPT_HANDLE(L,5,ctrl);
	switch(lua_type(L,6)){
		case LUA_TNUMBER:attr=luaL_checkinteger(L,6);break;
		case LUA_TSTRING:
			if(!str_to_open_protocol(luaL_checkstring(L,6),&attr))
				return luaL_argerror(L,6,"invalid attribute string");
		break;
		case LUA_TTABLE:
			for(int i=1;i<=luaL_len(L,6);i++){
				UINT32 x=0;
				lua_rawgeti(L,6,i);
				const char*str=luaL_checkstring(L,lua_gettop(L));
				if(!str_to_open_protocol(str,&x))
					return luaL_argerror(L,6,"invalid attribute string");
				attr|=x;
				lua_pop(L,1);
			}
		break;
		case LUA_TNIL:break;
		default:return luaL_argerror(L,6,"invalid attribute");
	}
	EFI_STATUS status=bs->bs->OpenProtocol(
		hand->hand,&guid,&proto,
		agent?agent->hand:NULL,
		ctrl?ctrl->hand:NULL,
		attr
	);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)||!proto)lua_pushnil(L);
	else uefi_data_to_protocol(L,&guid,proto,FALSE);
	return 1;
}

static int LuaUefiBSCloseProtocol(lua_State*L){
	EFI_GUID guid;
	GET_BS(L,1,bs);
	GET_HANDLE(L,2,hand);
	lua_arg_get_guid(L,3,false,&guid);
	GET_HANDLE(L,4,agent_hand);
	GET_HANDLE(L,5,ctrl_hand);
	EFI_STATUS status=bs->bs->CloseProtocol(
		hand->hand,
		&guid,
		agent_hand->hand,
		ctrl_hand->hand
	);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiBSOpenProtocolInformation(lua_State*L){
	//GET_BS(L,1,bs);
	// TODO: implement it
	return luaL_error(L,"not implemented");
}

static int LuaUefiBSProtocolsPerHandle(lua_State*L){
	GET_BS(L,1,bs);
	GET_HANDLE(L,2,hand);
	EFI_GUID**protos=NULL;
	UINTN count=0;
	EFI_STATUS status=bs->bs->ProtocolsPerHandle(hand,&protos,&count);
	uefi_status_to_lua(L,status);
	if(!EFI_ERROR(status)){
		lua_createtable(L,0,0);
		for(UINTN i=0;i<count;i++){
			uefi_guid_to_lua(L,protos[i]);
			lua_rawseti(L,-2,i+1);
		}
		FreePool(protos);
	}else lua_pushnil(L);
	return 2;
}

static int LuaUefiBSLocateHandleBuffer(lua_State*L){
	EFI_GUID guid;
	GET_BS(L,1,bs);
	const char*by=luaL_checkstring(L,2);
	lua_arg_get_guid(L,3,false,&guid);
	UINTN count=0;
	EFI_HANDLE*hands=NULL;
	EFI_LOCATE_SEARCH_TYPE type;
	if(AsciiStriCmp(by,"all")==0)type=AllHandles;
	else if(AsciiStriCmp(by,"protocol")==0)type=ByProtocol;
	else return luaL_argerror(L,2,"invalid search type");
	EFI_STATUS status=bs->bs->LocateHandleBuffer(
		type,&guid,
		NULL,&count,&hands
	);
	uefi_status_to_lua(L,status);
	if(!EFI_ERROR(status)){
		lua_createtable(L,0,0);
		for(UINTN i=0;i<count;i++){
			uefi_handle_to_lua(L,hands[i]);
			lua_rawseti(L,-2,i+1);
		}
	}else lua_pushnil(L);
	return 2;
}

static int LuaUefiBSLocateProtocol(lua_State*L){
	size_t ds=0;
	void*reg=NULL;
	EFI_GUID guid;
	GET_BS(L,1,bs);
	lua_arg_get_guid(L,2,false,&guid);
	lua_arg_get_data(L,3,true,&reg,&ds);
	VOID*proto=NULL;
	EFI_STATUS status=bs->bs->LocateProtocol(
		&guid,reg,&proto
	);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)||!proto)lua_pushnil(L);
	else uefi_data_to_protocol(L,&guid,proto,FALSE);
	return 2;
}

static int LuaUefiBSCalculateCrc32(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	GET_BS(L,1,bs);
	lua_arg_get_data(L,2,false,&data,&ds);
	UINT32 out=0;
	EFI_STATUS status=bs->bs->CalculateCrc32(
		data,ds,&out
	);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiBSCreateEventEx(lua_State*L){
	EFI_GUID guid;
	GET_BS(L,1,bs);
	UINT32 type=0;
	EFI_TPL tpl=0;
	const char*_type=luaL_checkstring(L,2);
	const char*_tpl=luaL_checkstring(L,3);
	bool got=lua_arg_get_guid(L,4,true,&guid);
	if(!uefi_str_to_event_type(_type,&type))
		return luaL_argerror(L,2,"invalid event type");
	if(!uefi_str_to_tpl(_tpl,&tpl))
		return luaL_argerror(L,3,"invalid tpl");
	int data=0,func=0;
	if(!lua_isnoneornil(L,6)){
		data=luaL_ref(L,LUA_REGISTRYINDEX);
	}
	if(!lua_isnoneornil(L,5)){
		luaL_checktype(L,5,LUA_TFUNCTION);
		func=luaL_ref(L,LUA_REGISTRYINDEX);
	}
	uefi_create_event_ex(
		L,bs->bs,type,tpl,
		got?&guid:NULL,
		data,func
	);
	return 2;
}

static int LuaUefiBSToString(lua_State*L){
	GET_BS(L,1,bs);
	lua_pushfstring(L,LUA_UEFI_BS" %p",bs->bs);
	return 1;
}

void uefi_bs_to_lua(lua_State*L,EFI_BOOT_SERVICES*bs){
	struct lua_uefi_bs_data*e;
	if(!bs){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_bs_data));
	luaL_getmetatable(L,LUA_UEFI_BS);
	lua_setmetatable(L,-2);
	e->bs=bs;
}

struct lua_uefi_meta_table LuaUefiBSMetaTable={
	.name=LUA_UEFI_BS,
	.reg=(const luaL_Reg[]){
		{"RaiseTPL",                  LuaUefiBSRaiseTPL},
		{"RestoreTPL",                LuaUefiBSRestoreTPL},
		{"GetMemoryMap",              LuaUefiBSGetMemoryMap},
		{"CreateEvent",               LuaUefiBSCreateEvent},
		{"SetTimer",                  LuaUefiBSSetTimer},
		{"WaitForEvent",              LuaUefiBSWaitForEvent},
		{"SignalEvent",               LuaUefiBSSignalEvent},
		{"CloseEvent",                LuaUefiBSCloseEvent},
		{"CheckEvent",                LuaUefiBSCheckEvent},
		{"HandleProtocol",            LuaUefiBSHandleProtocol},
		{"RegisterProtocolNotify",    LuaUefiBSRegisterProtocolNotify},
		{"LocateDevicePath",          LuaUefiBSLocateDevicePath},
		{"InstallConfigurationTable", LuaUefiBSInstallConfigurationTable},
		{"LoadImage",                 LuaUefiBSLoadImage},
		{"StartImage",                LuaUefiBSStartImage},
		{"Exit",                      LuaUefiBSExit},
		{"UnloadImage",               LuaUefiBSUnloadImage},
		{"ExitBootServices",          LuaUefiBSExitBootServices},
		{"GetNextMonotonicCount",     LuaUefiBSGetNextMonotonicCount},
		{"Stall",                     LuaUefiBSStall},
		{"SetWatchdogTimer",          LuaUefiBSSetWatchdogTimer},
		{"ConnectController",         LuaUefiBSConnectController},
		{"DisconnectController",      LuaUefiBSDisconnectController},
		{"OpenProtocol",              LuaUefiBSOpenProtocol},
		{"CloseProtocol",             LuaUefiBSCloseProtocol},
		{"OpenProtocolInformation",   LuaUefiBSOpenProtocolInformation},
		{"ProtocolsPerHandle",        LuaUefiBSProtocolsPerHandle},
		{"LocateHandleBuffer",        LuaUefiBSLocateHandleBuffer},
		{"LocateProtocol",            LuaUefiBSLocateProtocol},
		{"CalculateCrc32",            LuaUefiBSCalculateCrc32},
		{"CreateEventEx",             LuaUefiBSCreateEventEx},
		{NULL, NULL}
	},
	.tostring=LuaUefiBSToString,
	.gc=NULL,
};

#endif
