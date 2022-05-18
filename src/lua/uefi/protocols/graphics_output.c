/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"
#define LUA_UEFI_PROTO_GRAPHICS_OUTPUT "UEFI Graphics Output Protocol"
struct lua_uefi_graphics_output_proto{EFI_GRAPHICS_OUTPUT_PROTOCOL*proto;};
#define OPT_PROTO(L,n,var) OPT_UDATA(L,n,var,lua_uefi_graphics_output_proto,LUA_UEFI_PROTO_GRAPHICS_OUTPUT)
#define GET_PROTO(L,n,var) OPT_PROTO(L,n,var);CHECK_NULL(L,n,var)

static void load_pixel_bitmask(lua_State*L,EFI_PIXEL_BITMASK*mask){
	lua_createtable(L,0,0);
	lua_pushliteral(L,"RedMask");
	lua_pushinteger(L,mask->RedMask);
	lua_settable(L,-3);
	lua_pushliteral(L,"GreenMask");
	lua_pushinteger(L,mask->GreenMask);
	lua_settable(L,-3);
	lua_pushliteral(L,"BlueMask");
	lua_pushinteger(L,mask->BlueMask);
	lua_settable(L,-3);
	lua_pushliteral(L,"ReservedMask");
	lua_pushinteger(L,mask->ReservedMask);
	lua_settable(L,-3);
}

static void load_info(lua_State*L,EFI_GRAPHICS_OUTPUT_MODE_INFORMATION*info){
	lua_createtable(L,0,0);
	lua_pushliteral(L,"Version");
	lua_pushinteger(L,info->Version);
	lua_settable(L,-3);
	lua_pushliteral(L,"HorizontalResolution");
	lua_pushinteger(L,info->HorizontalResolution);
	lua_settable(L,-3);
	lua_pushliteral(L,"VerticalResolution");
	lua_pushinteger(L,info->VerticalResolution);
	lua_settable(L,-3);
	lua_pushliteral(L,"PixelFormat");
	switch(info->PixelFormat){
		case PixelRedGreenBlueReserved8BitPerColor:lua_pushstring(L,"argb");break;
		case PixelBlueGreenRedReserved8BitPerColor:lua_pushstring(L,"abgr");break;
		case PixelBitMask:lua_pushstring(L,"mask");break;
		case PixelBltOnly:lua_pushstring(L,"bltonly");break;
		default:lua_pushnil(L);break;
	}
	lua_settable(L,-3);
	lua_pushliteral(L,"PixelInformation");
	load_pixel_bitmask(L,&info->PixelInformation);
	lua_settable(L,-3);
	lua_pushliteral(L,"PixelsPerScanLine");
	lua_pushinteger(L,info->PixelsPerScanLine);
	lua_settable(L,-3);
}

static void load_mode(lua_State*L,EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE*mode){
	lua_createtable(L,0,0);
	lua_pushliteral(L,"MaxMode");
	lua_pushinteger(L,mode->MaxMode);
	lua_settable(L,-3);
	lua_pushliteral(L,"Mode");
	lua_pushinteger(L,mode->Mode);
	lua_settable(L,-3);
	lua_pushliteral(L,"Info");
	load_info(L,mode->Info);
	lua_settable(L,-3);
	lua_pushliteral(L,"FrameBufferBase");
	lua_pushinteger(L,mode->FrameBufferBase);
	lua_settable(L,-3);
	lua_pushliteral(L,"FrameBufferSize");
	lua_pushinteger(L,mode->FrameBufferSize);
	lua_settable(L,-3);
}

void uefi_graphics_output_get_pixel(lua_State*L,int t,EFI_GRAPHICS_OUTPUT_BLT_PIXEL*pixel){
	switch(lua_type(L,t)){
		case LUA_TTABLE:{
			lua_getfield(L,t,"Red");
			pixel->Red=luaL_checkinteger(L,-1);
			lua_pop(L,1);
			lua_getfield(L,t,"Green");
			pixel->Green=luaL_checkinteger(L,-1);
			lua_pop(L,1);
			lua_getfield(L,t,"Blue");
			pixel->Blue=luaL_checkinteger(L,-1);
			lua_pop(L,1);
			pixel->Reserved=0;
		}break;
		case LUA_TUSERDATA:{
			size_t ds=0;
			void*data=NULL;
			lua_arg_get_data(L,t,false,&data,&ds);
			if(data&&(ds==0||ds>sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL))){
				CopyMem(
					pixel,data,
					sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
				);
				break;
			}
		}//fallthrough
		default:luaL_argerror(L,t,"invalid pixel data");
	}
}

void uefi_graphics_output_get_pixels(lua_State*L,int t,EFI_GRAPHICS_OUTPUT_BLT_PIXEL**pixels,BOOLEAN*alloc){
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL*buff=NULL;
	*alloc=FALSE,*pixels=NULL;
	switch(lua_type(L,t)){
		case LUA_TUSERDATA:{
			size_t ds=0;
			void*data=NULL;
			lua_arg_get_data(L,t,false,&data,&ds);
			buff=data;
		}break;
		case LUA_TTABLE:{
			lua_Integer cnt=luaL_len(L,t);
			UINTN size=cnt*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
			if(!(buff=AllocateZeroPool(size))){
				luaL_error(L,"allocate pool failed");
				return;
			}
			*alloc=TRUE;
			for(size_t i=0;i<(size_t)cnt;i++){
				lua_rawgeti(L,t,i+1);
				uefi_graphics_output_get_pixel(
					L,lua_gettop(L),&buff[i]
				);
				lua_pop(L,1);
			}
		};break;
		case LUA_TNONE:case LUA_TNIL:break;
		default:luaL_argerror(L,t,"invalid pixels data");
	}
	*pixels=buff;
}

static BOOLEAN str_to_blt_oper(const char*str,EFI_GRAPHICS_OUTPUT_BLT_OPERATION*oper){
	if(!str||!oper)return FALSE;
	if(AsciiStriCmp(str,"video-fill")==0)*oper=EfiBltVideoFill;
	else if(AsciiStriCmp(str,"video-to-blt-buffer")==0)*oper=EfiBltVideoToBltBuffer;
	else if(AsciiStriCmp(str,"buffer-to-video")==0)*oper=EfiBltBufferToVideo;
	else if(AsciiStriCmp(str,"video-to-video")==0)*oper=EfiBltVideoToVideo;
	else return FALSE;
	return TRUE;
}

static int LuaUefiGraphicsOutputProtocolToString(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_pushfstring(L,LUA_UEFI_PROTO_GRAPHICS_OUTPUT" %p",proto->proto);
	return 1;
}

static int LuaUefiGraphicsOutputProtocolBlt(lua_State*L){
	BOOLEAN alloc=FALSE;
	EFI_GRAPHICS_OUTPUT_BLT_OPERATION oper=0;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL*buff=NULL;
	GET_PROTO(L,1,proto);
	if(!str_to_blt_oper(luaL_checkstring(L,3),&oper))
		return luaL_argerror(L,3,"invalid blt operation");
	UINTN srcx=luaL_checkinteger(L,4);
	UINTN srcy=luaL_checkinteger(L,5);
	UINTN dstx=luaL_checkinteger(L,6);
	UINTN dsty=luaL_checkinteger(L,7);
	UINTN width=luaL_checkinteger(L,8);
	UINTN height=luaL_checkinteger(L,9);
	UINTN delta=luaL_optinteger(L,10,0);
	uefi_graphics_output_get_pixels(L,2,&buff,&alloc);
	EFI_STATUS status=proto->proto->Blt(
		proto->proto,
		buff,oper,
		srcx,srcy,
		dstx,dsty,
		width,height,
		delta
	);
	if(buff&&alloc)FreePool(buff);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiGraphicsOutputProtocolSetMode(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_Integer mode=luaL_checkinteger(L,2);
	EFI_STATUS status=proto->proto->SetMode(proto->proto,(UINTN)mode);
	uefi_status_to_lua(L,status);
	return 1;
}

static int LuaUefiGraphicsOutputProtocolQueryMode(lua_State*L){
	GET_PROTO(L,1,proto);
	lua_Integer mode=luaL_checkinteger(L,2);
	UINTN infos=0;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION*info=NULL;
	EFI_STATUS status=proto->proto->QueryMode(
		proto->proto,(UINTN)mode,&infos,&info
	);
	uefi_status_to_lua(L,status);
	if(EFI_ERROR(status)||!info)lua_pushnil(L);
	else load_info(L,info);
	return 2;
}

static int LuaUefiGraphicsOutputProtocolMode(lua_State*L){
	GET_PROTO(L,1,proto);
	if(!proto->proto->Mode)lua_pushnil(L);
	else load_mode(L,proto->proto->Mode);
	return 1;
}

EFI_GRAPHICS_OUTPUT_PROTOCOL*uefi_lua_to_graphics_output_protocol(lua_State*L,int n){
	OPT_PROTO(L,n,proto);
	return proto?proto->proto:NULL;
}

void uefi_graphics_output_protocol_to_lua(lua_State*L,EFI_GRAPHICS_OUTPUT_PROTOCOL*proto){
	struct lua_uefi_graphics_output_proto*e;
	if(!proto){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_graphics_output_proto));
	luaL_getmetatable(L,LUA_UEFI_PROTO_GRAPHICS_OUTPUT);
	lua_setmetatable(L,-2);
	e->proto=proto;
}

struct lua_uefi_meta_table LuaUefiGraphicsOutputProtocolMetaTable={
	.name=LUA_UEFI_PROTO_GRAPHICS_OUTPUT,
	.reg=(const luaL_Reg[]){
		{"QueryMode", LuaUefiGraphicsOutputProtocolQueryMode},
		{"SetMode",   LuaUefiGraphicsOutputProtocolSetMode},
		{"Blt",       LuaUefiGraphicsOutputProtocolBlt},
		{"Mode",      LuaUefiGraphicsOutputProtocolMode},
		{NULL, NULL}
	},
	.tostring=LuaUefiGraphicsOutputProtocolToString,
	.gc=NULL,
};

#endif
