static struct NAME{
	const TYPE code;
	const char*type;
	const char*name;
}NAME[]={
	#define STR(_type,_str) {.code=(_type),.type=(#_type),.name=(#_str)},
	#include TARGET
	#undef STR
};
DECL_CMP_CONV(lv_,NAME,NULL,CODE,code,const TYPE,string,type,const char*)
DECL_CMP_CONV(lv_,NAME,NULL,CODE,code,const TYPE,name,name,const char*)
DECL_STRCASECMP_XCONV(lv_,NAME,string,type,const char*,CODE,code,TYPE)
DECL_STRCASECMP_XCONV(lv_,NAME,name,name,const char*,CODE,code,TYPE)
#undef NAME
#undef CODE
#undef TYPE
#undef TARGET
