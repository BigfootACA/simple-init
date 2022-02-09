#include <Library/DebugLib.h>
#include "stdio.h"
#include "errno.h"
FILE *fopen(const char * file __attribute__((unused)), const char * mode __attribute__((unused))){
	DEBUG((EFI_D_ERROR,"try to call unsupported fopen\n"));
	errno=ENOSYS;
	return NULL;
}
int fclose(FILE *f __attribute__((unused))){
	DEBUG((EFI_D_ERROR,"try to call unsupported fclose\n"));
	errno=ENOSYS;
	return -1;
}
int fseek(FILE *f __attribute__((unused)), long p __attribute__((unused)), int m __attribute__((unused))){
	DEBUG((EFI_D_ERROR,"try to call unsupported fseek\n"));
	errno=ENOSYS;
	return -1;
}
char*getenv(const char*name){
	return NULL;
}
