/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_HIVEX
#ifdef ENABLE_UUID
#ifndef _BCD_H
#define _BCD_H
#include<stdint.h>
#include<stdbool.h>
#include<uuid/uuid.h>
struct bcd_store;
struct bcd_object;
struct bcd_element;
typedef struct bcd_store*bcd_store;
typedef struct bcd_object*bcd_object;
typedef struct bcd_element*bcd_element;
typedef struct bcd_device*bcd_device;
typedef enum bcd_value_type bcd_value_type;
typedef enum bcd_value_class bcd_value_class;
typedef enum bcd_type_debugger_type bcd_type_debugger_type;
typedef enum bcd_type_safe_boot bcd_type_safe_boot;
typedef enum bcd_type_nx_policy bcd_type_nx_policy;
typedef enum bcd_type_pae_policy bcd_type_pae_policy;
typedef enum bcd_type_boot_menu_policy bcd_type_boot_menu_policy;
typedef enum bcd_type_device_type bcd_type_device_type;
typedef enum bcd_type_local_device_type bcd_type_local_device_type;
typedef enum bcd_type_boot_ux_display_message bcd_type_boot_ux_display_message;
extern const char*bcd_type_debugger_type_name[];
extern const char*bcd_type_safe_boot_name[];
extern const char*bcd_type_nx_policy_name[];
extern const char*bcd_type_pae_policy_name[];
extern const char*bcd_type_device_type_name[];
extern const char*bcd_type_boot_menu_policy_name[];
extern const char*bcd_type_local_device_type_name[];
extern const char*bcd_type_boot_ux_display_message_name[];
enum bcd_value_type{
	BCD_TYPE_UNKNOWN0            = 0x00,
	BCD_TYPE_DEVICE              = 0x01,
	BCD_TYPE_STRING              = 0x02,
	BCD_TYPE_OBJECT              = 0x03,
	BCD_TYPE_OBJECT_LIST         = 0x04,
	BCD_TYPE_INTEGER             = 0x05,
	BCD_TYPE_BOOLEAN             = 0x06,
	BCD_TYPE_INTEGER_LIST        = 0x07,
	BCD_TYPE_MAX,
};
enum bcd_value_class{
	BCD_CLASS_LIBRARY            = 0x01,
	BCD_CLASS_APPLICATION        = 0x02,
	BCD_CLASS_DEVICE             = 0x03,
	BCD_CLASS_UNKNOWN0           = 0x04,
	BCD_CLASS_OEM                = 0x05,
	BCD_CLASS_MAX,
};
enum bcd_type_debugger_type{
	BCD_DebuggerType_Serial      = 0x00,
	BCD_DebuggerType_1394        = 0x01,
	BCD_DebuggerType_Usb         = 0x02,
	BCD_DebuggerType_Net         = 0x03,
	BCD_DebuggerType_Local       = 0x04,
	BCD_DebuggerType_MAX,
};
enum bcd_type_safe_boot{
	BCD_SafeBoot_Minimal         = 0x00,
	BCD_SafeBoot_Network         = 0x01,
	BCD_SafeBoot_DsRepair        = 0x02,
	BCD_SafeBoot_MAX,
};
enum bcd_type_nx_policy{
	BCD_NxPolicy_OptIn           = 0x00,
	BCD_NxPolicy_OptOut          = 0x01,
	BCD_NxPolicy_AlwaysOff       = 0x02,
	BCD_NxPolicy_AlwaysOn        = 0x03,
	BCD_NxPolicy_MAX,
};
enum bcd_type_pae_policy{
	BCD_PAEPolicy_Default        = 0x00,
	BCD_PAEPolicy_ForceEnable    = 0x01,
	BCD_PAEPolicy_ForceDisable   = 0x02,
	BCD_PAEPolicy_MAX,
};
enum bcd_type_boot_menu_policy{
	BCD_BootMenuPolicy_Legacy    = 0x00,
	BCD_BootMenuPolicy_Standard  = 0x01,
	BCD_BootMenuPolicy_MAX,
};
enum bcd_type_device_type{
	BCD_DT_DiskDevice            = 0x00,
	BCD_DT_Unknown0              = 0x01,
	BCD_DT_LegacyPartitionDevice = 0x02,
	BCD_DT_SerialDevice          = 0x03,
	BCD_DT_UdpDevice             = 0x04,
	BCD_DT_BootDevice            = 0x05,
	BCD_DT_PartitionDevice       = 0x06,
	BCD_DT_Unknown1              = 0x07,
	BCD_DT_LocateDevice          = 0x08,
	BCD_DT_MAX,
};
enum bcd_type_local_device_type{
	BCD_LDT_LocalDevice          = 0x00,
	BCD_LDT_FloppyDevice         = 0x01,
	BCD_LDT_CdRomDevice          = 0x02,
	BCD_LDT_RamDiskDevice        = 0x03,
	BCD_LDT_Unknown0             = 0x04,
	BCD_LDT_FileDevice           = 0x05,
	BCD_LDT_VirtualDiskDevice    = 0x06,
	BCD_LDT_MAX,
};
enum bcd_type_boot_ux_display_message{
	BCD_BDM_Default              = 0x00,
	BCD_BDM_Resume               = 0x01,
	BCD_BDM_HyperV               = 0x02,
	BCD_BDM_Recovery             = 0x03,
	BCD_BDM_StartupRepair        = 0x04,
	BCD_BDM_SystemImageRecovery  = 0x05,
	BCD_BDM_CommandPrompt        = 0x06,
	BCD_BDM_SystemRestore        = 0x07,
	BCD_BDM_PushButtonReset      = 0x08,
	BCD_BDM_FactoryReset         = 0x09,
	BCD_BDM_FveRecovery          = 0x0A,
	BCD_BDM_MAX,
};
extern bcd_store bcd_store_open(const char*path,int flags);
extern const char*bcd_store_get_path(bcd_store store);
extern hive_h*bcd_store_get_hive(bcd_store store);
#endif
#endif
#endif
