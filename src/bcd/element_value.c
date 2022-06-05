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
#include"keyval.h"
#include"bcdstore.h"

hive_value_h bcd_element_get_value(bcd_element ele){
	return ele?ele->ele:0;
}

void*bcd_element_get_value_data(bcd_element ele){
	char*data=NULL;
	size_t len;
	hive_type type;
	if(
		!ele||
		!(data=hivex_value_value(
			bcd_element_get_hive(ele),
			bcd_element_get_value(ele),
			&type,&len
		))||
		bcd_element_get_value_type(ele)!=type||
		bcd_element_get_value_length(ele)!=len
	){
		if(data)free(data);
		data=NULL;
	}
	return data;
}

char*bcd_element_get_value_string(bcd_element ele){
	return ele?hivex_value_string(
		bcd_element_get_hive(ele),
		bcd_element_get_value(ele)
	):NULL;
}

char**bcd_element_get_value_multiple_strings(bcd_element ele){
	return ele?hivex_value_multiple_strings(
		bcd_element_get_hive(ele),
		bcd_element_get_value(ele)
	):NULL;
}

int64_t bcd_element_get_value_number(bcd_element ele,int64_t def){
	if(!ele)return def;
	hive_h*h=bcd_element_get_hive(ele);
	hive_value_h v=bcd_element_get_value(ele);
	char*data,t[8]={0};
	switch(bcd_element_get_value_type(ele)){
		case hive_t_REG_BINARY:
			if(!(data=bcd_element_get_value_data(ele)))return def;
			strncpy(t,data,bcd_element_get_value_length(ele));
			free(data);
			return *(int64_t*)t;
		case hive_t_REG_QWORD:return hivex_value_qword(h,v);
		case hive_t_REG_DWORD:return (int64_t)hivex_value_dword(h,v);
		case hive_t_REG_DWORD_BIG_ENDIAN:return (int64_t)be32toh(hivex_value_dword(h,v));
		default:return def;
	}
}

bool bcd_element_get_value_uuid(bcd_element ele,uuid_t out){
	if(!ele||!out)return false;
	bool st=false;
	char*str=bcd_element_get_value_string(ele);
	if(str){
		if(strlen(str)==38&&*str=='{'){
			str[37]=0;
			st=uuid_parse(str+1,out)==0;
		}
		free(str);
	}
	return st;
}

bcd_object bcd_element_get_value_object(bcd_element ele){
	uuid_t u;
	if(!bcd_element_get_value_uuid(ele,u))return NULL;
	return bcd_get_object_by_uuid(bcd_element_get_store(ele),u);
}

const char*bcd_element_get_value_uuid_name(bcd_element ele,char*buf){
	if(!ele)EPRET(EINVAL);
	uuid_t uuid;
	memset(buf,0,40);
	if(!bcd_element_get_value_uuid(ele,uuid))return NULL;
	const char*r=bcd_get_name_by_guid(uuid);
	buf[0]='{';
	if(r)strcpy(buf+1,r);
	else uuid_unparse(uuid,buf+1);
	buf[strlen(buf)-1]='}';
	return buf;
}

const char*bcd_element_get_value_enum(bcd_element ele){
	int64_t v=bcd_element_get_value_number(ele,-1);
	int32_t type=bcd_element_get_type(ele);
	if(v<0||type==0)return NULL;
	for(size_t s=0;BcdSpecTypes[s].table;s++){
		if(BcdSpecTypes[s].type!=type)continue;
		if(v>(int32_t)BcdSpecTypes[s].length-1)return NULL;
		return BcdSpecTypes[s].table[v];
	}
	return NULL;
}

uuid_t*bcd_element_get_value_uuid_list(bcd_element ele,size_t*size){
	char**strs=bcd_element_get_value_multiple_strings(ele),*c;
	if(!strs)return NULL;
	uuid_t*u;
	size_t cnt,len,s;
	for(cnt=0;strs[cnt];cnt++);
	len=sizeof(uuid_t)*(cnt+1);
	if(!(u=malloc(len)))goto fail;
	memset(u,0,len);
	if(size)(*size)=0;
	for(s=0;(c=strs[s]);s++){
		if(!*c)continue;
		if(c[0]!='{'||strlen(c)!=38||c[37]!='}')goto fail;
		c[37]=0;
		uuid_parse(c+1,u[s]);
		if(size)(*size)++;
	}
	done:
	if(strs){
		for(s=0;strs[s];s++)free(strs[s]);
		free(strs);
	}
	return u;
	fail:
	if(!u)free(u);
	u=NULL;
	goto done;
}

char*bcd_element_get_value_uuid_name_list(bcd_element ele,bool refer,const char*prefix,const char*suffix){
	char*buf,*p,*n;
	size_t cnt,len,a,pr,su;
	uuid_t*u=bcd_element_get_value_uuid_list(ele,&cnt);
	if(!u)return NULL;
	pr=strlen(prefix),su=strlen(suffix);
	len=(cnt*(pr+48+su+(refer?256:0)))+1;
	if(!(p=buf=malloc(len)))goto fail;
	memset(buf,0,len);
	for(a=0;a<cnt;a++){
		strcpy(p,prefix),p+=pr;
		*(p++)='{';
		const char*r=bcd_get_name_by_guid(u[a]);
		if(r)strcpy(p,r),p+=strlen(r);
		else{
			char bs[40]={0};
			uuid_unparse(u[a],bs);
			strcpy(p,bs),p+=strlen(bs);
		}
		*(p++)='}';
		if(refer&&(n=bcd_object_get_description(
			bcd_get_object_by_uuid(bcd_element_get_store(ele),u[a])
		))){
			*(p++)=' ',*(p++)='(';
			strncpy(p,n,255),p+=strlen(n);
			*(p++)=')';
			free(n);
		}
		strcpy(p,suffix),p+=su;
	}
	free(u);
	return buf;
	fail:
	if(u)free(u);
	if(buf)free(buf);
	return NULL;
}

bcd_device bcd_element_get_value_device(bcd_element ele){
	if(!ele)EPRET(EINVAL);
	char*data;
	hive_h*h=bcd_element_get_hive(ele);
	hive_value_h v=bcd_element_get_value(ele);
	// TODO: the length may not be fixed
	if(sizeof(struct bcd_device)!=bcd_element_get_value_length(ele))return NULL;
	if(bcd_element_get_value_type(ele)!=hive_t_binary)return NULL;
	if(!(data=hivex_value_value(h,v,NULL,NULL)))return NULL;
	return (bcd_device)data;
}

hive_type bcd_element_get_value_type(bcd_element ele){
	return ele?ele->val_type:0;
}

size_t bcd_element_get_value_length(bcd_element ele){
	return ele?ele->val_len:0;
}

#endif
