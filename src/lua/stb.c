/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_UEFI
#define STBI_NO_STDIO
#endif
#ifdef ENABLE_LUA
#include<string.h>
#include"xlua.h"
#include"logger.h"
#include"stb_image.h"
#include"stb_image_resize.h"
#include"stb_image_write.h"
#define TAG "lua"

static int lua_stbi_load(lua_State*L){
	size_t size=0;
	int x=0,y=0,c=0;
	void*ret=NULL,*buf=NULL;
	int dc=luaL_optinteger(L,2,4);
	if(dc<3||dc>4)return luaL_argerror(L,2,"invalid channels");
	switch(lua_type(L,1)){
		case LUA_TSTRING:{
			const char*file=luaL_checkstring(L,1);
			if(fs_read_whole_file(NULL,file,&buf,&size)==0&&buf&&size>0)
				ret=stbi_load_from_memory(buf,size,&x,&y,&c,dc);
		}break;
		case LUA_TUSERDATA:{
			#ifndef STBI_NO_STDIO
			struct luaL_Stream*d1;
			if((d1=luaL_testudata(L,1,LUA_FILEHANDLE))){
				luaL_argcheck(L,1,d1->f!=NULL,"file must not null");
				ret=stbi_load_from_file(d1->f,&x,&y,&c,dc);
				break;
			}
			#endif
			struct lua_data*d2;
			if((d2=luaL_testudata(L,1,LUA_DATA))){
				luaL_argcheck(L,1,d2->data!=NULL,"data must not null");
				luaL_argcheck(L,1,d2->size>0,"data too small");
				ret=stbi_load_from_memory(d2->data,d2->size,&x,&y,&c,dc);
			}
			struct lua_fsh*d3;
			if((d3=luaL_testudata(L,1,LUA_DATA))){
				luaL_argcheck(L,1,d3->f!=NULL,"fsh must not null");
				if(fs_read_all(d3->f,&buf,&size)==0&&buf&&size>0)
					ret=stbi_load_from_memory(buf,size,&x,&y,&c,dc);
			}
		}/*fallthrough*/
		default:return luaL_argerror(L,1,"unknown argument type");
	}
	if(x<0||y<0||c<0||!ret)lua_pushnil(L);
	else lua_data_to_lua(L,true,ret,x*y*c);
	lua_pushinteger(L,x);
	lua_pushinteger(L,y);
	lua_pushinteger(L,c);
	if(buf)free(buf);
	return 4;
}

static int lua_stbi_reason(lua_State*L){
	const char*reason=stbi_failure_reason();
	if(!reason)lua_pushnil(L);
	else lua_pushstring(L,reason);
	return 1;
}

static int lua_stbir_resize(lua_State*L){
	size_t size=0,len=0;
	void*data=NULL,*ret=NULL;
	lua_arg_get_data(L,1,false,&data,&size);
	int iw=luaL_checkinteger(L,2);
	int ih=luaL_checkinteger(L,3);
	int ow=luaL_checkinteger(L,4);
	int oh=luaL_checkinteger(L,5);
	int ch=luaL_optinteger(L,6,4);
	len=ow*oh*ch;
	if(!data||size<=0)return luaL_argerror(L,1,"invalid image");
	if(iw<=0)return luaL_argerror(L,2,"invalid input width");
	if(ih<=0)return luaL_argerror(L,3,"invalid input height");
	if(ow<=0)return luaL_argerror(L,4,"invalid output width");
	if(oh<=0)return luaL_argerror(L,5,"invalid output height");
	if(ch<3||ch>4)return luaL_argerror(L,6,"invalid channels");
	if(size<(size_t)(iw*ih*ch))return luaL_argerror(L,1,"image too small");
	if(!(ret=malloc(len)))return luaL_error(L,"allocate output buffer failed");
	memset(ret,0,len);
	int r=stbir_resize_uint8(data,iw,ih,0,ret,ow,oh,0,ch);
	if(r!=1){
		free(ret);
		lua_pushnil(L);
	}else lua_data_to_lua(L,true,ret,len);
	return 1;
}

struct write_data{
	void*data;
	size_t size;
};

static void stbiw_write_callback(void*context,void*data,int size){
	struct write_data*wd=context;
	if(!wd||!data||size<=0)return;
	size_t rs=wd->size+size;
	void*p=wd->data?realloc(wd->data,rs):malloc(rs);
	if(!p)return;
	memcpy(p+wd->size,data,size);
	if(wd->data)free(wd->data);
	wd->size=rs,wd->data=p;
}

static int lua_stbiw_write(lua_State*L){
	int ret=0;
	size_t size=0;
	void*data=NULL;
	struct write_data wd;
	const char*type=luaL_checkstring(L,1);
	lua_arg_get_data(L,2,false,&data,&size);
	int w=luaL_checkinteger(L,3);
	int h=luaL_checkinteger(L,4);
	int c=luaL_optinteger(L,5,4);
	if(!data||size<=0)return luaL_argerror(L,2,"invalid data");
	if(w<=0)return luaL_argerror(L,3,"invalid width");
	if(h<=0)return luaL_argerror(L,4,"invalid height");
	if(c<3||c>4)return luaL_argerror(L,5,"invalid channels");
	memset(&wd,0,sizeof(wd));
	if(strcasecmp(type,"png")==0)
		ret=stbi_write_png_to_func(stbiw_write_callback,&wd,w,h,c,data,0);
	else if(strcasecmp(type,"bmp")==0)
		ret=stbi_write_bmp_to_func(stbiw_write_callback,&wd,w,h,c,data);
	else if(strcasecmp(type,"tga")==0)
		ret=stbi_write_tga_to_func(stbiw_write_callback,&wd,w,h,c,data);
	else if(strcasecmp(type,"hdr")==0)
		ret=stbi_write_hdr_to_func(stbiw_write_callback,&wd,w,h,c,data);
	else if(strcasecmp(type,"jpg")==0){
		int q=luaL_optinteger(L,6,90);
		if(c<1||c>100)return luaL_argerror(L,6,"invalid quality");
		ret=stbi_write_jpg_to_func(stbiw_write_callback,&wd,w,h,c,data,q);
	}else return luaL_argerror(L,1,"invalid type");
	if(ret!=1){
		if(wd.data)free(wd.data);
		memset(&wd,0,sizeof(wd));
	}
	if(!data||size<=0)lua_pushnil(L);
	else lua_data_to_lua(L,true,wd.data,wd.size);
	return 1;
}

static const luaL_Reg stb_image_lib[]={
	{"reason", lua_stbi_reason},
	{"load",   lua_stbi_load},
	{"resize", lua_stbir_resize},
	{"write",  lua_stbiw_write},
	{NULL, NULL}
};

LUAMOD_API int luaopen_stb(lua_State*L){
	lua_createtable(L,0,0);
	lua_pushliteral(L,"image");
	luaL_newlib(L,stb_image_lib);
	lua_settable(L,-3);
	return 1;
}
#endif
