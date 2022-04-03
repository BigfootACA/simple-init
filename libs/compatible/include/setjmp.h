#ifndef	_SETJMP_H
#define	_SETJMP_H
#include "compatible.h"
#include  <Library/BaseLib.h>
#ifdef __cplusplus
extern "C" {
#endif

#include <bits/setjmp.h>

typedef BASE_LIBRARY_JUMP_BUFFER jmp_buf[1];
#define setjmp(env)   (INTN)SetJump((env))
static inline _Noreturn void longjmp(jmp_buf env,int val){
	LongJump(env,(UINTN)((val==0)?1:val));
#ifdef __GNUC__
	__builtin_unreachable ();         // Keep GCC happy
#endif
}

#ifdef __cplusplus
}
#endif

#endif
