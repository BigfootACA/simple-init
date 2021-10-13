#ifndef _COMPATIBLE_H
#define _COMPATIBLE_H
#include<stddef.h>
extern int efi_status_to_errno(EFI_STATUS st);
extern char*comp_strdup(const char*s);
extern char*comp_strndup(const char*s,size_t n);
extern size_t comp_strnlen(const char*s,size_t n);
#define strdup comp_strdup
#define strndup comp_strndup
#define strnlen comp_strnlen
#endif
