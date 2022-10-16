/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<Uefi.h>
#include<Library/BaseLib.h>
#include<Library/PrintLib.h>
#include<Library/DevicePathLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Protocol/DiskIo.h>
#include<Protocol/DiskIo2.h>
#include<Protocol/BlockIo.h>
#include<Protocol/BlockIo2.h>
#include<Protocol/DevicePath.h>
#include<Protocol/LoadedImage.h>
#include<Protocol/PartitionInfo.h>
#include<Protocol/SimpleFileSystem.h>
#include<Guid/FileInfo.h>
#include<Guid/FileSystemInfo.h>
#include"../fs_internal.h"
#include"uefi.h"

struct vol_data{
	BOOLEAN found;
	EFI_HANDLE*hand;
};

static int fs_volume_update_core(fsvol_private_info*info){
	CHAR16*txt=NULL;
	struct vol_data*d;
	struct fsvol_info_fs*pfi;
	struct fsvol_info_part*ppi;
	EFI_FILE_PROTOCOL*fp=NULL;
	EFI_FILE_SYSTEM_INFO*si=NULL;
	EFI_BLOCK_IO_PROTOCOL*block=NULL;
	EFI_DEVICE_PATH_PROTOCOL*dp=NULL;
	EFI_PARTITION_INFO_PROTOCOL*pi=NULL;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*fs=NULL;
	if(!info||!(d=info->data))RET(EINVAL);
	pfi=&info->info.fs,ppi=&info->info.part;
	if(!d->hand)RET(EBADF);
	info->info.features=0;
	gBS->HandleProtocol(d->hand,&gEfiBlockIoProtocolGuid,(VOID**)&block);
	gBS->HandleProtocol(d->hand,&gEfiPartitionInfoProtocolGuid,(VOID**)&pi);
	gBS->HandleProtocol(d->hand,&gEfiSimpleFileSystemProtocolGuid,(VOID**)&fs);
	if(fs){
		fs->OpenVolume(fs,&fp);
		if(fp)efi_file_get_info(
			fp,&gEfiFileSystemInfoGuid,
			NULL,(VOID**)&si
		);
	}
	memset(info->info.name,0,sizeof(info->info.name));
	memset(info->info.title,0,sizeof(info->info.title));
	if(si){
		info->info.features|=FSVOL_FILES;
		pfi->size=si->VolumeSize;
		pfi->avail=si->FreeSpace;
		pfi->used=pfi->size-pfi->avail;
		if(si->ReadOnly)info->info.features|=FSVOL_READONLY;
		UnicodeStrToAsciiStrS(si->VolumeLabel,pfi->label,sizeof(pfi->label));
		if(pfi->label[0]){
			if(!info->info.name[0])strncpy(
				info->info.name,pfi->label,
				sizeof(info->info.name)
			);
			if(!info->info.title[0])strncpy(
				info->info.title,pfi->label,
				sizeof(info->info.title)
			);
		}
	}
	if(block&&block->Media){
		ppi->sector_size=block->Media->BlockSize;
		ppi->sector_count=block->Media->LastBlock+1;
		ppi->size=ppi->sector_size*ppi->sector_count;
		if(block->Media->LogicalPartition)
			info->info.features|=FSVOL_PARTITION;
		if(block->Media->RemovableMedia)
			info->info.features|=FSVOL_REMOVABLE;
		if(block->Media->ReadOnly)
			info->info.features|=FSVOL_READONLY;
	}else if(si){
		ppi->size=si->VolumeSize;
		ppi->sector_size=si->BlockSize;
		ppi->sector_count=si->VolumeSize/si->BlockSize;
	}
	if(pi){
		if(pi->System==1)info->info.features|=FSVOL_SYSTEM;
		switch(pi->Type){
			case PARTITION_TYPE_GPT:
				AsciiSPrint(
					ppi->uuid,sizeof(ppi->uuid),"%g",
					&pi->Info.Gpt.UniquePartitionGUID
				);
				UnicodeStrToAsciiStrS(
					pi->Info.Gpt.PartitionName,
					ppi->label,sizeof(ppi->label)
				);
				if(ppi->label[0]){
					if(!info->info.name[0])strncpy(
						info->info.name,pfi->label,
						sizeof(info->info.name)
					);
					if(!info->info.title[0])strncpy(
						info->info.title,pfi->label,
						sizeof(info->info.title)
					);
				}
			break;
			case PARTITION_TYPE_MBR:
				if(pi->Info.Mbr.BootIndicator==0x80)
					info->info.features|=FSVOL_BOOTABLE;
			break;
		}
	}
	if(
		(!info->info.name[0]||!info->info.title[0])&&
		(dp=DevicePathFromHandle(d->hand))&&
		(txt=ConvertDevicePathToText(dp,TRUE,TRUE))
	){
		if(!info->info.name[0])UnicodeStrToAsciiStrS(
			txt,info->info.name,
			sizeof(info->info.name)
		);
		if(!info->info.title[0])UnicodeStrToAsciiStrS(
			txt,info->info.title,
			sizeof(info->info.title)
		);
		FreePool(txt);
	}
	if(si)FreePool(si);
	if(fp)fp->Close(fp);
	RET(0);
}

static int fs_volume_update(const fsvol*vol,fsvol_private_info*info){
	if(!info||!vol||info->vol!=vol)RET(EINVAL);
	return fs_volume_update_core(info);
}

static int scan_handle(const fsvol*vol,EFI_HANDLE*hand){
	int r;
	char id[64];
	struct vol_data*d;
	fsvol_private_info*info;
	if(!vol||!hand)RET(EINVAL);
	memset(id,0,sizeof(id));
	snprintf(id,sizeof(id)-1,"uefi-%x",(intptr_t)hand);
	if(
		(info=fsvolp_lookup_by_id(id))||
		(info=fsvolp_lookup_by_fsid(2,(intptr_t)hand))
	){
		if((d=info->data))d->found=TRUE;
		RET(EEXIST);
	}
	if(!(info=fsvol_info_new(
		vol,(void**)&d,
		id,NULL,NULL,0
	)))RET(ENOMEM);
	d->found=TRUE,d->hand=hand;
	info->info.fsid.id1=2;
	info->info.fsid.id2=(intptr_t)hand;
	r=fs_volume_update_core(info);
	r=fsvol_add_volume(info);
	if(r!=0){
		fsvol_info_delete(info);
		RET(ENOMEM);
	}
	RET(r);
}

static int scan_by_guid(const fsvol*vol,EFI_GUID*guid){
	UINTN cnt=0,i=0;
	EFI_HANDLE*hands=NULL;
	EFI_STATUS st=EFI_SUCCESS;
	if(!vol||!guid)RET(EINVAL);
	st=gBS->LocateHandleBuffer(
		ByProtocol,guid,
		NULL,&cnt,&hands
	);
	if(EFI_ERROR(st))RET(efi_status_to_errno(st));
	if(cnt<=0||!hands)RET(ENOENT);
	for(i=0;i<cnt;i++)scan_handle(vol,hands[i]);
	RET(0);
}

static int fs_volume_scan(const fsvol*vol){
	struct vol_data*d;
	fsvol_private_info*info,**infos;
	if((infos=fsvolp_get_by_driver_name(vol->name))){
		for(size_t i=0;(info=infos[i]);i++)
			if((d=info->data))d->found=FALSE;
		free(infos);
	}
	scan_by_guid(vol,&gEfiPartitionInfoProtocolGuid);
	scan_by_guid(vol,&gEfiBlockIoProtocolGuid);
	scan_by_guid(vol,&gEfiDiskIoProtocolGuid);
	scan_by_guid(vol,&gEfiDiskIo2ProtocolGuid);
	scan_by_guid(vol,&gEfiBlockIo2ProtocolGuid);
	scan_by_guid(vol,&gEfiSimpleFileSystemProtocolGuid);
	if((infos=fsvolp_get_by_driver_name(vol->name))){
		for(size_t i=0;(info=infos[i]);i++)
			if((d=info->data)&&!d->found)
				fsvol_info_delete(info);
		free(infos);
	}
	RET(0);
}

static int fs_volume_open(
	const fsvol*vol,
	fsvol_private_info*info,
	fsh**hand
){
	url*u=NULL;
	UINTN len=0;
	CHAR8*dpt=NULL;
	CHAR16*txt=NULL;
	struct vol_data*d;
	EFI_DEVICE_PATH_PROTOCOL*dp=NULL;
	if(!info||!hand)RET(EINVAL);
	if(!vol||info->vol!=vol)RET(EINVAL);
	if(!fs_has_vol_feature(info->info.features,FSVOL_FILES))RET(ENOTSUP);
	if(!(d=info->data)||!d->hand)RET(EBADF);
	if(!(dp=DevicePathFromHandle(d->hand)))RET(ENOENT);
	if(!(txt=ConvertDevicePathToText(dp,TRUE,TRUE)))DONE(ENOMEM);
	if(!(dpt=AllocateZeroPool(len=StrLen(txt)+1)))DONE(ENOMEM);
	if(!(u=url_new()))DONE(ENOMEM);
	UnicodeStrToAsciiStrS(txt,dpt,len);
	url_set_host(u,dpt,len);
	url_set_scheme(u,"uefi",0);
	url_set_path(u,"/",0);
	errno=fs_open_uri(hand,u,FILE_FLAG_FOLDER);
	done:
	if(u)url_free(u);
	if(dpt)FreePool(dpt);
	if(txt)FreePool(txt);
	return errno;
}

static fsvol vol_uefi={
	.magic=FS_VOLUME_MAGIC,
	.name="uefi",
	.info_data_size=sizeof(struct vol_data),
	.update=fs_volume_update,
	.scan=fs_volume_scan,
	.open=fs_volume_open,
};

void fsvol_register_uefi(bool deinit){
	if(!deinit)fsvol_register_dup(&vol_uefi);
}
