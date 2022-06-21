/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _ENUM_CONV_H
#define _ENUM_CONV_H
#define DECL_CONV(_prefix,_data,_default,_cmp,_from_name,_from_field,_from_type,_to_name,_to_field,_to_type)\
	_to_type _prefix##_from_name##_to_##_to_name(_from_type _from_name){\
		for(size_t i=0;i<ARRLEN(_data);i++)\
			if(_cmp)return (_data)[i]._to_field;\
		return (_default);\
	}
#define DECL_XCONV(_prefix,_data,_cmp,_from_name,_from_field,_from_type,_to_name,_to_field,_to_type)\
	bool _prefix##_from_name##_to_##_to_name(_from_type _from_name,_to_type*_to_name){\
		if(!(_to_name))return false;\
		for(size_t i=0;i<ARRLEN(_data);i++)if((_cmp)){\
			(*(_to_name))=(_data)[i]._to_field;\
			return true;\
		}\
		return false;\
	}
#define CMP_CONV(_data,_field,_name)        ((_data)[i]._field==(_name))
#define STRCMP_CONV(_data,_field,_name)     ((_name)&&strcmp((_data)[i]._field,(_name))==0)
#define STRCASECMP_CONV(_data,_field,_name) ((_name)&&strcasecmp((_data)[i]._field,(_name))==0)
#define DECL_CMP_CONV(_prefix,_data,_default,_from_name,_from_field,_from_type,_to_name,_to_field,_to_type)\
	DECL_CONV(_prefix,_data,_default,CMP_CONV(_data,_from_field,_from_name),_from_name,_from_field,_from_type,_to_name,_to_field,_to_type)
#define DECL_STRCMP_CONV(_prefix,_data,_default,_from_name,_from_field,_from_type,_to_name,_to_field,_to_type)\
	DECL_CONV(_prefix,_data,_default,STRCMP_CONV(_data,_from_field,_from_name),_from_name,_from_field,_from_type,_to_name,_to_field,_to_type)
#define DECL_STRCASECMP_CONV(_prefix,_data,_from_name,_from_field,_from_type,_to_name,_to_field,_to_type)\
	DECL_CONV(_prefix,_data,STRCASECMP_CONV(_data,_from_field,_from_name),_from_name,_from_field,_from_type,_to_name,_to_field,_to_type)
#define DECL_CMP_XCONV(_prefix,_data,_from_name,_from_field,_from_type,_to_name,_to_field,_to_type)\
	DECL_XCONV(_prefix,_data,CMP_CONV(_data,_from_field,_from_name),_from_name,_from_field,_from_type,_to_name,_to_field,_to_type)
#define DECL_STRCMP_XCONV(_prefix,_data,_from_name,_from_field,_from_type,_to_name,_to_field,_to_type)\
	DECL_XCONV(_prefix,_data,STRCMP_CONV(_data,_from_field,_from_name),_from_name,_from_field,_from_type,_to_name,_to_field,_to_type)
#define DECL_STRCASECMP_XCONV(_prefix,_data,_from_name,_from_field,_from_type,_to_name,_to_field,_to_type)\
	DECL_XCONV(_prefix,_data,STRCASECMP_CONV(_data,_from_field,_from_name),_from_name,_from_field,_from_type,_to_name,_to_field,_to_type)

#endif
