#include<Library/DebugLib.h>
#include<Library/UefiBootManagerLib.h>
#include<Library/ReportStatusCodeLib.h>
extern int guiapp_main(int argc,char**argv);
extern INTN ShellAppMain(UINTN argc,CHAR16**argv);
int main(int argc,char**argv){
	return guiapp_main(argc,argv);
}
EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ih,IN EFI_SYSTEM_TABLE*st){
	REPORT_STATUS_CODE(EFI_PROGRESS_CODE,(EFI_SOFTWARE_DXE_BS_DRIVER|EFI_SW_PC_USER_SETUP));
	EfiBootManagerConnectAll();
	EfiBootManagerRefreshAllBootOption();
	DebugPrint(EFI_D_INFO,"Initialize SimpleInit GUI...\n");
	static UINTN argc=1;
	static CHAR16*argv[]={L"guiapp",NULL};
	return ShellAppMain(argc,argv);
}
