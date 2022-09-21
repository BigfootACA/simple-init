/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include<string.h>
#include"xlua.h"
#include"aboot.h"
#include"logger.h"
#ifdef ENABLE_UEFI
#include"uefi.h"
#include"compatible.h"
#include"uefi/lua_uefi.h"
#endif
#undef TAG
#define TAG "lua"

#define LUA_ABOOTIMG "Android Boot Image"
#define OPT_ABOOTIMG(L,n,var) OPT_UDATA(L,n,var,lua_abootimg,LUA_ABOOTIMG)
#define GET_ABOOTIMG(L,n,var) OPT_ABOOTIMG(L,n,var);CHECK_NULL(L,n,var)
struct lua_abootimg{aboot_image*img;};

void lua_abootimg_to_lua(lua_State*L,aboot_image*img){
	if(!img){
		lua_pushnil(L);
		return;
	}
	struct lua_abootimg*e;
	e=lua_newuserdata(L,sizeof(struct lua_abootimg));
	luaL_getmetatable(L,LUA_ABOOTIMG);
	lua_setmetatable(L,-2);
	memset(e,0,sizeof(struct lua_abootimg));
	e->img=img;
}

static int aboot_img_save(lua_State*L){
	bool ret=false;
	GET_ABOOTIMG(L,1,img);
	switch(lua_type(L,1)){
		case LUA_TSTRING:{
			const char*path=luaL_checkstring(L,1);
			#ifdef ENABLE_UEFI
			ret=abootimg_save_to_url_path(img->img,path);
			#else
			int cfd=luaL_optinteger(L,2,AT_FDCWD);
			ret=abootimg_save_to_file(img->img,cfd,path);
			#endif
		}break;
		#ifndef ENABLE_UEFI
		case LUA_TNUMBER:{
			int fd=luaL_checkinteger(L,1);
			ret=abootimg_save_to_fd(img->img,fd);
		}break;
		#else
		case LUA_TUSERDATA:{
			EFI_BLOCK_IO_PROTOCOL*d1;
			if((d1=uefi_lua_to_block_io_protocol(L,1))){
				ret=abootimg_save_to_blockio(img->img,d1);
				break;
			}
			EFI_FILE_PROTOCOL*d2;
			if((d2=uefi_lua_to_file_protocol(L,1))){
				CHAR16*path=NULL;
				lua_arg_get_char16(L,2,true,&path);
				if(path){
					ret=abootimg_save_to_wfile(img->img,d2,path);
					FreePool(path);
				}else ret=abootimg_save_to_fp(img->img,d2);
				break;
			}
		}
		#endif
		//fallthrough
		default:return luaL_argerror(L,1,"unknown argument type");
	}
	lua_pushboolean(L,ret);
	return 1;
}

static int aboot_img_size(lua_State*L){
	GET_ABOOTIMG(L,1,img);
	lua_pushinteger(L,abootimg_get_image_size(img->img));
	return 1;
}

#ifdef ENABLE_UEFI
#define IMPL_SAVE(tag)\
	static int aboot_img_save_##tag(lua_State*L){\
		bool ret=false;\
		GET_ABOOTIMG(L,1,img);\
		switch(lua_type(L,1)){\
			case LUA_TSTRING:{\
				const char*path=luaL_checkstring(L,1);\
				ret=abootimg_save_##tag##_to_url_path(img->img,path);\
			}break;\
			case LUA_TUSERDATA:{\
				EFI_BLOCK_IO_PROTOCOL*d1;\
				if((d1=uefi_lua_to_block_io_protocol(L,1))){\
					ret=abootimg_save_##tag##_to_blockio(img->img,d1);\
					break;\
				}\
				EFI_FILE_PROTOCOL*d2;\
				if((d2=uefi_lua_to_file_protocol(L,1))){\
					CHAR16*path=NULL;\
					lua_arg_get_char16(L,2,true,&path);\
					if(path){\
						ret=abootimg_save_##tag##_to_wfile(img->img,d2,path);\
						FreePool(path);\
					}else ret=abootimg_save_##tag##_to_fp(img->img,d2);\
					break;\
				}\
			}/*fallthrough*/\
			default:return luaL_argerror(L,1,"unknown argument type");\
		}\
		lua_pushboolean(L,ret);\
		return 1;\
	}\
	static int aboot_img_load_##tag(lua_State*L){\
		bool ret=false;\
		GET_ABOOTIMG(L,1,img);\
		switch(lua_type(L,1)){\
			case LUA_TSTRING:{\
				const char*path=luaL_checkstring(L,1);\
				ret=abootimg_load_##tag##_from_url_path(img->img,path);\
			}break;\
			case LUA_TUSERDATA:{\
				EFI_BLOCK_IO_PROTOCOL*d1;\
				if((d1=uefi_lua_to_block_io_protocol(L,1))){\
					ret=abootimg_load_##tag##_from_blockio(img->img,d1);\
					break;\
				}\
				EFI_FILE_PROTOCOL*d2;\
				if((d2=uefi_lua_to_file_protocol(L,1))){\
					CHAR16*path=NULL;\
					lua_arg_get_char16(L,2,true,&path);\
					if(path){\
						ret=abootimg_load_##tag##_from_wfile(img->img,d2,path);\
						FreePool(path);\
					}else ret=abootimg_load_##tag##_from_fp(img->img,d2);\
					break;\
				}\
				struct lua_data*d3;\
				if((d3=luaL_testudata(L,1,LUA_DATA))){\
					ret=abootimg_set_##tag(img->img,d3->data,d3->size);\
					break;\
				}\
			}/*fallthrough*/\
			default:return luaL_argerror(L,1,"unknown argument type");\
		}\
		lua_pushboolean(L,ret);\
		return 1;\
	}
#else
#define IMPL_SAVE(tag)\
	static int aboot_img_save_##tag(lua_State*L){\
		bool ret=false;\
		GET_ABOOTIMG(L,1,img);\
		switch(lua_type(L,1)){\
			case LUA_TSTRING:{\
				const char*path=luaL_checkstring(L,1);\
				int cfd=luaL_optinteger(L,2,AT_FDCWD);\
				ret=abootimg_save_##tag##_to_file(img->img,cfd,path);\
			}break;\
			case LUA_TNUMBER:{\
				int fd=luaL_checkinteger(L,1);\
				ret=abootimg_save_##tag##_to_fd(img->img,fd);\
			}break;\
			default:return luaL_argerror(L,1,"unknown argument type");\
		}\
		lua_pushboolean(L,ret);\
		return 1;\
	}\
	static int aboot_img_load_##tag(lua_State*L){\
		bool ret=false;\
		GET_ABOOTIMG(L,1,img);\
		switch(lua_type(L,1)){\
			case LUA_TSTRING:{\
				const char*path=luaL_checkstring(L,1);\
				int cfd=luaL_optinteger(L,2,AT_FDCWD);\
				ret=abootimg_load_##tag##_from_file(img->img,cfd,path);\
			}break;\
			case LUA_TNUMBER:{\
				int fd=luaL_checkinteger(L,1);\
				ret=abootimg_load_##tag##_from_fd(img->img,fd);\
			}break;\
			case LUA_TUSERDATA:{\
				struct lua_data*d1;\
				if((d1=luaL_testudata(L,1,LUA_DATA))){\
					ret=abootimg_set_##tag(img->img,d1->data,d1->size);\
					break;\
				}\
			}/*fallthrough*/\
			default:return luaL_argerror(L,1,"unknown argument type");\
		}\
		lua_pushboolean(L,ret);\
		return 1;\
	}
#endif
#define IMPL_GET(tag)\
	static int aboot_img_get_##tag(lua_State*L){\
		GET_ABOOTIMG(L,1,img);\
		size_t len=abootimg_get_##tag##_size(img->img);\
		void*data=malloc(len);\
		if(data&&!abootimg_copy_##tag(img->img,data,len)){\
			free(data);\
			data=NULL;\
		}\
		if(!data)lua_pushnil(L);\
		else lua_data_to_lua(L,true,data,len);\
		return 1;\
	}
#define IMPL_STRING(tag)\
	static int aboot_img_##tag(lua_State*L){\
		GET_ABOOTIMG(L,1,img);\
		if(!lua_isnoneornil(L,2))abootimg_set_##tag(\
			img->img,luaL_checkstring(L,2)\
		);\
		lua_pushstring(L,abootimg_get_##tag(img->img));\
		return 1;\
	}
#define IMPL_INTEGER(tag)\
	static int aboot_img_##tag(lua_State*L){\
		GET_ABOOTIMG(L,1,img);\
		if(!lua_isnoneornil(L,2))abootimg_set_##tag(\
			img->img,luaL_checkinteger(L,2)\
		);\
		lua_pushinteger(L,abootimg_get_##tag(img->img));\
		return 1;\
	}
#define IMPL_PART(tag)\
	IMPL_GET(tag)\
	IMPL_SAVE(tag)\
	IMPL_INTEGER(tag##_size)\
	IMPL_INTEGER(tag##_address)\

IMPL_PART(kernel)
IMPL_PART(ramdisk)
IMPL_PART(second)
IMPL_STRING(name)
IMPL_STRING(cmdline)
IMPL_INTEGER(page_size)
IMPL_INTEGER(tags_address)

static int abootimg_to_string(lua_State*L){
	GET_ABOOTIMG(L,1,img);
	lua_pushfstring(
		L,LUA_ABOOTIMG" %p",
		img->img
	);
	return 1;
}

static int abootimg_gc(lua_State*L){
	GET_ABOOTIMG(L,1,img);
	if(img->img)abootimg_free(img->img);
	img->img=NULL;
	return 1;
}

static int aboot_img_lib_create(lua_State*L){
	lua_abootimg_to_lua(L,abootimg_new_image());
	return 1;
}

static int aboot_img_lib_load(lua_State*L){
	aboot_image*img=NULL;
	switch(lua_type(L,1)){
		case LUA_TSTRING:{
			const char*path=luaL_checkstring(L,1);
			#ifdef ENABLE_UEFI
			img=abootimg_load_from_url_path(path);
			#else
			int cfd=luaL_optinteger(L,2,AT_FDCWD);
			img=abootimg_load_from_file(cfd,path);
			#endif
		}break;
		#ifndef ENABLE_UEFI
		case LUA_TNUMBER:{
			int fd=luaL_checkinteger(L,1);
			img=abootimg_load_from_fd(fd);
		}break;
		#else
		case LUA_TUSERDATA:{
			EFI_BLOCK_IO_PROTOCOL*d1;
			if((d1=uefi_lua_to_block_io_protocol(L,1))){
				img=abootimg_load_from_blockio(d1);
				break;
			}
			EFI_FILE_PROTOCOL*d2;
			if((d2=uefi_lua_to_file_protocol(L,1))){
				CHAR16*path=NULL;
				lua_arg_get_char16(L,2,true,&path);
				if(path){
					img=abootimg_load_from_wfile(d2,path);
					FreePool(path);
				}else img=abootimg_load_from_fp(d2);
				break;
			}
			struct lua_data*d3;
			if((d3=luaL_testudata(L,1,LUA_DATA))){
				img=abootimg_load_from_memory(d3->data,d3->size);
				break;
			}
		}
		#endif
		//fallthrough
		default:return luaL_argerror(L,1,"unknown argument type");
	}
	lua_abootimg_to_lua(L,img);
	return 1;
}

static const luaL_Reg aboot_img_lib[]={
	{"create",      aboot_img_lib_create},
	{"load",        aboot_img_lib_load},
	{NULL, NULL}
};

#define DECL_GETSET(tag)\
	{#tag,       aboot_img_##tag},\
	{"get_"#tag, aboot_img_##tag},\
	{"set_"#tag, aboot_img_##tag},
#define DECL_CONT(tag)\
	{"load_"#tag, aboot_img_load_##tag},\
	{"save_"#tag, aboot_img_save_##tag},\
	{"get_"#tag,  aboot_img_get_##tag},
#define DECL_PART(tag)\
	DECL_GETSET(tag##_size)\
	DECL_GETSET(tag##_address)\
	DECL_CONT(tag)
static luaL_Reg abootimg_meta[]={
	{"save",     aboot_img_save},
	{"size",     aboot_img_size},
	{"get_size", aboot_img_size},
	DECL_GETSET(name)
	DECL_GETSET(cmdline)
	DECL_GETSET(page_size)
	DECL_GETSET(tags_address)
	DECL_PART(kernel)
	DECL_PART(ramdisk)
	DECL_PART(second)
	{NULL,NULL}
};

LUAMOD_API int luaopen_abootimg(lua_State*L){
	xlua_create_metatable(
		L,LUA_ABOOTIMG,
		abootimg_meta,
		abootimg_to_string,
		abootimg_gc
	);
	luaL_newlib(L,aboot_img_lib);
	return 1;
}
#endif
