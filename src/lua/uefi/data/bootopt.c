/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_LUA
#include"../lua_uefi.h"

static int LuaUefiBootOptionToString(lua_State*L){
	GET_BOOT_OPTION(L,1,bo);
	lua_pushfstring(L,LUA_UEFI_BOOT_OPTION" %p",bo->bo);
	return 1;
}

static int LuaUefiBootOptionFilePath(lua_State*L){
	bool found=false;
	GET_BOOT_OPTION(L,1,bo);
	OPT_DEVICE_PATH(L,2,dp);
	if(bo->bo){
		if(dp){
			luaL_argcheck(L,bo->allocated,1,"read only option");
			luaL_argcheck(L,dp->dp!=NULL,2,"Device Path must not null");
			if(bo->bo->FilePath)FreePool(bo->bo->FilePath);
			bo->bo->FilePath=DuplicateDevicePath(dp->dp);
		}
		if(bo->bo->FilePath){
			uefi_device_path_to_lua(L,bo->bo->FilePath);
			found=true;
		}
	}
	if(!found)lua_pushnil(L);
	return 1;
}

static int LuaUefiBootOptionOptionalData(lua_State*L){
	size_t ds=0;
	void*data=NULL;
	bool found=false;
	GET_BOOT_OPTION(L,1,bo);
	lua_arg_get_data(L,2,true,&data,&ds);
	if(bo->bo){
		if(data){
			luaL_argcheck(L,bo->allocated,1,"read only option");
			luaL_argcheck(L,ds>0,2,"data must not null");
			if(bo->bo->OptionalData)FreePool(bo->bo->OptionalData);
			bo->bo->OptionalData=AllocateCopyPool(ds,data);
			if(!bo->bo->OptionalData)return luaL_error(L,"allocate for data failed");
		}
		if(
			bo->bo->OptionalData&&
			bo->bo->OptionalDataSize>0&&
			bo->bo->OptionalDataSize!=(UINT32)-1
		){
			uefi_data_to_lua(
				L,FALSE,
				bo->bo->OptionalData,
				bo->bo->OptionalDataSize
			);
			found=true;
		}
	}
	if(!found)lua_pushnil(L);
	return 1;
}

static int LuaUefiBootOptionDescription(lua_State*L){
	CHAR16*c16=NULL;
	bool found=false;
	GET_BOOT_OPTION(L,1,bo);
	lua_arg_get_char16(L,2,true,&c16);
	if(bo->bo){
		if(c16){
			luaL_argcheck(L,bo->allocated,1,"read only option");
			if(bo->bo->Description)FreePool(bo->bo->Description);
			bo->bo->Description=c16;
		}
		if(bo->bo->Description){
			uefi_char16_16_to_lua(L,FALSE,bo->bo->Description);
			found=true;
		}
	}else if(c16)FreePool(c16);
	if(!found)lua_pushnil(L);
	return 1;
}

static int LuaUefiBootOptionOptionNumber(lua_State*L){
	GET_BOOT_OPTION(L,1,bo);
	if(bo->bo){
		if(!lua_isnoneornil(L,2)){
			luaL_argcheck(L,bo->allocated,1,"read only option");
			bo->bo->OptionNumber=lua_tointeger(L,2);
		}
		lua_pushinteger(L,bo->bo->OptionNumber);
	}else lua_pushnil(L);
	return 1;
}

static int LuaUefiBootOptionVendorGuid(lua_State*L){
	GET_BOOT_OPTION(L,1,bo);
	if(bo->bo){
		luaL_argcheck(L,bo->allocated,1,"read only option");
		lua_arg_get_guid(L,2,true,&bo->bo->VendorGuid);
		uefi_guid_to_lua(L,&bo->bo->VendorGuid);
	}else lua_pushnil(L);
	return 1;
}

static int LuaUefiBootOptionAttributes(lua_State*L){
	GET_BOOT_OPTION(L,1,bo);
	if(bo->bo){
		if(!lua_isnoneornil(L,2)){
			luaL_argcheck(L,bo->allocated,1,"read only option");
			bo->bo->Attributes=lua_tointeger(L,2);
		}
		lua_pushinteger(L,bo->bo->Attributes);
	}else lua_pushnil(L);
	return 1;
}

static int LuaUefiBootOptionOptionType(lua_State*L){
	GET_BOOT_OPTION(L,1,bo);
	if(bo->bo){
		if(!lua_isnoneornil(L,2)){
			luaL_argcheck(L,bo->allocated,1,"read only option");
			if(!uefi_str_to_load_option_type(
				lua_tostring(L,2),&bo->bo->OptionType
			))return luaL_argerror(L,2,"invalid load option type");
		}
		lua_pushstring(L,uefi_load_option_type_to_str(bo->bo->OptionType));
	}else lua_pushnil(L);
	return 1;
}

static int LuaUefiBootOptionStatus(lua_State*L){
	GET_BOOT_OPTION(L,1,bo);
	if(!bo->bo)lua_pushnil(L);
	else uefi_status_to_lua(L,bo->bo->Status);
	return 1;
}

static int LuaUefiBootOptionExitData(lua_State*L){
	GET_BOOT_OPTION(L,1,bo);
	if(bo->bo&&bo->bo->ExitData&&bo->bo->ExitDataSize>0)
		uefi_data_dup_to_lua(L,bo->bo->ExitData,bo->bo->ExitDataSize);
	else lua_pushnil(L);
	return 1;
}

static int LuaUefiBootOptionGarbageCollect(lua_State*L){
	GET_BOOT_OPTION(L,1,bo);
	if(bo->allocated&&bo->bo)EfiBootManagerFreeLoadOption(bo->bo);
	bo->allocated=false,bo->bo=NULL;
	return 0;
}

void uefi_bootopt_to_lua(lua_State*L,EFI_BOOT_MANAGER_LOAD_OPTION*opt,BOOLEAN allocated){
	struct lua_uefi_bootopt_data*e;
	if(!opt){
		lua_pushnil(L);
		return;
	}
	e=lua_newuserdata(L,sizeof(struct lua_uefi_bootopt_data));
	luaL_getmetatable(L,LUA_UEFI_BOOT_OPTION);
	lua_setmetatable(L,-2);
	e->allocated=allocated;
	e->bo=opt;
}

struct lua_uefi_meta_table LuaUefiBootOptionMetaTable={
	.name=LUA_UEFI_BOOT_OPTION,
	.reg=(const luaL_Reg[]){
		{"OptionNumber",    LuaUefiBootOptionOptionNumber},
		{"GetOptionNumber", LuaUefiBootOptionOptionNumber},
		{"SetOptionNumber", LuaUefiBootOptionOptionNumber},
		{"OptionType",      LuaUefiBootOptionOptionType},
		{"GetOptionType",   LuaUefiBootOptionOptionType},
		{"SetOptionType",   LuaUefiBootOptionOptionType},
		{"Attributes",      LuaUefiBootOptionAttributes},
		{"GetAttributes",   LuaUefiBootOptionAttributes},
		{"SetAttributes",   LuaUefiBootOptionAttributes},
		{"Description",     LuaUefiBootOptionDescription},
		{"GetDescription",  LuaUefiBootOptionDescription},
		{"SetDescription",  LuaUefiBootOptionDescription},
		{"FilePath",        LuaUefiBootOptionFilePath},
		{"GetFilePath",     LuaUefiBootOptionFilePath},
		{"SetFilePath",     LuaUefiBootOptionFilePath},
		{"OptionalData",    LuaUefiBootOptionOptionalData},
		{"GetOptionalData", LuaUefiBootOptionOptionalData},
		{"SetOptionalData", LuaUefiBootOptionOptionalData},
		{"VendorGuid",      LuaUefiBootOptionVendorGuid},
		{"GetVendorGuid",   LuaUefiBootOptionVendorGuid},
		{"SetVendorGuid",   LuaUefiBootOptionVendorGuid},
		{"Status",          LuaUefiBootOptionStatus},
		{"GetStatus",       LuaUefiBootOptionStatus},
		{"ExitData",        LuaUefiBootOptionExitData},
		{"GetExitData",     LuaUefiBootOptionExitData},
		{NULL, NULL}
	},
	.tostring=LuaUefiBootOptionToString,
	.gc=LuaUefiBootOptionGarbageCollect,
};

#endif
