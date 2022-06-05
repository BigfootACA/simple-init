/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_HIVEX
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
extern void bcd_store_free(bcd_store store);

extern void bcd_dump(bcd_store bcd);
extern void bcd_dump_all(bcd_store bcd);

extern bcd_object bcd_get_object_by_node(bcd_store bcd,hive_node_h node);
extern bcd_object bcd_get_object_by_key(bcd_store bcd,const char*key);
extern bcd_object bcd_get_object_by_name(bcd_store bcd,const char*name);
extern bcd_object bcd_get_object_by_uuid(bcd_store bcd,uuid_t uuid);
extern bcd_object*bcd_get_all_objects(bcd_store bcd);
extern bcd_object*bcd_get_boot_menu_objects(bcd_store bcd);
extern char*bcd_object_get_key(bcd_object obj,char*buf);
extern char*bcd_object_get_display_name(bcd_object obj,char*buf);
extern bool bcd_object_get_uuid(bcd_object obj,uuid_t uuid);
extern int32_t bcd_object_get_type(bcd_object obj);
extern const char*bcd_object_get_type_name(bcd_object obj);
extern char*bcd_object_get_description(bcd_object obj);
extern bool bcd_object_is_type(bcd_object obj,int32_t type);
extern bool bcd_object_is_type_name(bcd_object obj,char*type);
extern const char*bcd_object_get_alias(bcd_object obj);
extern bcd_store bcd_object_get_store(bcd_object obj);
extern hive_h*bcd_object_get_hive(bcd_object obj);
extern void bcd_object_free(bcd_object obj);
extern void bcd_objects_free(bcd_object*objs);

extern bcd_element bcd_get_element_by_node(bcd_object obj,hive_node_h node);
extern bcd_element bcd_get_element_by_key(bcd_object obj,const char*key);
extern bcd_element bcd_get_element_by_name(bcd_object obj,const char*name);
extern bcd_element bcd_get_element_by_id(bcd_object obj,int32_t id);
extern bcd_element*bcd_get_all_elements(bcd_object obj);
extern const char*bcd_element_get_key(bcd_element ele);
extern int32_t bcd_element_get_type(bcd_element ele);
extern const char*bcd_element_get_type_name(bcd_element ele);
extern const char*bcd_element_get_display_name(bcd_element ele,char*buf);
extern bcd_value_type bcd_element_get_format(bcd_element ele);
extern bool bcd_element_is_format(bcd_element ele,bcd_value_type type);
extern bcd_value_class bcd_element_get_class(bcd_element ele);
extern bool bcd_element_is_class(bcd_element ele,bcd_value_class class);
extern bcd_object bcd_element_get_object(bcd_element ele);
extern bcd_store bcd_element_get_store(bcd_element ele);
extern hive_h*bcd_element_get_hive(bcd_element ele);
extern void bcd_element_free(bcd_element ele);
extern void bcd_elements_free(bcd_element*eles);
extern hive_value_h bcd_element_get_value(bcd_element ele);
extern void*bcd_element_get_value_data(bcd_element ele);
extern char*bcd_element_get_value_string(bcd_element ele);
extern char**bcd_element_get_value_multiple_strings(bcd_element ele);
extern int64_t bcd_element_get_value_number(bcd_element ele,int64_t def);
extern const char*bcd_element_get_value_enum(bcd_element ele);
extern bool bcd_element_get_value_uuid(bcd_element ele,uuid_t out);
extern bcd_object bcd_element_get_value_object(bcd_element ele);
extern const char*bcd_element_get_value_uuid_name(bcd_element ele,char*buf);
extern bcd_device bcd_element_get_value_device(bcd_element ele);
extern uuid_t*bcd_element_get_value_uuid_list(bcd_element ele,size_t*size);
extern char*bcd_element_get_value_uuid_name_list(bcd_element ele,bool refer,const char*prefix,const char*suffix);
extern hive_type bcd_element_get_value_type(bcd_element ele);
extern size_t bcd_element_get_value_length(bcd_element ele);

extern bcd_type_device_type bcd_device_get_type(bcd_device dev);
extern const char*bcd_device_get_type_name(bcd_device dev);
extern bcd_type_local_device_type bcd_device_get_local_type(bcd_device dev);
extern const char*bcd_device_get_local_type_name(bcd_device dev);
extern bool bcd_device_get_disk_uuid(bcd_device dev,uuid_t*uuid);
extern char*bcd_device_get_disk_uuid_string(bcd_device dev,char*uuid);
extern bool bcd_device_get_part_uuid(bcd_device dev,uuid_t*uuid);
extern char*bcd_device_get_part_uuid_string(bcd_device dev,char*uuid);
extern void bcd_device_free(bcd_device dev);
#endif
#endif
