/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Library/UefiLib.h>
#include<Library/PrintLib.h>
#include<Library/DebugLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Library/UefiRuntimeServicesTableLib.h>
#include<Protocol/SimpleFileSystem.h>
#include<Guid/FileInfo.h>

void dump_memory(EFI_PHYSICAL_ADDRESS base,UINT64 size){
	DEBUG((EFI_D_INFO,"Start dump memory address 0x%lx size 0x%lx",base,size));
	for(
		EFI_PHYSICAL_ADDRESS off=0;
		off<base&&off<0x100000000;
		off++
	){
		EFI_PHYSICAL_ADDRESS addr=base+off;
		if((off%32)==0)DEBUG((EFI_D_INFO,"\n  0x%016lx | ",addr));
		DEBUG((EFI_D_INFO,"%02x ",((CHAR8*)(UINTN)addr)[0]));
	}
	DEBUG((EFI_D_INFO,"\nDump Done\n"));
}

static EFI_STATUS setvar(EFI_GUID*guid,CHAR16*key,VOID*buf,UINTN size){
        return gRT->SetVariable(
		key,
		guid,
		EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS,
		size,
		buf
	);
}
static EFI_STATUS setvar_str(EFI_GUID*guid,CHAR16*key,CHAR16*str,VA_LIST va){
	CHAR16 Buffer[512];
	UnicodeVSPrint(Buffer,sizeof(Buffer),str,va);
	return setvar(guid,key,Buffer,StrSize(Buffer));
}

EFI_STATUS efi_setvar(CHAR16*key,VOID*buf,UINTN size){
        return setvar(&gSimpleInitFileGuid,key,buf,size);
}

EFI_STATUS efi_setvar_str(CHAR16*key,CHAR16*str,...){
	VA_LIST va;
	VA_START(va,str);
	EFI_STATUS r=setvar_str(&gSimpleInitFileGuid,key,str,va);
	VA_END(va);
	return r;
}

EFI_STATUS efi_setvar_int(CHAR16 *name,UINTN num){
        return efi_setvar_str(name,L"%u",num);
}

EFI_STATUS efi_file_get_info(EFI_FILE_PROTOCOL*file,EFI_GUID*guid,UINTN*size,VOID**data){
	UINTN cnt=0;
	EFI_STATUS ret;
	if(!guid||!data)return EFI_INVALID_PARAMETER;
	ret=file->GetInfo(file,guid,&cnt,*data);
	if(ret==EFI_BUFFER_TOO_SMALL){
		if(size)*size=cnt;
		if(!(*data=AllocateZeroPool(cnt)))
			return EFI_OUT_OF_RESOURCES;
		ret=file->GetInfo(file,guid,&cnt,*data);
	}
	if(EFI_ERROR(ret)&&*data){
		FreePool(*data);
		cnt=0,*data=NULL;
	}
	if(size)*size=cnt;
	return ret;
}

EFI_STATUS efi_file_get_file_info(EFI_FILE_PROTOCOL*file,UINTN*size,EFI_FILE_INFO**data){
	return efi_file_get_info(file,&gEfiFileInfoGuid,size,(VOID**)data);
}

EFI_STATUS efi_file_read(EFI_FILE_PROTOCOL*file,UINTN size,VOID**data,UINTN*read){
	UINTN rs=size;
	EFI_STATUS ret;
	if(!file||!data)return EFI_INVALID_PARAMETER;
	if(!(*data=AllocateZeroPool(size+1)))
		return EFI_OUT_OF_RESOURCES;
	if(size<=0)return EFI_SUCCESS;
	ret=file->Read(file,&rs,*data);
	if(read)*read=rs;
	if(EFI_ERROR(ret)&&*data){
		FreePool(*data);
		if(read)*read=0;
		*data=NULL;
	}
	return ret;
}

EFI_STATUS efi_file_read_dir(EFI_FILE_PROTOCOL*file,EFI_FILE_INFO**data){
	UINTN size=sizeof(EFI_FILE_INFO)+256;
	EFI_STATUS status=efi_file_read(file,size,(VOID**)data,&size);
	if(status==EFI_BUFFER_TOO_SMALL)status=efi_file_read(file,size,(VOID**)data,&size);
	if((EFI_ERROR(status)||size<=0)&&*data){
		FreePool(*data);
		*data=NULL;
	}
	return status;
}

EFI_STATUS efi_file_read_whole(EFI_FILE_PROTOCOL*file,VOID**data,UINTN*read){
	UINTN size=0;
	EFI_STATUS ret;
	EFI_FILE_INFO*info=NULL;
	if(!file||!data)return EFI_INVALID_PARAMETER;
	ret=efi_file_get_file_info(file,NULL,&info);
	if(EFI_ERROR(ret))return ret;
	if(!info)return EFI_OUT_OF_RESOURCES;
	file->SetPosition(file,0);
	if(read&&*read>0&&info->FileSize>*read){
		if(read)*read=info->FileSize;
		ret=EFI_VOLUME_FULL;
	}else if(info->Attribute&EFI_FILE_DIRECTORY){
		if(read)*read=0;
		ret=EFI_UNSUPPORTED;
	}else ret=efi_file_read(file,info->FileSize,data,&size);
	FreePool(info);
	if(!EFI_ERROR(ret)&&read)*read=size;
	file->SetPosition(file,0);
	return ret;
}
