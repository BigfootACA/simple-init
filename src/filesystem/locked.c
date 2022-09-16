#include"fs_internal.h"

#define DECL_WRAPPER(name,decl,args)\
	int fs_##name decl{return fs_##name##_locked args;}
#define DECL_ONE_LOCK(name,decl,args,l1)\
	int fs_##name decl{\
		if(!l1)RET(EBADF);\
		MUTEX_LOCK(l1->lock);\
		int r=fs_##name##_locked args; \
		MUTEX_UNLOCK(l1->lock);\
		return r;\
	}
#define DECL_TWO_LOCK(name,decl,args,l1,l2)\
	int fs_##name decl{\
		if(!l1||!l2)RET(EBADF);\
		MUTEX_LOCK(l1->lock);\
		MUTEX_LOCK(l2->lock);\
		int r=fs_##name##_locked args; \
		MUTEX_UNLOCK(l1->lock);\
		MUTEX_UNLOCK(l2->lock);\
		return r;\
	}

DECL_ONE_LOCK(flush,(fsh*f),(f),f)
DECL_ONE_LOCK(tell,(fsh*f,size_t*pos),(f,pos),f)
DECL_ONE_LOCK(get_url,(fsh*f,url**out),(f,out),f)
DECL_ONE_LOCK(rename_uri,(fsh*f,url*to),(f,to),f)
DECL_ONE_LOCK(rename,(fsh*f,const char*to),(f,to),f)
DECL_ONE_LOCK(get_size,(fsh*f,size_t*out),(f,out),f)
DECL_ONE_LOCK(print,(fsh*f,const char*str),(f,str),f)
DECL_ONE_LOCK(println,(fsh*f,const char*str),(f,str),f)
DECL_ONE_LOCK(set_size,(fsh*f,size_t size),(f,size),f)
DECL_ONE_LOCK(get_type,(fsh*f,fs_type*type),(f,type),f)
DECL_ONE_LOCK(readdir,(fsh*f,fs_file_info*info),(f,info),f)
DECL_ONE_LOCK(get_path_alloc,(fsh*f,char**buff),(f,buff),f)
DECL_ONE_LOCK(get_info,(fsh*f,fs_file_info*info),(f,info),f)
DECL_ONE_LOCK(del_on_close,(fsh*f,const char*name),(f,name),f)
DECL_ONE_LOCK(seek,(fsh*f,size_t pos,int whence),(f,pos,whence),f)
DECL_ONE_LOCK(get_path,(fsh*f,char*buff,size_t len),(f,buff,len),f)
DECL_ONE_LOCK(unmap,(fsh*f,void*buffer,size_t size),(f,buffer,size),f)
DECL_ONE_LOCK(get_features,(fsh*f,fs_feature*features),(f,features),f)
DECL_ONE_LOCK(read_all_to_fd,(fsh*f,int fd,size_t*size),(f,fd,size),f)
DECL_ONE_LOCK(full_read_to_fd,(fsh*f,int fd,size_t size),(f,fd,size),f)
DECL_ONE_LOCK(full_read,(fsh*f,void*buffer,size_t btr),(f,buffer,btr),f)
DECL_ONE_LOCK(ioctl_va,(fsh*f,fs_ioctl_id id,va_list args),(f,id,args),f)
DECL_ONE_LOCK(full_write,(fsh*f,void*buffer,size_t btw),(f,buffer,btw),f)
DECL_ONE_LOCK(read_all,(fsh*f,void**buffer,size_t*size),(f,buffer,size),f)
DECL_ONE_LOCK(vprintf,(fsh*f,const char*format,va_list ap),(f,format,ap),f)
DECL_ONE_LOCK(rename_at,(fsh*f,const char*from,const char*to),(f,from,to),f)
DECL_ONE_LOCK(get_name,(fsh*f,char*buff,size_t buff_len),(f,buff,buff_len),f)
DECL_ONE_LOCK(full_read_alloc,(fsh*f,void**buffer,size_t btr),(f,buffer,btr),f)
DECL_ONE_LOCK(read,(fsh*f,void*buffer,size_t btr,size_t*br),(f,buffer,btr,br),f)
DECL_ONE_LOCK(write,(fsh*f,void*buffer,size_t btw,size_t*bw),(f,buffer,btw,bw),f)
DECL_ONE_LOCK(read_to_fd,(fsh*f,int fd,size_t size,size_t*sent),(f,fd,size,sent),f)
DECL_ONE_LOCK(read_alloc,(fsh*f,void**buffer,size_t btr,size_t*br),(f,buffer,btr,br),f)
DECL_ONE_LOCK(add_on_close,(fsh*f,const char*name,fs_handle_close*hand,void*data),(f,name,hand,data),f)
DECL_ONE_LOCK(map,(fsh*f,void**buffer,size_t off,size_t*size,fs_file_flag flag),(f,buffer,off,size,flag),f)
DECL_TWO_LOCK(read_all_to,(fsh*f,fsh*t,size_t*size),(f,t,size),f,t)
DECL_TWO_LOCK(full_read_to,(fsh*f,fsh*t,size_t size),(f,t,size),f,t)
DECL_TWO_LOCK(read_to,(fsh*f,fsh*t,size_t size,size_t*sent),(f,t,size,sent),f,t)
DECL_WRAPPER(exists,(fsh*f,const char*uri,bool*exists),(f,uri,exists,true))
DECL_WRAPPER(open,(fsh*f,fsh**nf,const char*uri,fs_file_flag flag),(f,nf,uri,flag,true))
DECL_WRAPPER(wait,(fsh**gots,fsh**waits,size_t cnt,long timeout,fs_wait_flag flag),(gots,waits,cnt,timeout,flag,true))

int fs_ioctl(fsh*f,fs_ioctl_id id,...){
	va_list args;
	va_start(args,id);
	int r=fs_ioctl_va(f,id,args);
	va_end(args);
	return r;
}
