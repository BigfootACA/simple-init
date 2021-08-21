#include<Library/UefiBootManagerLib.h>
extern int guiapp_main(int argc,char**argv);
int main(int argc,char**argv){
	EfiBootManagerConnectAll();
	EfiBootManagerRefreshAllBootOption();
	return guiapp_main(argc,argv);
}
