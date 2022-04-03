/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include<errno.h>
#include<Library/BaseLib.h>
#include"compatible.h"
#ifndef ENOMEDIUM
#define ENOMEDIUM ENXIO
#endif
#ifndef EUCLEAN
#define EUCLEAN EIO
#endif
weak_decl int errno=0;
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

const char*efi_status_to_short_string(EFI_STATUS st){
	switch(st){
		case EFI_SUCCESS:               return "success";
		case EFI_WARN_UNKNOWN_GLYPH:    return "warn-unknown-glyph";
		case EFI_WARN_DELETE_FAILURE:   return "warn-delete-failure";
		case EFI_WARN_WRITE_FAILURE:    return "warn-write-failure";
		case EFI_WARN_BUFFER_TOO_SMALL: return "warn-buffer-too-small";
		case EFI_WARN_STALE_DATA:       return "warn-stale-data";
		case EFI_LOAD_ERROR:            return "load-error";
		case EFI_INVALID_PARAMETER:     return "invalid-parameter";
		case EFI_UNSUPPORTED:           return "unsupported";
		case EFI_BAD_BUFFER_SIZE:       return "bad-buffer-size";
		case EFI_BUFFER_TOO_SMALL:      return "buffer-too-small";
		case EFI_NOT_READY:             return "not-ready";
		case EFI_DEVICE_ERROR:          return "device-error";
		case EFI_WRITE_PROTECTED:       return "write-protected";
		case EFI_OUT_OF_RESOURCES:      return "out-of-resources";
		case EFI_VOLUME_CORRUPTED:      return "volume-corrupted";
		case EFI_VOLUME_FULL:           return "volume-full";
		case EFI_NO_MEDIA:              return "no-media";
		case EFI_MEDIA_CHANGED:         return "media-changed";
		case EFI_NOT_FOUND:             return "not-found";
		case EFI_ACCESS_DENIED:         return "access-denied";
		case EFI_NO_RESPONSE:           return "no-response";
		case EFI_NO_MAPPING:            return "no-mapping";
		case EFI_TIMEOUT:               return "timeout";
		case EFI_NOT_STARTED:           return "not-started";
		case EFI_ALREADY_STARTED:       return "already-started";
		case EFI_ABORTED:               return "aborted";
		case EFI_ICMP_ERROR:            return "icmp-error";
		case EFI_TFTP_ERROR:            return "tftp-error";
		case EFI_PROTOCOL_ERROR:        return "protocol-error";
		case EFI_INCOMPATIBLE_VERSION:  return "incompatible-version";
		case EFI_SECURITY_VIOLATION:    return "security-violation";
		case EFI_CRC_ERROR:             return "crc-error";
		case EFI_END_OF_MEDIA:          return "end-of-media";
		case EFI_END_OF_FILE:           return "end-of-file";
		case EFI_INVALID_LANGUAGE:      return "invalid-language";
		case EFI_COMPROMISED_DATA:      return "compromised-data";
		default:                        return "unknown";
	}
}

BOOLEAN efi_short_string_to_status(const char*str,EFI_STATUS*st){
	if(!str||!st)return FALSE;
	if(AsciiStriCmp(str,"success")==0)
		*st=EFI_SUCCESS;
	else if(AsciiStriCmp(str,"warn-unknown-glyph")==0)
		*st=EFI_WARN_UNKNOWN_GLYPH;
	else if(AsciiStriCmp(str,"warn-delete-failure")==0)
		*st=EFI_WARN_DELETE_FAILURE;
	else if(AsciiStriCmp(str,"warn-write-failure")==0)
		*st=EFI_WARN_WRITE_FAILURE;
	else if(AsciiStriCmp(str,"warn-buffer-too-small")==0)
		*st=EFI_WARN_BUFFER_TOO_SMALL;
	else if(AsciiStriCmp(str,"warn-stale-data")==0)
		*st=EFI_WARN_STALE_DATA;
	else if(AsciiStriCmp(str,"load-error")==0)
		*st=EFI_LOAD_ERROR;
	else if(AsciiStriCmp(str,"invalid-parameter")==0)
		*st=EFI_INVALID_PARAMETER;
	else if(AsciiStriCmp(str,"unsupported")==0)
		*st=EFI_UNSUPPORTED;
	else if(AsciiStriCmp(str,"bad-buffer-size")==0)
		*st=EFI_BAD_BUFFER_SIZE;
	else if(AsciiStriCmp(str,"buffer-too-small")==0)
		*st=EFI_BUFFER_TOO_SMALL;
	else if(AsciiStriCmp(str,"not-ready")==0)
		*st=EFI_NOT_READY;
	else if(AsciiStriCmp(str,"device-error")==0)
		*st=EFI_DEVICE_ERROR;
	else if(AsciiStriCmp(str,"write-protected")==0)
		*st=EFI_WRITE_PROTECTED;
	else if(AsciiStriCmp(str,"out-of-resources")==0)
		*st=EFI_OUT_OF_RESOURCES;
	else if(AsciiStriCmp(str,"volume-corrupted")==0)
		*st=EFI_VOLUME_CORRUPTED;
	else if(AsciiStriCmp(str,"volume-full")==0)
		*st=EFI_VOLUME_FULL;
	else if(AsciiStriCmp(str,"no-media")==0)
		*st=EFI_NO_MEDIA;
	else if(AsciiStriCmp(str,"media-changed")==0)
		*st=EFI_MEDIA_CHANGED;
	else if(AsciiStriCmp(str,"not-found")==0)
		*st=EFI_NOT_FOUND;
	else if(AsciiStriCmp(str,"access-denied")==0)
		*st=EFI_ACCESS_DENIED;
	else if(AsciiStriCmp(str,"no-response")==0)
		*st=EFI_NO_RESPONSE;
	else if(AsciiStriCmp(str,"no-mapping")==0)
		*st=EFI_NO_MAPPING;
	else if(AsciiStriCmp(str,"timeout")==0)
		*st=EFI_TIMEOUT;
	else if(AsciiStriCmp(str,"not-started")==0)
		*st=EFI_NOT_STARTED;
	else if(AsciiStriCmp(str,"already-started")==0)
		*st=EFI_ALREADY_STARTED;
	else if(AsciiStriCmp(str,"aborted")==0)
		*st=EFI_ABORTED;
	else if(AsciiStriCmp(str,"icmp-error")==0)
		*st=EFI_ICMP_ERROR;
	else if(AsciiStriCmp(str,"tftp-error")==0)
		*st=EFI_TFTP_ERROR;
	else if(AsciiStriCmp(str,"protocol-error")==0)
		*st=EFI_PROTOCOL_ERROR;
	else if(AsciiStriCmp(str,"incompatible-version")==0)
		*st=EFI_INCOMPATIBLE_VERSION;
	else if(AsciiStriCmp(str,"security-violation")==0)
		*st=EFI_SECURITY_VIOLATION;
	else if(AsciiStriCmp(str,"crc-error")==0)
		*st=EFI_CRC_ERROR;
	else if(AsciiStriCmp(str,"end-of-media")==0)
		*st=EFI_END_OF_MEDIA;
	else if(AsciiStriCmp(str,"end-of-file")==0)
		*st=EFI_END_OF_FILE;
	else if(AsciiStriCmp(str,"invalid-language")==0)
		*st=EFI_INVALID_LANGUAGE;
	else if(AsciiStriCmp(str,"compromised-data")==0)
		*st=EFI_COMPROMISED_DATA;
	else return FALSE;
	return TRUE;
}
