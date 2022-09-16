/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fs_internal.h"
#include"array.h"
#include"enum_conv.h"

static struct fs_type_str{
	const fs_type code;
	const char*type;
	const char*name;
}fs_type_str[]={
	#define STR(_type,_str) {.code=(_type),.type=(#_type),.name=(#_str)},
	STR(FS_TYPE_NONE,none)
	STR(FS_TYPE_PARENT,parent)
	STR(FS_TYPE_FILE_REG,file)
	STR(FS_TYPE_FILE_FOLDER,folder)
	STR(FS_TYPE_FILE_LINK,link)
	STR(FS_TYPE_FILE_SOCKET,socket)
	STR(FS_TYPE_FILE_BLOCK,block)
	STR(FS_TYPE_FILE_CHAR,char)
	STR(FS_TYPE_FILE_FIFO,fifo)
	STR(FS_TYPE_FILE_WHITEOUT,whiteout)
	STR(FS_TYPE_VOLUME_VIRTUAL,virtual)
	STR(FS_TYPE_VOLUME_HDD,hdd)
	STR(FS_TYPE_VOLUME_SSD,ssd)
	STR(FS_TYPE_VOLUME_CARD,card)
	STR(FS_TYPE_VOLUME_FLASH,flash)
	STR(FS_TYPE_VOLUME_TAPE,tape)
	STR(FS_TYPE_VOLUME_ROM,rom)
	STR(FS_TYPE_VOLUME_CD,cd)
	STR(FS_TYPE_VOLUME_USB,usb)
	STR(FS_TYPE_MAX,max)
	#undef STR
};
DECL_CMP_CONV(,fs_type_str,NULL,fs_type,code,const fs_type,string,type,const char*)
DECL_CMP_CONV(,fs_type_str,NULL,fs_type,code,const fs_type,name,name,const char*)
DECL_STRCASECMP_XCONV(,fs_type_str,string,type,const char*,fs_type,code,fs_type)
DECL_STRCASECMP_XCONV(,fs_type_str,name,name,const char*,fs_type,code,fs_type)
