/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_HIVEX
#define _GNU_SOURCE
#include<hivex.h>
#include<string.h>
#include"str.h"
#include"keyval.h"
#include"bcdstore.h"

static void bcd_dump_value(char*prefix,bcd_element ele){
	char*str,name[256],uuid[40],*xpre;
	if(!(xpre=malloc(strlen(prefix)+2)))goto fail;
	memset(xpre,0,strlen(prefix)+2);
	sprintf(xpre,"%s\t",prefix);
	printf("%s%s = ",prefix,bcd_element_get_display_name(ele,name));
	hive_type t=bcd_element_get_value_type(ele);
	enum bcd_value_type f=bcd_element_get_format(ele);
	switch(f){
		case BCD_TYPE_INTEGER:
			if(t!=hive_t_binary&&t!=hive_t_qword&&t!=hive_t_dword)goto fail;
			str=(char*)bcd_element_get_value_enum(ele);
			if(str)puts(str);
			else printf("%ld\n",bcd_element_get_value_number(ele,0));
		break;
		case BCD_TYPE_BOOLEAN:
			if(t!=hive_t_binary)goto fail;
			puts(BOOL2STR(bcd_element_get_value_number(ele,0)));
		break;
		case BCD_TYPE_INTEGER_LIST:
			if(t!=hive_t_binary)goto fail;
			if(!(str=bcd_element_get_value_data(ele)))goto fail;
			size_t len=bcd_element_get_value_length(ele);
			printf("HEX(%zu):[",len);
			for(size_t i=0;i<len;i++)
				printf("%02X",str[i]);
			puts("]");
			free(str);
		break;
		case BCD_TYPE_STRING:
		case BCD_TYPE_OBJECT:
			if(t!=hive_t_string)goto fail;
			if(f!=BCD_TYPE_OBJECT){
				if(!(str=bcd_element_get_value_string(ele)))goto fail;
				printf("\"%s\"\n",str);
				free(str);
			}else{
				printf("%s",bcd_element_get_value_uuid_name(ele,uuid));
				if((str=bcd_object_get_description(bcd_element_get_value_object(ele)))){
					printf(" (%s)",str);
					free(str);
				}
				putchar('\n');
			}
		break;
		case BCD_TYPE_OBJECT_LIST:{
			if(t!=hive_t_multiple_strings)goto fail;
			if(!(str=bcd_element_get_value_uuid_name_list(ele,true,xpre,"\n")))goto fail;
			printf("[\n%s%s]\n",str,prefix);
			free(str);
		}break;
		case BCD_TYPE_DEVICE:
			if(t!=hive_t_binary)goto fail;
			bcd_device d=bcd_element_get_value_device(ele);
			if(d){
				puts("DEVICE:(");
				printf("%s\tDevice Type: %s\n",prefix,bcd_device_get_type_name(d));
				printf("%s\tLocal Device Type: %s\n",prefix,bcd_device_get_local_type_name(d));
				printf("%s\tDisk UUID: %s\n",prefix,bcd_device_get_disk_uuid_string(d,uuid));
				printf("%s\tPartition UUID: %s\n",prefix,bcd_device_get_part_uuid_string(d,uuid));
				printf("%s)\n",prefix);
				bcd_device_free(d);
				break;
			}
		//fallthrough
		default:puts("(Unknown)");
	}
	free(xpre);
	return;
	fail:puts("(NULL)");
	if(xpre)free(xpre);
}

static void bcd_dump_bootmgr(bcd_store bcd){
	if(!bcd)return;
	bcd_object bootmgr=NULL;
	bcd_element*eles=NULL;
	if(!(bootmgr=bcd_get_object_by_name(bcd,"BOOTMGR")))return;
	if(!(eles=bcd_get_all_elements(bootmgr)))return;
	puts("Windows Boot Manager");
	for(size_t s=0;eles[s];s++)bcd_dump_value("\t",eles[s]);
}

static void bcd_dump_loader(bcd_store bcd){
	if(!bcd)return;
	char buf[40],*name;
	bcd_object*objs=NULL;
	bcd_element*eles=NULL;
	if(!(objs=bcd_get_boot_menu_objects(bcd)))return;
	puts("OS Loaders");
	for(size_t s=0;objs[s];s++){
		if(!(eles=bcd_get_all_elements(objs[s])))continue;
		printf(
			"\t%s@%s",
			bcd_object_get_display_name(objs[s],buf),
			bcd_object_get_type_name(objs[s])
		);
		if((name=bcd_object_get_description(objs[s]))){
			printf("(%s)",name);
			free(name);
		}
		putchar('\n');
		for(size_t e=0;eles[e];e++)
			bcd_dump_value("\t\t",eles[e]);
	}
}

void bcd_dump(struct bcd_store*bcd){
	if(!bcd)return;
	printf("BCD Store: %s\n\n",bcd_store_get_path(bcd));
	bcd_dump_bootmgr(bcd);
	putchar('\n');
	bcd_dump_loader(bcd);
}

void bcd_dump_all(struct bcd_store*bcd){
	if(!bcd)return;
	printf("BCD Store: %s\n",bcd_store_get_path(bcd));
	putchar('\n');
	uuid_t u;
	char buf[40],*name;
	bcd_object*objs=NULL;
	bcd_element*eles=NULL;
	if(!(objs=bcd_get_all_objects(bcd)))return;
	for(size_t s=0;objs[s];s++){
		if(!(eles=bcd_get_all_elements(objs[s])))continue;
		printf(
			"%s@%s",
			bcd_object_get_display_name(objs[s],buf),
			bcd_object_get_type_name(objs[s])
		);
		if((name=bcd_object_get_description(objs[s]))){
			printf(" (%s)",name);
			free(name);
		}
		putchar('\n');
		bcd_object_get_uuid(objs[s],u);
		uuid_unparse(u,buf);
		printf("\tGUID = %s\n",buf);
		printf("\tType = 0x%08X\n",bcd_object_get_type(objs[s]));
		for(size_t e=0;eles[e];e++)
			bcd_dump_value("\t",eles[e]);
		putchar('\n');
	}
}
#endif
