/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_SERIAL_IO "UEFI Serial IO Protocol"
struct lua_uefi_serial_io_proto{EFI_SERIAL_IO_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_serial_io_proto,LUA_UEFI_PROTO_SERIAL_IO)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static BOOLEAN str_to_control_bits(const char*str,UINT32*res){
	if(!str||!res)return FALSE;
	if(AsciiStriCmp(str,"clear-to-send")==0)*res=EFI_SERIAL_CLEAR_TO_SEND;
	else if(AsciiStriCmp(str,"data-set-ready")==0)*res=EFI_SERIAL_DATA_SET_READY;
	else if(AsciiStriCmp(str,"ring-indicate")==0)*res=EFI_SERIAL_RING_INDICATE;
	else if(AsciiStriCmp(str,"carrier-detect")==0)*res=EFI_SERIAL_CARRIER_DETECT;
	else if(AsciiStriCmp(str,"input-buffer-empty")==0)*res=EFI_SERIAL_INPUT_BUFFER_EMPTY;
	else if(AsciiStriCmp(str,"output-buffer-empty")==0)*res=EFI_SERIAL_OUTPUT_BUFFER_EMPTY;
	else if(AsciiStriCmp(str,"request-to-send")==0)*res=EFI_SERIAL_REQUEST_TO_SEND;
	else if(AsciiStriCmp(str,"data-terminal-ready")==0)*res=EFI_SERIAL_DATA_TERMINAL_READY;
	else if(AsciiStriCmp(str,"hardware-loopback-enable")==0)*res=EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE;
	else if(AsciiStriCmp(str,"software-loopback-enable")==0)*res=EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE;
	else if(AsciiStriCmp(str,"hardware-flow-control-enable")==0)*res=EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE;
	else return FALSE;
	return TRUE;
}

static BOOLEAN str_to_parity(const char*str,EFI_PARITY_TYPE*type){
	if(!str||!type)return FALSE;
	if(AsciiStriCmp(str,"default")==0)*type=DefaultParity;
	else if(AsciiStriCmp(str,"no")==0)*type=NoParity;
	else if(AsciiStriCmp(str,"even")==0)*type=EvenParity;
	else if(AsciiStriCmp(str,"odd")==0)*type=OddParity;
	else if(AsciiStriCmp(str,"mark")==0)*type=MarkParity;
	else if(AsciiStriCmp(str,"space")==0)*type=SpaceParity;
	else return FALSE;
	return TRUE;
}

static BOOLEAN str_to_stop(const char*str,EFI_STOP_BITS_TYPE*type){
	if(!str||!type)return FALSE;
	if(AsciiStriCmp(str,"default")==0)*type=DefaultStopBits;
	else if(AsciiStriCmp(str,"one")==0)*type=OneStopBit;
	else if(AsciiStriCmp(str,"onefive")==0)*type=OneFiveStopBits;
	else if(AsciiStriCmp(str,"two")==0)*type=TwoStopBits;
	else return FALSE;
	return TRUE;
}

static const char*parity_to_str(EFI_PARITY_TYPE type){
	switch(type){
		case DefaultParity:return "default";
		case NoParity:return "no";
		case EvenParity:return "even";
		case OddParity:return "odd";
		case MarkParity:return "mark";
		case SpaceParity:return "space";
		default:return NULL;
	}
}

static const char*stop_to_str(EFI_STOP_BITS_TYPE type){
	switch(type){
		case DefaultStopBits:return "default";
		case OneStopBit:return "one";
		case OneFiveStopBits:return "onefive";
		case TwoStopBits:return "two";
		default:return NULL;
	}
}

static void load_mode(lua_State*L,EFI_SERIAL_IO_MODE*mode){
	lua_createtable(L,0,0);
	lua_pushliteral(L,"ControlMask");
	lua_pushinteger(L,mode->ControlMask);
	lua_settable(L,-3);
	lua_pushliteral(L,"Timeout");
	lua_pushinteger(L,mode->Timeout);
	lua_settable(L,-3);
	lua_pushliteral(L,"BaudRate");
	lua_pushinteger(L,mode->BaudRate);
	lua_settable(L,-3);
	lua_pushliteral(L,"ReceiveFifoDepth");
	lua_pushinteger(L,mode->ReceiveFifoDepth);
	lua_settable(L,-3);
	lua_pushliteral(L,"DataBits");
	lua_pushinteger(L,mode->DataBits);
	lua_settable(L,-3);
	lua_pushliteral(L,"Parity");
	lua_pushstring(L,parity_to_str(mode->Parity));
	lua_settable(L,-3);
	lua_pushliteral(L,"StopBits");
	lua_pushstring(L,stop_to_str(mode->StopBits));
	lua_settable(L,-3);
	lua_pushliteral(L,"DataBits");
	lua_pushinteger(L,mode->DataBits);
	lua_settable(L,-3);
}

static int LuaUefiSerialIOProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_SERIAL_IO" %p",proto->proto);
	return 1;
}

static int LuaUefiSerialIOProtocolReset(lua_State*L){
	GET_PROTO(L,1,proto);
	EFI_STATUS status=proto->proto->Reset(proto->proto);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiSerialIOProtocolSetAttributes(lua_State*L){
	GET_PROTO(L,1,proto);
	EFI_PARITY_TYPE parity=0;
	EFI_STOP_BITS_TYPE stop_bits=0;
	UINT64 baud_rate=luaL_checkinteger(L,2);
	UINT32 fifo_dept=luaL_checkinteger(L,3);
	UINT32 timeout=luaL_checkinteger(L,4);
	if(!str_to_parity(luaL_checkstring(L,5),&parity))
		return luaL_argerror(L,5,"invalid parity");
	UINT8 data_bits=luaL_checkinteger(L,6);
	if(!str_to_stop(luaL_checkstring(L,7),&stop_bits))
		return luaL_argerror(L,5,"invalid stop bits");
	EFI_STATUS status=proto->proto->SetAttributes(
		proto->proto,baud_rate,fifo_dept,
		timeout,parity,data_bits,stop_bits
	);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiSerialIOProtocolSetControl(lua_State*L){
	GET_PROTO(L,1,proto);
	UINT32 control=0;
	switch(lua_type(L,2)){
		case LUA_TNUMBER:control=luaL_checkinteger(L,2);break;
		case LUA_TSTRING:
			if(!str_to_control_bits(luaL_checkstring(L,2),&control))
				return luaL_argerror(L,2,"invalid control bits string");
		break;
		case LUA_TTABLE:
			for(int i=1;i<=luaL_len(L,2);i++){
				UINT32 x=0;
				lua_rawgeti(L,2,i);
				const char*str=luaL_checkstring(L,lua_gettop(L));
				if(!str_to_control_bits(str,&x))
					return luaL_argerror(L,6,"invalid control bits string");
				control|=x;
				lua_pop(L,1);
			}
		break;
		default:return luaL_argerror(L,6,"invalid control bits");
	}
	EFI_STATUS status=proto->proto->SetControl(
		proto->proto,control
	);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiSerialIOProtocolGetControl(lua_State*L){
	BOOLEAN str=FALSE;
	GET_PROTO(L,1,proto);
	if(!lua_isnoneornil(L,2))
		str=lua_toboolean(L,2);
	UINT32 control=0;
	EFI_STATUS status=proto->proto->GetControl(
		proto->proto,&control
	);
	uefi_status_to_lua(L,status);
	if(str){
		size_t i=0;
		lua_createtable(L,0,0);
		if(control&EFI_SERIAL_CLEAR_TO_SEND)
			nadd_str_arr(L,i,"clear-to-send");
		if(control&EFI_SERIAL_DATA_SET_READY)
			nadd_str_arr(L,i,"data-set-ready");
		if(control&EFI_SERIAL_RING_INDICATE)
			nadd_str_arr(L,i,"ring-indicate");
		if(control&EFI_SERIAL_CARRIER_DETECT)
			nadd_str_arr(L,i,"carrier-detect");
		if(control&EFI_SERIAL_INPUT_BUFFER_EMPTY)
			nadd_str_arr(L,i,"input-buffer-empty");
		if(control&EFI_SERIAL_OUTPUT_BUFFER_EMPTY)
			nadd_str_arr(L,i,"output-buffer-empty");
		if(control&EFI_SERIAL_REQUEST_TO_SEND)
			nadd_str_arr(L,i,"request-to-send");
		if(control&EFI_SERIAL_DATA_TERMINAL_READY)
			nadd_str_arr(L,i,"data-terminal-ready");
		if(control&EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE)
			nadd_str_arr(L,i,"hardware-loopback-enable");
		if(control&EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE)
			nadd_str_arr(L,i,"software-loopback-enable");
		if(control&EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE)
			nadd_str_arr(L,i,"hardware-flow-control-enable");
	}else lua_pushinteger(L,control);
	return 2;
}

static int LuaUefiSerialIOProtocolRead(lua_State*L){
	GET_PROTO(L,1,proto);
	VOID*buffer=NULL;
	UINTN size=luaL_checkinteger(L,2);
	if(!(buffer=AllocateZeroPool(size))){
		uefi_status_to_lua(L,EFI_OUT_OF_RESOURCES);
		lua_pushnil(L);
		return 2;
	}
	EFI_STATUS status=proto->proto->Read(
		proto->proto,&size,buffer
	);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)){
		lua_pushnil(L);
		FreePool(buffer);
	}else uefi_data_to_lua(L,TRUE,buffer,size);
	return 2;
}

static int LuaUefiSerialIOProtocolWrite(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	GET_PROTO(L,1,proto);
	lua_arg_get_data(L,2,true,&data,&ds);
	UINTN size=luaL_optinteger(L,3,ds);
	if(!data)return luaL_argerror(L,2,"empty data");
	if(size<=0)return luaL_argerror(L,3,"empty data");
	EFI_STATUS status=proto->proto->Write(
		proto->proto,&size,data
	);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status))size=0;
	lua_pushinteger(L,size);
	return 2;
}

static int LuaUefiSerialIOProtocolMode(lua_State*L){
	GET_PROTO(L,1,proto);
	if(!proto->proto->Mode)lua_pushnil(L);
	else load_mode(L,proto->proto->Mode);
	return 1;
}

static int LuaUefiSerialIOProtocolRevision(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushinteger(L,proto->proto->Revision);
	return 1;
}

#ifdef EFI_SERIAL_IO_PROTOCOL_REVISION1p1
static int LuaUefiSerialIOProtocolDeviceTypeGuid(lua_State*L){
	GET_PROTO(L,1,proto);
	if(proto->proto->Revision<EFI_SERIAL_IO_PROTOCOL_REVISION1p1)
		lua_pushnil(L);
	else uefi_guid_to_lua(L,proto->proto->DeviceTypeGuid);
	return 1;
}
#endif

EFI_SERIAL_IO_PROTOCOL*uefi_lua_to_serial_io_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_serial_io_protocol_to_lua(lua_State*L,EFI_SERIAL_IO_PROTOCOL*proto){
	struct lua_uefi_serial_io_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_serial_io_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_SERIAL_IO);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiSerialIOProtocolMetaTable={
	.name=LUA_UEFI_PROTO_SERIAL_IO,
	.reg=(const luaL_Reg[]){
		{"Revision",       LuaUefiSerialIOProtocolRevision},
		{"Reset",          LuaUefiSerialIOProtocolReset},
		{"SetAttributes",  LuaUefiSerialIOProtocolSetAttributes},
		{"SetControl",     LuaUefiSerialIOProtocolSetControl},
		{"GetControl",     LuaUefiSerialIOProtocolGetControl},
		{"Write",          LuaUefiSerialIOProtocolWrite},
		{"Read",           LuaUefiSerialIOProtocolRead},
		{"Mode",           LuaUefiSerialIOProtocolMode},
		#ifdef EFI_SERIAL_IO_PROTOCOL_REVISION1p1
		{"DeviceTypeGuid", LuaUefiSerialIOProtocolDeviceTypeGuid},
		#endif
		{NULL, NULL}
	},
	.tostring=LuaUefiSerialIOProtocolToString,
	.gc=NULL,
};

#endif
