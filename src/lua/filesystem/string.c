/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fs.h"

bool lua_fs_get_flag(lua_State*L,int idx,bool nil,fs_file_flag*flag){
	switch(lua_type(L,idx)){
		case LUA_TNUMBER:*flag=luaL_checkinteger(L,idx);break;
		case LUA_TSTRING:{
			const char*str=luaL_checkstring(L,idx);
			for(size_t i=0;str[i];i++)switch(tolower(str[i])){
				case 'r':*flag|=FILE_FLAG_READ;break;
				case 'w':*flag|=FILE_FLAG_WRITE;break;
				case 'e':*flag|=FILE_FLAG_ACCESS;break;
				case 'c':*flag|=FILE_FLAG_CREATE;break;
				case 's':*flag|=FILE_FLAG_SYNC;break;
				case 'i':*flag|=FILE_FLAG_DIRECT;break;
				case 'a':*flag|=FILE_FLAG_APPEND;break;
				case 't':*flag|=FILE_FLAG_TRUNCATE;break;
				case 'n':*flag|=FILE_FLAG_NON_BLOCK;break;
				case 'd':*flag|=FILE_FLAG_FOLDER;break;
				case 'x':*flag|=FILE_FLAG_EXECUTE;break;
				case 'h':*flag|=FILE_FLAG_SHARED;break;
				case 'p':*flag|=FILE_FLAG_PRIVATE;break;
				case 'f':*flag|=FILE_FLAG_FIXED;break;
				default:luaL_argerror(L,idx,"invalid flag");return false;
			}
		}break;
		case LUA_TNIL:case LUA_TNONE:if(nil)break;//fallthrough
		default:luaL_argerror(L,idx,"unknown flag");return false;
	}
	return true;
}

bool lua_fs_get_type(lua_State*L,int idx,bool nil,fs_type*type){
	switch(lua_type(L,idx)){
		case LUA_TNUMBER:*type=luaL_checkinteger(L,idx);break;
		case LUA_TSTRING:{
			const char*str=luaL_checkstring(L,idx);
			if(strcasecmp(str,"parent")==0)*type=FS_TYPE_PARENT;
			else if(strcasecmp(str,"file")==0)*type=FS_TYPE_FILE;
			else if(strcasecmp(str,"volume")==0)*type=FS_TYPE_VOLUME;
			else if(strcasecmp(str,"reg")==0)*type=FS_TYPE_FILE_REG;
			else if(strcasecmp(str,"folder")==0)*type=FS_TYPE_FILE_FOLDER;
			else if(strcasecmp(str,"link")==0)*type=FS_TYPE_FILE_LINK;
			else if(strcasecmp(str,"socket")==0)*type=FS_TYPE_FILE_SOCKET;
			else if(strcasecmp(str,"block")==0)*type=FS_TYPE_FILE_BLOCK;
			else if(strcasecmp(str,"char")==0)*type=FS_TYPE_FILE_CHAR;
			else if(strcasecmp(str,"fifo")==0)*type=FS_TYPE_FILE_FIFO;
			else if(strcasecmp(str,"whiteout")==0)*type=FS_TYPE_FILE_WHITEOUT;
			else if(strcasecmp(str,"virtual")==0)*type=FS_TYPE_VOLUME_VIRTUAL;
			else if(strcasecmp(str,"hdd")==0)*type=FS_TYPE_VOLUME_HDD;
			else if(strcasecmp(str,"ssd")==0)*type=FS_TYPE_VOLUME_SSD;
			else if(strcasecmp(str,"card")==0)*type=FS_TYPE_VOLUME_CARD;
			else if(strcasecmp(str,"flash")==0)*type=FS_TYPE_VOLUME_FLASH;
			else if(strcasecmp(str,"tape")==0)*type=FS_TYPE_VOLUME_TAPE;
			else if(strcasecmp(str,"rom")==0)*type=FS_TYPE_VOLUME_ROM;
			else if(strcasecmp(str,"cd")==0)*type=FS_TYPE_VOLUME_CD;
			else if(strcasecmp(str,"usb")==0)*type=FS_TYPE_VOLUME_USB;
			else{
				luaL_argerror(L,idx,"unknown type");
				return false;
			}
		}break;
		case LUA_TNIL:case LUA_TNONE:if(nil)break;//fallthrough
		default:luaL_argerror(L,idx,"unknown type");return false;
	}
	return true;
}

bool lua_fs_get_feature(lua_State*L,int idx,bool nil,fs_feature*feature){
	switch(lua_type(L,idx)){
		case LUA_TNUMBER:*feature=luaL_checkinteger(L,idx);break;
		case LUA_TSTRING:{
			const char*str=luaL_checkstring(L,idx);
			if(strcasecmp(str,"readable")==0)*feature=FS_FEATURE_READABLE;
			else if(strcasecmp(str,"writable")==0)*feature=FS_FEATURE_WRITABLE;
			else if(strcasecmp(str,"seekable")==0)*feature=FS_FEATURE_SEEKABLE;
			else if(strcasecmp(str,"unix-perm")==0)*feature=FS_FEATURE_UNIX_PERM;
			else if(strcasecmp(str,"unix-device")==0)*feature=FS_FEATURE_UNIX_DEVICE;
			else if(strcasecmp(str,"non-block")==0)*feature=FS_FEATURE_NON_BLOCK;
			else if(strcasecmp(str,"have-stat")==0)*feature=FS_FEATURE_HAVE_STAT;
			else if(strcasecmp(str,"have-size")==0)*feature=FS_FEATURE_HAVE_SIZE;
			else if(strcasecmp(str,"have-path")==0)*feature=FS_FEATURE_HAVE_PATH;
			else if(strcasecmp(str,"have-time")==0)*feature=FS_FEATURE_HAVE_TIME;
			else if(strcasecmp(str,"have-folder")==0)*feature=FS_FEATURE_HAVE_FOLDER;
			else{
				luaL_argerror(L,idx,"unknown feature");
				return false;
			}
		}break;
		case LUA_TNIL:case LUA_TNONE:if(nil)break;//fallthrough
		default:luaL_argerror(L,idx,"unknown feature");return false;
	}
	return true;
}
