#include "setjmp.h"
#include "stdlib.h"
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
extern int main_retval;
extern jmp_buf main_exit;
weak_decl void exit(int status){
	main_retval = status;          // Save our exit status.  Allows a status of 0.
	longjmp(main_exit, 0x55);     // Get out of here.  longjmp can't return 0. Use 0x55 for a non-zero value.
#ifdef __GNUC__
	__builtin_unreachable ();         // Keep GCC happy
#endif
}
weak_decl void abort(void){
	// FUCK YOU
	Print(L"FATAL ERROR\n");
	DEBUG((EFI_D_ERROR,"FATAL ERROR\n"));
	*((volatile int*)0)=0;
	ASSERT(FALSE);
	CpuDeadLoop();
#ifdef __GNUC__
	__builtin_unreachable ();         // Keep GCC happy
#endif
}
