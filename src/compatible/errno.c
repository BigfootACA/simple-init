/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#ifndef ENOMEDIUM
#define ENOMEDIUM ENXIO
#endif
#ifndef EUCLEAN
#define EUCLEAN EIO
#endif
int efi_status_to_errno(EFI_STATUS st){
	switch(st){
		case EFI_SUCCESS:
			return 0;
		case EFI_SECURITY_VIOLATION:
			return EPERM;
		case EFI_WRITE_PROTECTED:
			return EROFS;
		case EFI_ACCESS_DENIED:
			return EACCES;
		case EFI_NO_MEDIA:
			return ENOMEDIUM;
		case EFI_NO_RESPONSE:
			return ENODATA;
		case EFI_NOT_FOUND:
			return ENOENT;
		case EFI_NOT_READY:
			return EAGAIN;
		case EFI_NO_MAPPING:
			return EFAULT;
		case EFI_NOT_STARTED:
			return ESRCH;
		case EFI_MEDIA_CHANGED:
			return EBUSY;
		case EFI_BAD_BUFFER_SIZE:
		case EFI_BUFFER_TOO_SMALL:
		case EFI_INVALID_LANGUAGE:
		case EFI_INVALID_PARAMETER:
			return EINVAL;
		case EFI_OUT_OF_RESOURCES:
			return ENOMEM;
		case EFI_COMPROMISED_DATA:
		case EFI_VOLUME_CORRUPTED:
			return EUCLEAN;
		case EFI_LOAD_ERROR:
			return EBADF;
		case EFI_ICMP_ERROR:
		case EFI_TFTP_ERROR:
		case EFI_HTTP_ERROR:
		case EFI_PROTOCOL_ERROR:
			return EPROTO;
		case EFI_CRC_ERROR:
		case EFI_TIMEOUT:
			return ETIMEDOUT;
		case EFI_VOLUME_FULL:
			return ENOSPC;
		case EFI_END_OF_FILE:
		case EFI_END_OF_MEDIA:
			return ENOSTR;
		case EFI_ALREADY_STARTED:
			return EALREADY;
		case EFI_ABORTED:
			return EINTR;
		case EFI_UNSUPPORTED:
			return ENOSYS;
		case EFI_DEVICE_ERROR:
			return EIO;
		case EFI_INCOMPATIBLE_VERSION:
			return ENOTSUP;
		default:
			return 255;
	}
}
