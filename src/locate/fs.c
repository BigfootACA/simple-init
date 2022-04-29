/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"internal.h"

locate_match_state locate_match_fs_name(locate_dest*loc){
	EFI_STATUS st;
	UINTN bs=0,vs=0;
	char label[256];
	locate_match_state res=MATCH_FAILED;
	EFI_FILE_SYSTEM_VOLUME_LABEL*fi=NULL;
	char*name=GSTR("by_fs_label",NULL);
	if(!name)return MATCH_SKIP;
	if(!*name||strlen(name)>255)
		EDONE(tlog_warn("invalid file system label"));
	if(!loc||!loc->root)return MATCH_FAILED;
	st=loc->root->GetInfo(
		loc->root,
		&gEfiFileSystemVolumeLabelInfoIdGuid,
		&bs,fi
	);
	if(st==EFI_BUFFER_TOO_SMALL){
		if(!(fi=AllocateZeroPool(bs)))
			EDONE(tlog_warn("allocate memory failed"));
		st=loc->root->GetInfo(
			loc->root,
			&gEfiFileSystemVolumeLabelInfoIdGuid,
			&bs,fi
		);
	}
	if(EFI_ERROR(st)||!fi)EDONE(tlog_warn(
		"get file system volume label failed: %s",
		efi_status_to_string(st)
	));
	vs=StrLen(fi->VolumeLabel);
	if(vs<=0||vs>255)goto done;
	UnicodeStrToAsciiStrS(
		fi->VolumeLabel,
		label,
		sizeof(label)
	);
	if(AsciiStriCmp(label,name)!=0)goto done;
	res=MATCH_SUCCESS;
	done:
	if(name)free(name);
	if(fi)FreePool(fi);
	return res;
}

locate_match_state locate_match_file(locate_dest*loc){
	EFI_STATUS st;
	CHAR16*fn=NULL;
	UINTN fs=PATH_MAX*sizeof(CHAR16);
	EFI_FILE_PROTOCOL*f=NULL;
	char*name=GSTR("by_file",NULL);
	if(!name)return MATCH_SKIP;
	if(!*name||AsciiStrLen(name)>PATH_MAX-1){
		tlog_warn("invalid file name");
		free(name);
		return MATCH_INVALID;
	}
	if(!loc||!loc->root)return MATCH_FAILED;
	if(!(fn=AllocateZeroPool(fs)))return MATCH_FAILED;
	AsciiStrToUnicodeStrS(
		name,fn,
		fs/sizeof(CHAR16)
	);
	free(name);
	st=loc->root->Open(
		loc->root,
		&f,fn,
		EFI_FILE_MODE_READ,
		0
	);
	FreePool(fn);
	if(EFI_ERROR(st)||!f)return MATCH_FAILED;
	f->Close(f);
	return MATCH_SUCCESS;
}
