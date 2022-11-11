#include<Uefi.h>
#include<Library/BaseLib.h>
#include<Library/UefiBootManagerLib.h>

BOOLEAN uefi_str_to_tpl(IN CONST CHAR8*str,OUT EFI_TPL*tpl){
	if(!str||!tpl)return FALSE;
	if(AsciiStriCmp(str,"application")==0)
		*tpl=TPL_APPLICATION;
	else if(AsciiStriCmp(str,"callback")==0)
		*tpl=TPL_CALLBACK;
	else if(AsciiStriCmp(str,"notify")==0)
		*tpl=TPL_NOTIFY;
	else if(AsciiStriCmp(str,"high-level")==0)
		*tpl=TPL_HIGH_LEVEL;
	else return FALSE;
	return TRUE;
}

BOOLEAN uefi_str_to_event_type(IN CONST CHAR8*str,OUT UINT32*type){
	if(!str||!type)return FALSE;
	if(AsciiStriCmp(str,"signal-virtual-address-change")==0)
		*type=EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE;
	else if(AsciiStriCmp(str,"signal-exit-boot-services")==0)
		*type=EVT_SIGNAL_EXIT_BOOT_SERVICES;
	else if(AsciiStriCmp(str,"notify-signal")==0)
		*type=EVT_NOTIFY_SIGNAL;
	else if(AsciiStriCmp(str,"notify-wait")==0)
		*type=EVT_NOTIFY_WAIT;
	else if(AsciiStriCmp(str,"runtime")==0)
		*type=EVT_RUNTIME;
	else if(AsciiStriCmp(str,"timer")==0)
		*type=EVT_TIMER;
	else return FALSE;
	return TRUE;
}

BOOLEAN uefi_str_to_reset_type(IN CONST CHAR8*str,OUT EFI_RESET_TYPE*type){
	if(!str||!type)return FALSE;
	if(AsciiStriCmp(str,"platform-specific")==0)
		*type=EfiResetPlatformSpecific;
	else if(AsciiStriCmp(str,"shutdown")==0)
		*type=EfiResetShutdown;
	else if(AsciiStriCmp(str,"warm")==0)
		*type=EfiResetWarm;
	else if(AsciiStriCmp(str,"cold")==0)
		*type=EfiResetCold;
	else return FALSE;
	return TRUE;
}

BOOLEAN uefi_str_to_memory_type(IN CONST CHAR8*str,OUT EFI_MEMORY_TYPE*type){
	if(!str||!type)return FALSE;
	if(AsciiStriCmp(str,"reserved-memory-type")==0)
		*type=EfiReservedMemoryType;
	else if(AsciiStriCmp(str,"loader-code")==0)
		*type=EfiLoaderCode;
	else if(AsciiStriCmp(str,"loader-data")==0)
		*type=EfiLoaderData;
	else if(AsciiStriCmp(str,"boot-services-code")==0)
		*type=EfiBootServicesCode;
	else if(AsciiStriCmp(str,"boot-services-data")==0)
		*type=EfiBootServicesData;
	else if(AsciiStriCmp(str,"runtime-services-code")==0)
		*type=EfiRuntimeServicesCode;
	else if(AsciiStriCmp(str,"runtime-services-data")==0)
		*type=EfiRuntimeServicesData;
	else if(AsciiStriCmp(str,"conventional-memory")==0)
		*type=EfiConventionalMemory;
	else if(AsciiStriCmp(str,"unusable-memory")==0)
		*type=EfiUnusableMemory;
	else if(AsciiStriCmp(str,"acpi-reclaim-memory")==0)
		*type=EfiACPIReclaimMemory;
	else if(AsciiStriCmp(str,"acpi-memory-nvs")==0)
		*type=EfiACPIMemoryNVS;
	else if(AsciiStriCmp(str,"memory-mapped-io")==0)
		*type=EfiMemoryMappedIO;
	else if(AsciiStriCmp(str,"memory-mapped-io-port-space")==0)
		*type=EfiMemoryMappedIOPortSpace;
	else if(AsciiStriCmp(str,"pal-code")==0)
		*type=EfiPalCode;
	else if(AsciiStriCmp(str,"persistent-memory")==0)
		*type=EfiPersistentMemory;
	else return FALSE;
	return TRUE;
}

BOOLEAN uefi_str_to_load_option_type(const char*str,EFI_BOOT_MANAGER_LOAD_OPTION_TYPE*type){
	if(!str||!type)return FALSE;
	if(AsciiStriCmp(str,"boot")==0)return LoadOptionTypeBoot;
	else if(AsciiStriCmp(str,"driver")==0)return LoadOptionTypeDriver;
	else if(AsciiStriCmp(str,"sysprep")==0)return LoadOptionTypeSysPrep;
	else if(AsciiStriCmp(str,"platform-recovery")==0)return LoadOptionTypePlatformRecovery;
	else return FALSE;
	return TRUE;
}

const char*uefi_memory_type_to_str(EFI_MEMORY_TYPE type){
	switch(type){
		case EfiReservedMemoryType:return "reserved-memory-type";
		case EfiLoaderCode:return "loader-code";
		case EfiLoaderData:return "loader-data";
		case EfiBootServicesCode:return "boot-services-code";
		case EfiBootServicesData:return "boot-services-data";
		case EfiRuntimeServicesCode:return "runtime-services-code";
		case EfiRuntimeServicesData:return "runtime-services-data";
		case EfiConventionalMemory:return "conventional-memory";
		case EfiUnusableMemory:return "unusable-memory";
		case EfiACPIReclaimMemory:return "acpi-reclaim-memory";
		case EfiACPIMemoryNVS:return "acpi-memory-nvs";
		case EfiMemoryMappedIO:return "memory-mapped-io";
		case EfiMemoryMappedIOPortSpace:return "memory-mapped-io-port-space";
		case EfiPalCode:return "pal-code";
		case EfiPersistentMemory:return "persistent-memory";
		default:return NULL;
	}
}

const char*uefi_load_option_type_to_str(EFI_BOOT_MANAGER_LOAD_OPTION_TYPE type){
	switch(type){
		case LoadOptionTypeBoot:return "boot";
		case LoadOptionTypeDriver:return "driver";
		case LoadOptionTypeSysPrep:return "sysprep";
		case LoadOptionTypePlatformRecovery:return "platform-recovery";
		default:return NULL;
	}
}
