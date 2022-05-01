/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"internal.h"
static list*locate_cache=NULL;

static void init_locate(locate_dest*loc,const char*tag,EFI_HANDLE*hand){
	ZeroMem(loc,sizeof(locate_dest));
	AsciiStrCpyS(loc->tag,sizeof(loc->tag)-1,tag);
	loc->file_hand=hand;
	gBS->HandleProtocol(
		hand,
		&gEfiPartitionInfoProtocolGuid,
		(VOID**)&loc->part_proto
	);
	gBS->HandleProtocol(
		hand,
		&gEfiSimpleFileSystemProtocolGuid,
		(VOID**)&loc->file_proto
	);
	gBS->HandleProtocol(
		hand,
		&gEfiBlockIoProtocolGuid,
		(VOID**)&loc->block_proto
	);
	if(loc->file_proto)loc->file_proto->OpenVolume(
		loc->file_proto,
		&loc->root
	);
}

static locate_dest*new_locate(const char*tag,EFI_HANDLE*hand){
	locate_dest*loc=malloc(sizeof(locate_dest));
	if(!loc)return NULL;
	init_locate(loc,tag,hand);
	return loc;
}

static int init_default(){
	EFI_LOADED_IMAGE_PROTOCOL*li=NULL;
	EFI_STATUS st=gBS->OpenProtocol(
		gImageHandle,
		&gEfiLoadedImageProtocolGuid,
		(VOID**)&li,
		gImageHandle,
		NULL,
		EFI_OPEN_PROTOCOL_GET_PROTOCOL
	);
	if(EFI_ERROR(st)){
		tlog_error(
			"open protocol failed: %s",
			efi_status_to_string(st)
		);
		return -1;
	}
	locate_dest*loc=new_locate("default",li->DeviceHandle);
	if(!loc){
		tlog_error("create default locate failed");
		return -1;
	}
	list_obj_add_new(&locate_cache,loc);
	return 0;
}

static const char*get_match_name(enum locate_match_state st){
	switch(st){
		case MATCH_NONE:return "none";
		case MATCH_SKIP:return "skip";
		case MATCH_SUCCESS:return "success";
		case MATCH_INVALID:return "invalid";
		case MATCH_FAILED:return "failed";
		default:return "unknown";
	}
}

static bool try_protocol(const char*tag,locate_dest*loc,EFI_GUID*protocol){
	UINTN cnt=0;
	EFI_STATUS st;
	EFI_HANDLE*hands=NULL;
	CHAR16*dpt=NULL;
	char dpx[PATH_MAX],guid[64];
	EFI_DEVICE_PATH_PROTOCOL*dp=NULL;
	AsciiSPrint(guid,sizeof(guid),"%g",protocol);
	tlog_verbose("try locate protocol in guid %s",guid);
	st=gBS->LocateHandleBuffer(
		ByProtocol,
		protocol,
		NULL,&cnt,&hands
	);
	if(EFI_ERROR(st)){
		tlog_warn(
			"locate protocol %s failed: %s",
			guid,efi_status_to_string(st)
		);
		return false;
	}
	tlog_debug(
		"found %llu handles in protocol %s",
		(unsigned long long)cnt,guid
	);
	for(UINTN i=0;i<cnt;i++){
		init_locate(loc,tag,hands[i]);
		tlog_verbose(
			"try match handle %p (%llu)",
			hands[i],(unsigned long long)i
		);
		for(UINTN s=0;locate_matches[s];s++){
			enum locate_match_state st=locate_matches[s](loc);
			if(st!=MATCH_SKIP)tlog_verbose(
				"handle %p (%llu) match %llu state %s",
				hands[i],(unsigned long long)i,
				(unsigned long long)s,get_match_name(st)
			);
			switch(st){
				case MATCH_NONE:
				case MATCH_SKIP:
				case MATCH_SUCCESS:continue;
				case MATCH_INVALID:return false;
				case MATCH_FAILED:goto match_fail;
			}
		}
		ZeroMem(dpx,sizeof(dpx));
		if(
			(dp=DevicePathFromHandle(hands[i]))&&
			(dpt=ConvertDevicePathToText(dp,TRUE,FALSE))
		){
			UnicodeStrToAsciiStrS(dpt,dpx,sizeof(dpx)-1);
			FreePool(dpt);
		}
		if(!dpx[0])AsciiStrCpyS(dpx,sizeof(dpx)-1,"(Unknown)");
		tlog_info("found locate %s",dpx);
		return true;
		match_fail:;
	}
	return false;
}

static locate_dest*load_locate(const char*tag){
	locate_dest*loc=AllocatePool(sizeof(locate_dest));
	if(!loc)return NULL;
	if(try_protocol(tag,loc,&gEfiPartitionInfoProtocolGuid))return loc;
	if(try_protocol(tag,loc,&gEfiSimpleFileSystemProtocolGuid))return loc;
	if(try_protocol(tag,loc,&gEfiBlockIoProtocolGuid))return loc;
	if(loc)FreePool(loc);
	return NULL;

}

static bool locate_cmp(list*l,void*d){
	LIST_DATA_DECLARE(x,l,locate_dest*);
	return d&&x&&AsciiStrCmp(x->tag,(char*)d)==0;
}

static locate_dest*get_locate(const char*tag){
	list*l;
	if(!tag||!*tag)return NULL;
	if(!locate_cache)init_default();
	if((l=list_search_one(locate_cache,locate_cmp,(void*)tag)))
		return LIST_DATA(l,locate_dest*);
	if(confd_get_type_base(BASE,tag)!=TYPE_KEY)
		return NULL;
	locate_dest*loc=load_locate(tag);
	if(loc)list_obj_add_new(&locate_cache,loc);
	return loc;
}

static void dump_block_info(EFI_BLOCK_IO_PROTOCOL*bi){
	char buf[64];
	if(!bi||!bi->Media)return;
	EFI_BLOCK_IO_MEDIA*m=bi->Media;
	UINT64 size=m->BlockSize*(m->LastBlock+1);
	tlog_verbose("  block:");
	tlog_verbose("    revision:      0x%llx",(unsigned long long)bi->Revision);
	tlog_verbose("    media id:      %d",m->MediaId);
	tlog_verbose("    read only:     %s",BOOL2STR(m->ReadOnly));
	tlog_verbose("    sector size:   %d",m->BlockSize);
	tlog_verbose("    total sectors: %lld",m->LastBlock+1);
	tlog_verbose(
		"    total size:    %lld (%s)",size,
		make_readable_str_buf(buf,sizeof(buf),size,1,0)
	);
}

static void dump_fs_info(EFI_FILE_PROTOCOL*root){
	EFI_STATUS st;
	EFI_FILE_SYSTEM_INFO*fi=NULL;
	char buf[64],xn[PATH_MAX];
	if(!root)return;
	st=efi_file_get_info(root,&gEfiFileSystemInfoGuid,NULL,(VOID**)&fi);
	if(EFI_ERROR(st))return;
	ZeroMem(xn,sizeof(xn));
	UnicodeStrToAsciiStrS(fi->VolumeLabel,xn,sizeof(xn));
	tlog_verbose("  file system:");
	tlog_verbose("    revision:     0x%llx",(unsigned long long)root->Revision);
	tlog_verbose("    sector size:  %d",fi->BlockSize);
	tlog_verbose("    read only:    %s",BOOL2STR(fi->ReadOnly));
	tlog_verbose("    volume label: %s",xn[0]?xn:"(none)");
	tlog_verbose(
		"    total size:   %lld (%s)",fi->VolumeSize,
		make_readable_str_buf(buf,sizeof(buf),fi->VolumeSize,1,0)
	);
	tlog_verbose(
		"    free space:   %lld (%s)",fi->FreeSpace,
		make_readable_str_buf(buf,sizeof(buf),fi->FreeSpace,1,0)
	);
}

static void dump_part_gpt(EFI_PARTITION_ENTRY*gpt){
	char xn[PATH_MAX],guid[64];
	if(!gpt)return;
	tlog_verbose("    gpt disk label");
	ZeroMem(xn,sizeof(xn));
	UnicodeStrToAsciiStrS(gpt->PartitionName,xn,sizeof(xn));
	tlog_verbose("      partition name: %s",xn[0]?xn:"(none)");
	ZeroMem(guid,sizeof(guid));
	AsciiSPrint(guid,sizeof(guid),"%g",&gpt->PartitionTypeGUID);
	tlog_verbose("      type guid:      %s",guid);
	ZeroMem(guid,sizeof(guid));
	AsciiSPrint(guid,sizeof(guid),"%g",&gpt->UniquePartitionGUID);
	tlog_verbose("      partition guid: %s",guid);
	tlog_verbose("      attributes:     %llx",(unsigned long long)gpt->Attributes);
	tlog_verbose("      starting lba:   %llx",(unsigned long long)gpt->StartingLBA);
	tlog_verbose("      ending lba:     %llx",(unsigned long long)gpt->EndingLBA);
}

static void dump_part_mbr(MBR_PARTITION_RECORD*mbr){
	if(!mbr)return;
	tlog_verbose("    mbr disk label");
	tlog_verbose("      type:           0x%02X",mbr->OSIndicator);
	tlog_verbose("      activated:      %s",BOOL2STR(mbr->BootIndicator==0x80));
	tlog_verbose("      start head:     %llx",(unsigned long long)mbr->StartHead);
	tlog_verbose("      start sector:   %llx",(unsigned long long)mbr->StartSector);
	tlog_verbose("      start track:    %llx",(unsigned long long)mbr->StartTrack);
	tlog_verbose("      end head:       %llx",(unsigned long long)mbr->EndHead);
	tlog_verbose("      end sector:     %llx",(unsigned long long)mbr->EndSector);
	tlog_verbose("      end track:      %llx",(unsigned long long)mbr->EndTrack);
}

static void dump_part_info(EFI_PARTITION_INFO_PROTOCOL*pi){
	if(!pi)return;
	tlog_verbose("  partition info:");
	tlog_verbose("    revision:     0x%llx",(unsigned long long)pi->Revision);
	tlog_verbose("    efi system:   %s",BOOL2STR(pi->System==1));
	switch(pi->Type){
		case PARTITION_TYPE_GPT:dump_part_gpt(&pi->Info.Gpt);break;
		case PARTITION_TYPE_MBR:dump_part_mbr(&pi->Info.Mbr);break;
		default:tlog_verbose("    disk label:   %u",pi->Type);
	}
}

static void dump_locate(locate_dest*loc){
	if(!loc||loc->dump)return;
	tlog_verbose("locate %s info:",loc->tag);
	if(loc->root)dump_fs_info(loc->root);
	if(loc->block_proto)dump_block_info(loc->block_proto);
	if(loc->part_proto)dump_part_info(loc->part_proto);
	loc->dump=true;
}

static locate_dest*init_locate_ret(locate_ret*ret,const char*file){
	locate_dest*loc;
	char buf[PATH_MAX],*tag=NULL,*path=NULL;
	if(!ret||!file||!*file)return NULL;
	ZeroMem(ret,sizeof(locate_ret));
	ZeroMem(buf,sizeof(buf));
	AsciiStrCpyS(buf,sizeof(buf)-1,file);
	ret->type=LOCATE_FILE;
	if(buf[0]=='@'){
		tag=buf+1;
		char*d=strchr(buf,':');
		if(d)*d=0,path=d+1;
	}else if(buf[0]=='#'){
		tag=buf+1;
		tlog_info("tag %s",tag);
		ret->type=LOCATE_BLOCK;
	}else path=buf,tag="default";
	AsciiStrCpyS(
		ret->tag,
		sizeof(ret->tag)-1,
		tag
	);
	if(path)AsciiStrCpyS(
		ret->path,
		sizeof(ret->path)-1,
		path
	);
	if(!(loc=get_locate(tag)))return NULL;
	dump_locate(loc);
	switch(ret->type){
		case LOCATE_FILE:
			ret->hand=loc->file_hand;
			ret->fs=loc->file_proto;
			ret->root=loc->root;
		break;
		case LOCATE_BLOCK:
			ret->block=loc->block_proto;
		break;
		default:;
	}
	ret->part=loc->part_proto;
	return loc;
}

static bool locate_open_file(locate_dest*loc,locate_ret*ret,UINT64 mode,bool quiet){
	EFI_STATUS st;
	if(!ret->path[0]){
		if(!quiet)tlog_warn("no path specified");
		return false;
	}
	if(!loc->root){
		if(!quiet)tlog_warn(
			"locate '%s' have no file system",
			ret->tag
		);
		return false;
	}
	ZeroMem(ret->path16,sizeof(ret->path16));
	AsciiStrToUnicodeStrS(
		ret->path,ret->path16,
		sizeof(ret->path16)/sizeof(CHAR16)
	);
	st=loc->root->Open(loc->root,&ret->file,ret->path16,mode,0);
	if(EFI_ERROR(st)){
		if(!quiet)tlog_error(
			"open file '%s' failed: %s",
			ret->path,efi_status_to_string(st)
		);
		return false;
	}
	if(!ret->file){
		if(!quiet)tlog_error(
			"open file '%s' failed",
			ret->path
		);
		return false;
	}
	if(!quiet)tlog_debug("open file '%s' as %p",ret->path,ret->file);
	if(!(ret->device=FileDevicePath(loc->file_hand,ret->path16))){
		if(!quiet)tlog_error(
			"get file '%s' device path failed",
			ret->path
		);
		return false;
	}
	return true;
}

static bool locate_open_block(locate_dest*loc,locate_ret*ret,bool quiet){
	if(!(ret->device=DevicePathFromHandle(loc->file_hand))){
		if(!quiet)tlog_error(
			"get block '%s' device path failed",
			ret->tag
		);
		return false;
	}
	return true;
}

static bool _boot_locate(locate_ret*ret,const char*file,bool quiet){
	locate_dest*loc=init_locate_ret(ret,file);
	if(loc)switch(ret->type){
		case LOCATE_FILE:return locate_open_file(loc,ret,EFI_FILE_MODE_READ,quiet);
		case LOCATE_BLOCK:return locate_open_block(loc,ret,quiet);
		default:;
	}
	return false;
}

bool boot_locate(locate_ret*ret,const char*file){
	return _boot_locate(ret,file,false);
}

bool boot_locate_quiet(locate_ret*ret,const char*file){
	return _boot_locate(ret,file,true);
}

bool boot_locate_create_file(locate_ret*ret,const char*file){
	locate_dest*loc=init_locate_ret(ret,file);
	return loc&&ret->type==LOCATE_FILE&&locate_open_file(loc,ret,
		EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE,false
	);
}

char*locate_find_name(char*buf,size_t len){
	CHAR8 name[255];
	ZeroMem(buf,len);
	for(int i=0;i<4096;i++){
		ZeroMem(name,sizeof(name));
		AsciiSPrint(name,sizeof(name),"auto-%d",i);
		if(confd_get_type_base("locates",name)==TYPE_KEY)continue;
		AsciiStrCpyS(buf,len-1,name);
		return buf;
	}
	tlog_warn("no available locate name found");
	return NULL;
}

bool locate_add_by_device_path(char*tag,bool save,EFI_DEVICE_PATH_PROTOCOL*dp){
	bool ret=false;
	CHAR16*dpt=NULL;
	UINTN xps=PATH_MAX;
	CHAR8*xpt=NULL;
	if(!tag||!dp||get_locate(tag))return ret;
	if(!(dpt=ConvertDevicePathToText(dp,FALSE,FALSE)))return ret;
	confd_add_key_base(BASE,tag);
	confd_set_save_base(BASE,tag,save);
	if((xpt=AllocateZeroPool(xps))){
		UnicodeStrToAsciiStrS(dpt,xpt,sizeof(xpt));
		confd_set_string_dict(BASE,tag,"by_device_path",xpt);
		FreePool(xpt);
		ret=true;
	}
	FreePool(dpt);
	return ret;
}

bool locate_auto_add_by_device_path(char*buf,size_t len,EFI_DEVICE_PATH_PROTOCOL*dp){
	return locate_add_by_device_path(locate_find_name(buf,len),false,dp);
}
