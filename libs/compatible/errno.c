/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include"compatible.h"
#ifndef ENOMEDIUM
#define ENOMEDIUM ENXIO
#endif
#ifndef EUCLEAN
#define EUCLEAN EIO
#endif
int comp_errno=0;
RETURN_STATUS EFIerrno = RETURN_SUCCESS;
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

const char*efi_status_to_string(EFI_STATUS st){
	switch(st){
		case EFI_SUCCESS:               return "Success";
		case EFI_WARN_UNKNOWN_GLYPH:    return "Unknown Glyph";
		case EFI_WARN_DELETE_FAILURE:   return "Delete Failure";
		case EFI_WARN_WRITE_FAILURE:    return "Write Failure";
		case EFI_WARN_BUFFER_TOO_SMALL: return "Buffer Too Small";
		case EFI_WARN_STALE_DATA:       return "Stale Data";
		case EFI_LOAD_ERROR:            return "Load Error";
		case EFI_INVALID_PARAMETER:     return "Invalid Parameter";
		case EFI_UNSUPPORTED:           return "Unsupported";
		case EFI_BAD_BUFFER_SIZE:       return "Bad Buffer Size";
		case EFI_BUFFER_TOO_SMALL:      return "Buffer Too Small";
		case EFI_NOT_READY:             return "Not Ready";
		case EFI_DEVICE_ERROR:          return "Device Error";
		case EFI_WRITE_PROTECTED:       return "Write Protected";
		case EFI_OUT_OF_RESOURCES:      return "Out of Resources";
		case EFI_VOLUME_CORRUPTED:      return "Volume Corrupt";
		case EFI_VOLUME_FULL:           return "Volume Full";
		case EFI_NO_MEDIA:              return "No Media";
		case EFI_MEDIA_CHANGED:         return "Media changed";
		case EFI_NOT_FOUND:             return "Not Found";
		case EFI_ACCESS_DENIED:         return "Access Denied";
		case EFI_NO_RESPONSE:           return "No Response";
		case EFI_NO_MAPPING:            return "No mapping";
		case EFI_TIMEOUT:               return "Time out";
		case EFI_NOT_STARTED:           return "Not started";
		case EFI_ALREADY_STARTED:       return "Already started";
		case EFI_ABORTED:               return "Aborted";
		case EFI_ICMP_ERROR:            return "ICMP Error";
		case EFI_TFTP_ERROR:            return "TFTP Error";
		case EFI_PROTOCOL_ERROR:        return "Protocol Error";
		case EFI_INCOMPATIBLE_VERSION:  return "Incompatible Version";
		case EFI_SECURITY_VIOLATION:    return "Security Violation";
		case EFI_CRC_ERROR:             return "CRC Error";
		case EFI_END_OF_MEDIA:          return "End of Media";
		case EFI_END_OF_FILE:           return "End of File";
		case EFI_INVALID_LANGUAGE:      return "Invalid Language";
		case EFI_COMPROMISED_DATA:      return "Compromised Data";
		default:                        return "Unknown";
	}
}
