/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fdisk.h"

bool lua_fdisk_check_error(lua_State*L,int ret){
	if(ret>=0)return false;
	lua_getglobal(L,"fdisk_throw");
	bool err=lua_toboolean(L,-1);
	if(!err)return false;
	luaL_error(L,"operation failed: %s",strerror(-ret));
	return true;
}

static int lua_fdisk_partname(lua_State*L){
	LUA_ARG_MAX(2);
	const char*dev=luaL_checkstring(L,1);
	size_t no=luaL_checkinteger(L,2);
	char*partname=fdisk_partname(dev,no);
	if(partname){
		lua_pushstring(L,partname);
		free(partname);
	}else lua_pushnil(L);
	return 1;
}

static int lua_fdisk_parse_version_string(lua_State*L){
	LUA_ARG_MAX(1);
	lua_pushinteger(L,fdisk_parse_version_string(luaL_checkstring(L,1)));
	return 1;
}

static int lua_fdisk_get_library_version(lua_State*L){
	LUA_ARG_MAX(0);
	const char*version=NULL;
	lua_pushinteger(L,fdisk_get_library_version(&version));
	if(!version)lua_pushnil(L);
	else lua_pushstring(L,version);
	return 2;
}

static int lua_fdisk_get_library_features(lua_State*L){
	LUA_ARG_MAX(0);
	const char**versions=NULL;
	int cnt=fdisk_get_library_features(&versions);
	if(versions){
		lua_createtable(L,0,0);
		for(int i=0;i<cnt&&versions[i];i++){
			lua_pushstring(L,versions[i]);
			lua_rawseti(L,-2,i+1);
		}
	}else lua_pushnil(L);
	return 1;
}

static int lua_fdisk_new_context(lua_State*L){
	LUA_ARG_MAX(0);
	lua_fdisk_context_to_lua(L,true,fdisk_new_context());
	return 1;
}

static int lua_fdisk_new_parttype(lua_State*L){
	LUA_ARG_MAX(0);
	lua_fdisk_parttype_to_lua(L,true,fdisk_new_parttype());
	return 1;
}

static int lua_fdisk_new_labelitem(lua_State*L){
	LUA_ARG_MAX(0);
	lua_fdisk_labelitem_to_lua(L,true,fdisk_new_labelitem());
	return 1;
}

static int lua_fdisk_new_table(lua_State*L){
	LUA_ARG_MAX(0);
	lua_fdisk_table_to_lua(L,true,fdisk_new_table());
	return 1;
}

static int lua_fdisk_new_partition(lua_State*L){
	LUA_ARG_MAX(0);
	lua_fdisk_partition_to_lua(L,true,fdisk_new_partition());
	return 1;
}

static int lua_fdisk_new_partition_auto(lua_State*L){
	LUA_ARG_MAX(5);
	struct fdisk_partition*part;
	size_t partno=luaL_optinteger(L,1,0);
	fdisk_sector_t start=luaL_optinteger(L,2,0);
	fdisk_sector_t size=luaL_optinteger(L,3,0);
	const char*name=luaL_optstring(L,4,NULL);
	struct lua_fdisk_parttype*type=NULL;
	if(!lua_isnoneornil(L,5))type=luaL_checkudata(L,5,LUA_FDISK_PARTTYPE);
	if((part=fdisk_new_partition())){
		if(partno>0)fdisk_partition_set_partno(part,partno);
		else fdisk_partition_partno_follow_default(part,true);
		if(start>0)fdisk_partition_set_start(part,start);
		else fdisk_partition_start_follow_default(part,true);
		if(size>0)fdisk_partition_set_size(part,size);
		else fdisk_partition_end_follow_default(part,true);
		if(name)fdisk_partition_set_name(part,name);
		if(type&&type->data)fdisk_partition_set_type(part,type->data);
	}
	lua_fdisk_partition_to_lua(L,true,part);
	return 1;
}

static int lua_fdisk_new_unknown_parttype(lua_State*L){
	LUA_ARG_MAX(2);
	unsigned int code=luaL_checkinteger(L,1);
	const char*typestr=luaL_checkstring(L,2);
	lua_fdisk_parttype_to_lua(L,true,fdisk_new_unknown_parttype(code,typestr));
	return 1;
}

static int lua_fdisk_new_iterator(lua_State*L){
	LUA_ARG_MAX(1);
	int direction=luaL_optinteger(L,1,FDISK_ITER_FORWARD);
	lua_fdisk_iter_to_lua(L,fdisk_new_iter(direction));
	return 1;
}

static const luaL_Reg fdisk_lib[]={
	{"partname",                   lua_fdisk_partname},
	{"parse_version_string",       lua_fdisk_parse_version_string},
	{"get_library_version",        lua_fdisk_get_library_version},
	{"get_library_features",       lua_fdisk_get_library_features},
	{"new_context",                lua_fdisk_new_context},
	{"new_parttype",               lua_fdisk_new_parttype},
	{"new_unknown_parttype",       lua_fdisk_new_unknown_parttype},
	{"new_labelitem",              lua_fdisk_new_labelitem},
	{"new_table",                  lua_fdisk_new_table},
	{"new_partition",              lua_fdisk_new_partition},
	{"new_partition_auto",         lua_fdisk_new_partition_auto},
	{"new_iterator",               lua_fdisk_new_iterator},
	{NULL, NULL}
};

static struct lua_fdisk_meta_table*tables[]={
	&lua_fdisk_ask,
	&lua_fdisk_context,
	&lua_fdisk_field,
	&lua_fdisk_label,
	&lua_fdisk_partition,
	&lua_fdisk_parttype,
	&lua_fdisk_script,
	&lua_fdisk_table,
	&lua_fdisk_labelitem,
	&lua_fdisk_iter,
	NULL
};

static struct lua_fdisk_enum{
	const char*name;
	struct lua_fdisk_enum_item{
		const char*key;
		int64_t value;
	}*items;
}enums[]={
	{"label_type",(struct lua_fdisk_enum_item[]){
		{"DOS",FDISK_DISKLABEL_DOS},
		{"SUN",FDISK_DISKLABEL_SUN},
		{"SGI",FDISK_DISKLABEL_SGI},
		{"BSD",FDISK_DISKLABEL_BSD},
		{"GPT",FDISK_DISKLABEL_GPT},
		{NULL,0}
	}},
	{"ask_type",(struct lua_fdisk_enum_item[]){
		{"NONE",   FDISK_ASKTYPE_NONE},
		{"NUMBER", FDISK_ASKTYPE_NUMBER},
		{"OFFSET", FDISK_ASKTYPE_OFFSET},
		{"WARN",   FDISK_ASKTYPE_WARN},
		{"WARNX",  FDISK_ASKTYPE_WARNX},
		{"INFO",   FDISK_ASKTYPE_INFO},
		{"YESNO",  FDISK_ASKTYPE_YESNO},
		{"STRING", FDISK_ASKTYPE_STRING},
		{"MENU",   FDISK_ASKTYPE_MENU},
		{NULL,0}
	}},
	{"size_unit",(struct lua_fdisk_enum_item[]){
		{"HUMAN", FDISK_SIZEUNIT_HUMAN},
		{"BYTES", FDISK_SIZEUNIT_BYTES},
		{NULL,0}
	}},
	{"parttype_parser_flags",(struct lua_fdisk_enum_item[]){
		{"DATA",       FDISK_PARTTYPE_PARSE_DATA},
		{"DATALAST",   FDISK_PARTTYPE_PARSE_DATALAST},
		{"SHORTCUT",   FDISK_PARTTYPE_PARSE_SHORTCUT},
		{"ALIAS",      FDISK_PARTTYPE_PARSE_ALIAS},
		{"DEPRECATED", FDISK_PARTTYPE_PARSE_DEPRECATED},
		{"NOUNKNOWN",  FDISK_PARTTYPE_PARSE_NOUNKNOWN},
		{"SEQNUM",     FDISK_PARTTYPE_PARSE_SEQNUM},
		{"NAME",       FDISK_PARTTYPE_PARSE_NAME},
		{"DEFAULT",    FDISK_PARTTYPE_PARSE_DEFAULT},
		{NULL,0}
	}},
	{"field_type",(struct lua_fdisk_enum_item[]){
		{"NONE",      FDISK_FIELD_NONE},
		{"DEVICE",    FDISK_FIELD_DEVICE},
		{"START",     FDISK_FIELD_START},
		{"END",       FDISK_FIELD_END},
		{"SECTORS",   FDISK_FIELD_SECTORS},
		{"CYLINDERS", FDISK_FIELD_CYLINDERS},
		{"SIZE",      FDISK_FIELD_SIZE},
		{"TYPE",      FDISK_FIELD_TYPE},
		{"TYPEID",    FDISK_FIELD_TYPEID},
		{"ATTR",      FDISK_FIELD_ATTR},
		{"BOOT",      FDISK_FIELD_BOOT},
		{"BSIZE",     FDISK_FIELD_BSIZE},
		{"CPG",       FDISK_FIELD_CPG},
		{"EADDR",     FDISK_FIELD_EADDR},
		{"FSIZE",     FDISK_FIELD_FSIZE},
		{"NAME",      FDISK_FIELD_NAME},
		{"SADDR",     FDISK_FIELD_SADDR},
		{"UUID",      FDISK_FIELD_UUID},
		{"FSUUID",    FDISK_FIELD_FSUUID},
		{"FSLABEL",   FDISK_FIELD_FSLABEL},
		{"FSTYPE",    FDISK_FIELD_FSTYPE},
		{NULL,0}
	}},
	{"label_item",(struct lua_fdisk_enum_item[]){
		{"ID",                 FDISK_LABELITEM_ID},
		{"SUN_LABELID",        SUN_LABELITEM_LABELID},
		{"SUN_VTOCID",         SUN_LABELITEM_VTOCID},
		{"SUN_RPM",            SUN_LABELITEM_RPM},
		{"SUN_ACYL",           SUN_LABELITEM_ACYL},
		{"SUN_PCYL",           SUN_LABELITEM_PCYL},
		{"SUN_APC",            SUN_LABELITEM_APC},
		{"SUN_INTRLV",         SUN_LABELITEM_INTRLV},
		{"BSD_TYPE",           BSD_LABELITEM_TYPE},
		{"BSD_DISK",           BSD_LABELITEM_DISK},
		{"BSD_PACKNAME",       BSD_LABELITEM_PACKNAME},
		{"BSD_FLAGS",          BSD_LABELITEM_FLAGS},
		{"BSD_SECSIZE",        BSD_LABELITEM_SECSIZE},
		{"BSD_NTRACKS",        BSD_LABELITEM_NTRACKS},
		{"BSD_SECPERCYL",      BSD_LABELITEM_SECPERCYL},
		{"BSD_CYLINDERS",      BSD_LABELITEM_CYLINDERS},
		{"BSD_RPM",            BSD_LABELITEM_RPM},
		{"BSD_INTERLEAVE",     BSD_LABELITEM_INTERLEAVE},
		{"BSD_TRACKSKEW",      BSD_LABELITEM_TRACKSKEW},
		{"BSD_CYLINDERSKEW",   BSD_LABELITEM_CYLINDERSKEW},
		{"BSD_HEADSWITCH",     BSD_LABELITEM_HEADSWITCH},
		{"BSD_TRKSEEK",        BSD_LABELITEM_TRKSEEK},
		{"SGI_PCYLCOUNT",      SGI_LABELITEM_PCYLCOUNT},
		{"SGI_SPARECYL",       SGI_LABELITEM_SPARECYL},
		{"SGI_ILFACT",         SGI_LABELITEM_ILFACT},
		{"SGI_BOOTFILE",       SGI_LABELITEM_BOOTFILE},
		{"GPT_ID",             GPT_LABELITEM_ID},
		{"GPT_FIRSTLBA",       GPT_LABELITEM_FIRSTLBA},
		{"GPT_LASTLBA",        GPT_LABELITEM_LASTLBA},
		{"GPT_ALTLBA",         GPT_LABELITEM_ALTLBA},
		{"GPT_ENTRIESLBA",     GPT_LABELITEM_ENTRIESLBA},
		{"GPT_ENTRIESALLOC",   GPT_LABELITEM_ENTRIESALLOC},
		{"GPT_ENTRIESLASTLBA", GPT_LABELITEM_ENTRIESLASTLBA},
		{NULL,0}
	}},
	{"align",(struct lua_fdisk_enum_item[]){
		{"UP",      FDISK_ALIGN_UP},
		{"DOWN",    FDISK_ALIGN_DOWN},
		{"NEAREST", FDISK_ALIGN_NEAREST},
		{NULL,0}
	}},
	{"iter",(struct lua_fdisk_enum_item[]){
		{"FORWARD",  FDISK_ITER_FORWARD},
		{"BACKWARD", FDISK_ITER_BACKWARD},
		{NULL,0}
	}},
	{NULL,NULL}
};

LUAMOD_API int luaopen_fdisk(lua_State*L){
	struct lua_fdisk_enum*n;
	struct lua_fdisk_enum_item*t;
	lua_pushboolean(L,false);
	lua_setglobal(L,"fdisk_throw");
	for(size_t i=0;tables[i];i++)xlua_create_metatable(
		L,tables[i]->name,
		tables[i]->reg,
		tables[i]->tostring,
		tables[i]->gc
	);
	luaL_newlib(L,fdisk_lib);
	for(size_t x=0;(n=&enums[x])->name;x++){
		lua_createtable(L,0,0);
		for(size_t y=0;(t=&n->items[y])->key;y++){
			lua_pushinteger(L,t->value);
			lua_setfield(L,-2,t->key);
		}
		lua_setfield(L,-2,n->name);
	}
	return 1;
}
