/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include<stdbool.h>
#include<string.h>
#include"xlua.h"

LUAMOD_API int lua_feature(lua_State*L){
	bool result=false;
	const char*key=luaL_checkstring(L,1);
	if(!key)return 0;
	#ifdef ENABLE_UEFI
	if(strcasecmp(key,"uefi")==0)result=true;
	#endif
	#ifdef ENABLE_READLINE
	if(strcasecmp(key,"initshell")==0)result=true;
	#endif
	#ifdef ENABLE_FREETYPE2
	if(strcasecmp(key,"freetype2")==0)result=true;
	#endif
	#ifdef ENABLE_GUI
	if(strcasecmp(key,"gui")==0)result=true;
	#endif
	#ifdef ENABLE_DRM
	if(strcasecmp(key,"drm")==0)result=true;
	#endif
	#ifdef ENABLE_GTK
	if(strcasecmp(key,"gtk")==0)result=true;
	#endif
	#ifdef ENABLE_SDL2
	if(strcasecmp(key,"sdl2")==0)result=true;
	#endif
	#ifdef ENABLE_LODEPNG
	if(strcasecmp(key,"lodepng")==0)result=true;
	#endif
	#ifdef ENABLE_LIBJPEG
	if(strcasecmp(key,"libjpeg")==0)result=true;
	#endif
	#ifdef ENABLE_NANOSVG
	if(strcasecmp(key,"nanosvg")==0)result=true;
	#endif
	#ifdef ENABLE_JSONC
	if(strcasecmp(key,"jsonc")==0)result=true;
	#endif
	#ifdef ENABLE_MXML
	if(strcasecmp(key,"mxml")==0)result=true;
	#endif
	#ifdef ENABLE_VNCSERVER
	if(strcasecmp(key,"vncserver")==0)result=true;
	#endif
	#ifdef ENABLE_HIVEX
	if(strcasecmp(key,"hivex")==0)result=true;
	#endif
	#ifdef ENABLE_STB
	if(strcasecmp(key,"stb")==0)result=true;
	#endif
	#ifdef ENABLE_LUA
	if(strcasecmp(key,"lua")==0)result=true;
	#endif
	if(strcasecmp(key,"mount")==0)result=true;
	lua_pushboolean(L,result);
	return 1;
}
#endif
