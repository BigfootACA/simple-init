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
#include"logger.h"
#include"nanosvg.h"
#include"nanosvgrast.h"
#include"filesystem.h"
#ifdef ENABLE_UEFI
#include"uefi.h"
#include"compatible.h"
#include<Library/MemoryAllocationLib.h>
#endif
#define TAG "lua"

#define LUA_NANOSVG_IMG "NanoSVG Image"
#define LUA_NANOSVG_RAST "NanoSVG Rasterizer"
#define OPT_NSVG_IMG(L,n,var) OPT_UDATA(L,n,var,lua_nanosvg_img,LUA_NANOSVG_IMG)
#define GET_NSVG_IMG(L,n,var) OPT_NSVG_IMG(L,n,var);CHECK_NULL(L,n,var)
#define OPT_NSVG_RAST(L,n,var) OPT_UDATA(L,n,var,lua_nanosvg_rast,LUA_NANOSVG_RAST)
#define GET_NSVG_RAST(L,n,var) OPT_NSVG_RAST(L,n,var);CHECK_NULL(L,n,var)
struct lua_nanosvg_img{NSVGimage*img;};
struct lua_nanosvg_rast{NSVGrasterizer*rast;};

void lua_nanosvg_img_to_lua(lua_State*L,NSVGimage*img){
	if(!img){
		lua_pushnil(L);
		return;
	}
	struct lua_nanosvg_img*e;
	e=lua_newuserdata(L,sizeof(struct lua_nanosvg_img));
	luaL_getmetatable(L,LUA_NANOSVG_IMG);
	lua_setmetatable(L,-2);
	memset(e,0,sizeof(struct lua_nanosvg_img));
	e->img=img;
}

void lua_nanosvg_rast_to_lua(lua_State*L,NSVGrasterizer*rast){
	if(!rast){
		lua_pushnil(L);
		return;
	}
	struct lua_nanosvg_rast*e;
	e=lua_newuserdata(L,sizeof(struct lua_nanosvg_rast));
	luaL_getmetatable(L,LUA_DATA);
	lua_setmetatable(L,-2);
	memset(e,0,sizeof(struct lua_nanosvg_rast));
	e->rast=rast;
}

static int nanosvg_rasterize(lua_State*L,int n,NSVGimage*img,NSVGrasterizer*rast){
	float tx=luaL_optnumber(L,n+1,0.0);
	float ty=luaL_optnumber(L,n+2,0.0);
	float scale=luaL_optnumber(L,n+3,1.0);
	luaL_argcheck(L,scale>0,n+3,"scale must greater than zero");
	int width=luaL_optinteger(L,n+4,img->width*scale);
	int height=luaL_optinteger(L,n+5,img->height*scale);
	int stride=luaL_optinteger(L,n+6,width*4);
	luaL_argcheck(L,width>0,n+4,"width must greater than zero");
	luaL_argcheck(L,height>0,n+5,"height must greater than zero");
	luaL_argcheck(L,stride>0,n+6,"stride must greater than zero");
	size_t size=width*height*4;
	void*data=malloc(size);
	if(!data)return luaL_error(L,"alloc image failed");
	memset(data,0,size);
	nsvgRasterize(rast,img,tx,ty,scale,data,width,height,stride);
	lua_data_to_lua(L,true,data,size);
	lua_pushinteger(L,width);
	lua_pushinteger(L,height);
	return 3;
}

static int nanosvg_lib_create_rast(lua_State*L){
	lua_nanosvg_rast_to_lua(L,nsvgCreateRasterizer());
	return 1;
}

static int nanosvg_lib_parse(lua_State*L){
	NSVGimage*img=NULL;
	const char*input=luaL_checkstring(L,1);
	const char*units=luaL_optstring(L,2,"px");
	float dpi=luaL_optnumber(L,3,200.0);
	img=nsvgParse((char*)input,units,dpi);
	lua_nanosvg_img_to_lua(L,img);
	return 1;
}

static int nanosvg_lib_parse_file(lua_State*L){
	NSVGimage*img=NULL;
	const char*file=luaL_checkstring(L,1);
	const char*units=luaL_optstring(L,2,"px");
	float dpi=luaL_optnumber(L,3,200.0);
	char*data=NULL;
	if(fs_read_whole_file(NULL,file,(void**)&data,NULL)!=0||!data)
		return luaL_error(L,"open file %s failed",file);
	img=nsvgParse(data,units,dpi);
	free(data);
	lua_nanosvg_img_to_lua(L,img);
	return 1;
}

static int nanosvg_lib_rast(lua_State*L){
	int n=1;
	bool alloc=false;
	NSVGrasterizer*r=NULL;
	if(luaL_testudata(L,n,LUA_NANOSVG_RAST)){
		GET_NSVG_RAST(L,n,rast);
		luaL_argcheck(L,rast->rast!=NULL,n,"rasterizer must not null");
		r=rast->rast,n++;
	}else{
		if(!(r=nsvgCreateRasterizer()))
			return luaL_error(L,"create rasterizer failed");
		alloc=true;
	}
	GET_NSVG_IMG(L,n,img);
	luaL_argcheck(L,img->img!=NULL,n,"image must not null");
	int x=nanosvg_rasterize(L,n+1,img->img,r);
	if(alloc)nsvgDeleteRasterizer(r);
	return x;
}

static int nanosvg_rast_rast(lua_State*L){
	GET_NSVG_RAST(L,1,rast);
	GET_NSVG_IMG(L,2,img);
	luaL_argcheck(L,rast->rast!=NULL,1,"rasterizer must not null");
	luaL_argcheck(L,img->img!=NULL,2,"image must not null");
	return nanosvg_rasterize(L,2,img->img,rast->rast);
}

static int nanosvg_rast_to_string(lua_State*L){
	GET_NSVG_RAST(L,1,nsvg);
	lua_pushfstring(
		L,LUA_NANOSVG_RAST" %p",
		nsvg->rast
	);
	return 1;
}

static int nanosvg_rast_gc(lua_State*L){
	GET_NSVG_RAST(L,1,nsvg);
	if(nsvg->rast)nsvgDeleteRasterizer(nsvg->rast);
	return 1;
}

static int nanosvg_img_rast(lua_State*L){
	int n=1;
	bool alloc=false;
	NSVGrasterizer*r=NULL;
	GET_NSVG_IMG(L,n,img);
	luaL_argcheck(L,img->img!=NULL,n,"image must not null");
	n++;
	if(luaL_testudata(L,n,LUA_NANOSVG_RAST)){
		GET_NSVG_RAST(L,n,rast);
		luaL_argcheck(L,rast->rast!=NULL,n,"rasterizer must not null");
		r=rast->rast,n++;
	}else{
		if(!(r=nsvgCreateRasterizer()))
			return luaL_error(L,"create rasterizer failed");
		alloc=true;
	}
	int x=nanosvg_rasterize(L,n,img->img,r);
	if(alloc)nsvgDeleteRasterizer(r);
	return x;
}

static int nanosvg_img_get_width(lua_State*L){
	GET_NSVG_IMG(L,1,nsvg);
	if(!nsvg->img)lua_pushnil(L);
	else lua_pushnumber(L,nsvg->img->width);
	return 1;
}

static int nanosvg_img_get_height(lua_State*L){
	GET_NSVG_IMG(L,1,nsvg);
	if(!nsvg->img)lua_pushnil(L);
	else lua_pushnumber(L,nsvg->img->height);
	return 1;
}

static int nanosvg_img_to_string(lua_State*L){
	GET_NSVG_IMG(L,1,nsvg);
	lua_pushfstring(
		L,LUA_NANOSVG_IMG" %p",
		nsvg->img
	);
	return 1;
}

static int nanosvg_img_gc(lua_State*L){
	GET_NSVG_IMG(L,1,nsvg);
	if(nsvg->img)nsvgDelete(nsvg->img);
	return 1;
}

static const luaL_Reg nanosvg_lib[]={
	{"parse",       nanosvg_lib_parse},
	{"parse_file",  nanosvg_lib_parse_file},
	{"get_rast",    nanosvg_lib_create_rast},
	{"rasterize",   nanosvg_lib_rast},
	{NULL, NULL}
};

static luaL_Reg nanosvg_img_meta[]={
	{"width",     nanosvg_img_get_width},
	{"height",    nanosvg_img_get_height},
	{"rasterize", nanosvg_img_rast},
	{NULL,NULL}
};

static luaL_Reg nanosvg_rast_meta[]={
	{"rasterize", nanosvg_rast_rast},
	{NULL,NULL}
};

LUAMOD_API int luaopen_nanosvg(lua_State*L){
	xlua_create_metatable(
		L,LUA_NANOSVG_IMG,
		nanosvg_img_meta,
		nanosvg_img_to_string,
		nanosvg_img_gc
	);
	xlua_create_metatable(
		L,LUA_NANOSVG_RAST,
		nanosvg_rast_meta,
		nanosvg_rast_to_string,
		nanosvg_rast_gc
	);
	luaL_newlib(L,nanosvg_lib);
	return 1;
}
#endif
