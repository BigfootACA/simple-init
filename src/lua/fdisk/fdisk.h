/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _LUA_FDISK_H
#define _LUA_FDISK_H
#include<ctype.h>
#include<stdlib.h>
#include<string.h>
#include<sys/sysmacros.h>
#include<libfdisk/libfdisk.h>
#include"xlua.h"

#define LUA_FDISK_ASK "FDisk Ask"
#define LUA_FDISK_CONTEXT "FDisk Context"
#define LUA_FDISK_FIELD "FDisk Field"
#define LUA_FDISK_LABEL "FDisk Label"
#define LUA_FDISK_PARTITION "FDisk Partition"
#define LUA_FDISK_PARTTYPE "FDisk Partition Type"
#define LUA_FDISK_SCRIPT "FDisk Script"
#define LUA_FDISK_TABLE "FDisk Table"
#define LUA_FDISK_LABELITEM "FDisk Label Item"
#define LUA_FDISK_ITER "FDisk Iterator"
struct lua_fdisk_cb{int func_ref,data_ref;};
struct lua_fdisk_ask{struct fdisk_ask*data;};
struct lua_fdisk_field{struct fdisk_context*parent;struct fdisk_field*data;};
struct lua_fdisk_label{struct fdisk_context*parent;struct fdisk_label*data;};
struct lua_fdisk_partition{struct fdisk_partition*data;};
struct lua_fdisk_parttype{struct fdisk_parttype*data;};
struct lua_fdisk_script{struct fdisk_context*parent;struct fdisk_script*data;};
struct lua_fdisk_table{struct fdisk_table*data;};
struct lua_fdisk_labelitem{struct fdisk_labelitem*data;};
struct lua_fdisk_iter{struct fdisk_iter*data;};
struct lua_fdisk_context{struct fdisk_context*data;struct lua_fdisk_cb ask_cb;};
struct lua_fdisk_meta_table{
	char*name;
	const luaL_Reg*reg;
	const lua_CFunction tostring;
	const lua_CFunction gc;
};
extern struct lua_fdisk_meta_table lua_fdisk_ask;
extern struct lua_fdisk_meta_table lua_fdisk_context;
extern struct lua_fdisk_meta_table lua_fdisk_field;
extern struct lua_fdisk_meta_table lua_fdisk_label;
extern struct lua_fdisk_meta_table lua_fdisk_partition;
extern struct lua_fdisk_meta_table lua_fdisk_parttype;
extern struct lua_fdisk_meta_table lua_fdisk_script;
extern struct lua_fdisk_meta_table lua_fdisk_table;
extern struct lua_fdisk_meta_table lua_fdisk_labelitem;
extern struct lua_fdisk_meta_table lua_fdisk_iter;
extern bool lua_fdisk_check_error(lua_State*L,int ret);
extern void lua_fdisk_ask_to_lua(lua_State*L,bool allocated,struct fdisk_ask*data);
extern void lua_fdisk_context_to_lua(lua_State*L,bool allocated,struct fdisk_context*data);
extern void lua_fdisk_field_to_lua(lua_State*L,struct fdisk_context*parent,struct fdisk_field*data);
extern void lua_fdisk_label_to_lua(lua_State*L,struct fdisk_context*parent,struct fdisk_label*data);
extern void lua_fdisk_partition_to_lua(lua_State*L,bool allocated,struct fdisk_partition*data);
extern void lua_fdisk_parttype_to_lua(lua_State*L,bool allocated,struct fdisk_parttype*data);
extern void lua_fdisk_script_to_lua(lua_State*L,bool allocated,struct fdisk_context*parent,struct fdisk_script*data);
extern void lua_fdisk_table_to_lua(lua_State*L,bool allocated,struct fdisk_table*data);
extern void lua_fdisk_labelitem_to_lua(lua_State*L,bool allocated,struct fdisk_labelitem*data);
extern void lua_fdisk_iter_to_lua(lua_State*L,struct fdisk_iter*data);
#endif
