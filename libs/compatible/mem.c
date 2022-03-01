#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include "sys/types.h"
#include "string.h"
#undef memcpy
#undef memmove
#undef memset
#undef memcmp
#undef memchr
#undef memrchr
void*comp_memcpy(void*dest,const void*src,size_t size){
	return (void*)CopyMem((VOID*)dest,(VOID*)src,(UINTN)size);
}
void*comp_memmove(void*dest,const void*src,size_t size){
	return (void*)CopyMem((VOID*)dest,(VOID*)src,(UINTN)size);
}
void*comp_memset(void*buff,int value,size_t size){
	return (void*)SetMem((VOID*)buff,(UINTN)size,(UINT8)value);
}
int comp_memcmp(const void*buff1,const void *buff2,size_t size){
	return (int)CompareMem((CONST VOID*)buff1,(CONST VOID*)buff2,(UINTN)size);
}
void*comp_memchr(const void*buff,int value,size_t size){
	return (void*)ScanMem8((VOID*)buff,(UINTN)size,(UINT8)value);
}
void *comp_memrchr(const void *m, int c, size_t n){
	const unsigned char *s = m;
	c = (unsigned char)c;
	while (n--) if (s[n]==c) return (void *)(s+n);
	return 0;
}
extern __typeof(comp_memcpy) memcpy __attribute__((__weak__, __alias__("comp_memcpy")));
extern __typeof(comp_memmove) memmove __attribute__((__weak__, __alias__("comp_memmove")));
extern __typeof(comp_memset) memset __attribute__((__weak__, __alias__("comp_memset")));
extern __typeof(comp_memcmp) memcmp __attribute__((__weak__, __alias__("comp_memcmp")));
extern __typeof(comp_memchr) memchr __attribute__((__weak__, __alias__("comp_memchr")));
extern __typeof(comp_memrchr) memrchr __attribute__((__weak__, __alias__("comp_memrchr")));
