#ifdef ENABLE_GUI
#ifdef ENABLE_HIVEX
#include<stdio.h>
#include<string.h>
#include"regedit.h"
#include"defines.h"
const char*hivex_type_to_string(hive_type type){
	switch(type){
		case hive_t_REG_NONE:                       return "KEY";
		case hive_t_REG_SZ:                         return "SZ";
		case hive_t_REG_EXPAND_SZ:                  return "EXT-SZ";
		case hive_t_REG_BINARY:                     return "BIN";
		case hive_t_REG_DWORD:                      return "DWORD";
		case hive_t_REG_DWORD_BIG_ENDIAN:           return "DWORD-BE";
		case hive_t_REG_LINK:                       return "LINK";
		case hive_t_REG_MULTI_SZ:                   return "MUL-SZ";
		case hive_t_REG_RESOURCE_LIST:              return "RES-LIST";
		case hive_t_REG_FULL_RESOURCE_DESCRIPTOR:   return "RES-DESC";
		case hive_t_REG_RESOURCE_REQUIREMENTS_LIST: return "RES-REQS";
		case hive_t_REG_QWORD:                      return "QWORD";
		default:                                    return "UNKNOWN";
	}
}
char*hivex_value_to_string(char*buf,size_t len,hive_h*h,hive_value_h val){
	size_t size=0;
	hive_type type;
	char*r=hivex_value_value(h,val,&type,&size),*x=buf;
	memset(buf,0,len);
	switch(type){
		case hive_t_string:
		case hive_t_expand_string:
		case hive_t_link:
			if(!r)return NULL;
			strncpy(x,r,MIN(len-1,size));
		break;
		case hive_t_dword:
		case hive_t_dword_be:snprintf(x,len-1,"%x",hivex_value_dword(h,val));break;
		case hive_t_qword:snprintf(x,len-1,"%lx",hivex_value_qword(h,val));break;
		default:
			if(!r)return NULL;
			for(size_t i=0;i<size&&(size_t)(x-buf)<len-1;i++){
				if(i>0&&i%4==0)*x++=' ';
				snprintf(x,3,"%02x",r[i]);
				x+=2;
			}
	}
	if(r)free(r);
	return buf;
}
#endif
#endif
