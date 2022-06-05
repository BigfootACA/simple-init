/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_HIVEX
#ifndef BCDSTORE_H
#define BCDSTORE_H
#include<hivex.h>
#include<uuid/uuid.h>
#include"bcd.h"
#include"list.h"
typedef union{
	int32_t value;
	struct{
		unsigned long subtype:24;
		enum bcd_value_type format:4;
		enum bcd_value_class class:4;
	};
}bcd_element_type_in;
typedef struct guid{
	int32_t u1;
	int16_t u2;
	int16_t u3;
	int64_t u4;
}guid_t;
struct bcd_device{
	int8_t dev_type;
	int64_t flags;
	int64_t size;
	int64_t pad1;
	guid_t part_guid;
	int8_t local_dev_type;
	guid_t disk_guid;
	int64_t pad2;
	int64_t pad3;
};
struct bcd_spec_type_table{
	int32_t type;
	const char**table;
	size_t length;
};
struct bcd_object_type_table{
	int32_t type;
	const char*name;
};
struct bcd_element_type_table{
	int32_t type;
	const char*name;
	int32_t*objs;
};
struct bcd_guid_table{
	const char*name;
	uuid_t uuid;
};
struct bcd_store{
	char path[PATH_MAX];
	hive_h*reg;
	hive_node_h root;
	hive_node_h objs;
	list*objects;
	list*to_free;
};
struct bcd_object{
	struct bcd_store*bcd;
	uuid_t uuid;
	char alias[64];
	hive_node_h node;
	hive_node_h desc;
	hive_node_h eles;
	int32_t type;
	struct bcd_object_type_table*id;
	list*elements;
	list*to_free;
};
struct bcd_element{
	struct bcd_store*bcd;
	struct bcd_object*obj;
	char key[16];
	hive_node_h node;
	hive_value_h ele;
	hive_type val_type;
	size_t val_len;
	bcd_element_type_in et;
	struct bcd_element_type_table*id;
};
extern struct bcd_guid_table BcdGuidTable[];
extern struct bcd_spec_type_table BcdSpecTypes[];
extern struct bcd_object_type_table BcdObjectType[];
extern struct bcd_element_type_table BcdElementType[];

extern uuid_t*guid2uuid(uuid_t*uuid,guid_t guid);
extern guid_t*uuid2guid(guid_t*guid,uuid_t uuid);
extern bool bcd_get_guid_by_name(const char*name,uuid_t uuid);
extern const char*bcd_get_name_by_guid(uuid_t uuid);
#endif
#endif
