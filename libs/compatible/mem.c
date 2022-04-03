#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include "sys/types.h"
#include "string.h"
weak_decl void*memcpy(void*dest,const void*src,size_t size){
	return (void*)CopyMem((VOID*)dest,(VOID*)src,(UINTN)size);
}
weak_decl void*memmove(void*dest,const void*src,size_t size){
	return (void*)CopyMem((VOID*)dest,(VOID*)src,(UINTN)size);
}
weak_decl void*memset(void*buff,int value,size_t size){
	return (void*)SetMem((VOID*)buff,(UINTN)size,(UINT8)value);
}
weak_decl int memcmp(const void*buff1,const void *buff2,size_t size){
	return (int)CompareMem((CONST VOID*)buff1,(CONST VOID*)buff2,(UINTN)size);
}
weak_decl void*memchr(const void*buff,int value,size_t size){
	return (void*)ScanMem8((VOID*)buff,(UINTN)size,(UINT8)value);
}
weak_decl void *memrchr(const void *m, int c, size_t n){
	const unsigned char *s = m;
	c = (unsigned char)c;
	while (n--) if (s[n]==c) return (void *)(s+n);
	return 0;
}
