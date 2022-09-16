/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"fs_internal.h"
#include"system.h"

int fs_printf(fsh*f,const char*format,...){
	int r;
	va_list ap;
	va_start(ap,format);
	r=fs_vprintf(f,format,ap);
	va_end(ap);
	RET(r);
}

int fs_print_locked(fsh*f,const char*str){
	int r;
	if(!fsh_check(f))RET(EBADF);
	if(!str)RET(EINVAL);
	r=fs_full_write_locked(f,(void*)str,strlen(str));
	RET(r);
}

int fs_println_locked(fsh*f,const char*str){
	int r;
	if(!fsh_check(f))RET(EBADF);
	if((r=fs_print_locked(f,str))!=0)RET(r);
	if((r=fs_print_locked(f,"\n"))!=0)RET(r);
	RET(0);
}

int fs_vprintf_locked(fsh*f,const char*format,va_list ap){
	int r;
	char*buf=NULL;
	if(!fsh_check(f))RET(EBADF);
	if(!format)RET(EINVAL);
	vasprintf(&buf,format,ap);
	if(!buf)RET(EINVAL);
	r=fs_print_locked(f,buf);
	free(buf);
	RET(r);
}

int fs_printf_locked(fsh*f,const char*format,...){
	int r;
	va_list ap;
	va_start(ap,format);
	r=fs_vprintf_locked(f,format,ap);
	va_end(ap);
	RET(r);
}

int fs_full_write_locked(fsh*f,void*buffer,size_t btw){
	int r;
	size_t bw,wr=0;
	if(!fsh_check(f))RET(EBADF);
	if(!buffer)RET(EINVAL);
	do{
		errno=0,bw=0;
		r=fs_write_locked(f,buffer+wr,btw-wr,&bw),wr+=bw;
		if(r!=0){
			if(r==EINTR)continue;
			if(r==EAGAIN&&fs_has_flag(
				f->flags,FILE_FLAG_NON_BLOCK
			)){
				if((r=fs_wait(
					NULL,&f,1,-1,
					FILE_WAIT_IO_WRITE
				))!=0)RET(r);
				continue;
			}
			RET(r);
		}else if(wr==0)RET(EIO);
	}while(btw>wr);
	RET(0);
}

int fs_read_alloc_locked(
	fsh*f,
	void**buffer,
	size_t btr,
	size_t*br
){
	int r;
	if(!fsh_check(f))RET(EBADF);
	if(!buffer)RET(EINVAL);
	if(!(*buffer=malloc(btr+1)))RET(ENOMEM);
	((char*)*buffer)[btr]=0;
	r=fs_read_locked(f,*buffer,btr,br);
	if(r!=0)free(*buffer),*buffer=NULL;
	RET(r);
}

int fs_full_read_locked(
	fsh*f,
	void*buffer,
	size_t btr
){
	int r;
	size_t br,rd=0;
	if(!fsh_check(f))RET(EBADF);
	if(!buffer)RET(EINVAL);
	do{
		errno=0,br=0;
		r=fs_read_locked(
			f,buffer+rd,
			btr-rd,&br
		),rd+=br;
		if(r!=0){
			if(r==EINTR)continue;
			if(r==EAGAIN&&fs_has_flag(
				f->flags,FILE_FLAG_NON_BLOCK
			)){
				if((r=fs_wait_locked(
					NULL,&f,1,-1,
					FILE_WAIT_IO_READ,
					false
				))!=0)RET(r);
				continue;
			}
			RET(r);
		}else if(br==0)RET(EIO);
	}while(btr>rd);
	RET(0);
}

int fs_full_read_alloc_locked(fsh*f,void**buffer,size_t btr){
	int r;
	if(!fsh_check(f))RET(EBADF);
	if(!buffer)RET(EINVAL);
	if(!(*buffer=malloc(btr+1)))RET(ENOMEM);
	((char*)*buffer)[btr]=0;
	r=fs_full_read_locked(f,*buffer,btr);
	if(r!=0)free(*buffer),*buffer=NULL;
	RET(r);
}

int fs_read_to_locked(
	fsh*f,
	fsh*t,
	size_t size,
	size_t*sent
){
	int r;
	char buff[FS_BUF_SIZE];
	size_t br,rd=0,bs;
	if(!fsh_check(f))RET(EBADF);
	if(!fsh_check(t))RET(EBADF);
	do{
		errno=0,br=0;
		bs=MAX(size-rd,sizeof(buff));
		r=fs_read_locked(f,buff,bs,&br),rd+=br;
		if(r!=0){
			if(r==EINTR)continue;
			if(r==EAGAIN){
				if((r=fs_wait_locked(
					NULL,&f,1,-1,
					FILE_WAIT_IO_READ,
					false
				))!=0)RET(r);
				continue;
			}
			RET(r);
		}
		if(br>0&&(r=fs_full_write_locked(
			t,buff,br
		))!=0)RET(r);
		if(br==0||br!=bs)break;
	}while(size>rd);
	if(sent)*sent=br;
	RET(0);
}

int fs_read_to_fd_locked(
	fsh*f,
	int fd,
	size_t size,
	size_t*sent
){
	int r=0;
	char buff[FS_BUF_SIZE];
	size_t br,rd=0,bs;
	if(!fsh_check(f)||fd<0)RET(EBADF);
	do{
		errno=0,br=0,bs=MIN(size-rd,sizeof(buff));
		r=fs_read_locked(f,buff,bs,&br),rd+=br;
		if(r!=0){
			if(r==EINTR)continue;
			if(r==EAGAIN){
				if((r=fs_wait_locked(
					NULL,&f,1,-1,
					FILE_WAIT_IO_READ,
					false
				))!=0)RET(r);
				continue;
			}
			RET(r);
		}
		if(br>0&&full_write(
			fd,buff,br
		)!=(ssize_t)br)EXRET(EIO);
		if(br==0||br!=bs)break;
	}while(size>rd);
	if(sent)*sent=br;
	RET(r);
}

int fs_full_read_to_locked(fsh*f,fsh*t,size_t size){
	int r;
	size_t rd=0;
	r=fs_read_to_locked(f,t,size,&rd);
	if(r!=0||rd!=size)XRET(r,EIO);
	RET(0);
}

int fs_full_read_to_fd_locked(fsh*f,int fd,size_t size){
	int r;
	size_t rd=0;
	r=fs_read_to_fd_locked(f,fd,size,&rd);
	if(r!=0||rd!=size)XRET(r,EIO);
	RET(0);
}

int fs_read_all_to_locked(fsh*f,fsh*t,size_t*size){
	int r;
	size_t len=0;
	fs_seek_locked(f,0,SEEK_SET);
	if(size)*size=0;
	if(fs_get_size_locked(f,&len)==0&&len!=0){
		r=fs_full_read_to_locked(f,t,len);
		if(size)*size=len;
	}else while(true){
		len=0;
		r=fs_read_to_locked(
			f,t,FS_BUF_SIZE,&len
		);
		if(size)(*size)+=len;
		if(r!=0)break;
		if(len==0)break;
	}
	RET(r);
}

int fs_read_all_to_fd_locked(fsh*f,int fd,size_t*size){
	int r;
	size_t len=0;
	fs_seek_locked(f,0,SEEK_SET);
	if(size)*size=0;
	if(fs_get_size_locked(f,&len)==0&&len!=0){
		r=fs_full_read_to_fd_locked(f,fd,len);
		if(size)*size=len;
	}else while(true){
		len=0;
		r=fs_read_to_fd_locked(
			f,fd,FS_BUF_SIZE,&len
		);
		if(size)(*size)+=len;
		if(r!=0)break;
		if(len==0)break;
	}
	RET(r);
}

static bool hand_on_close_cmp(list*l,void*d){
	if(!l||!d)return false;
	LIST_DATA_DECLARE(p,l,struct fsh_hand_on_close*);
	if(!p||!p->name[0])return false;
	return strcmp(p->name,d);
}

int fs_del_on_close_locked(fsh*f,const char*name){
	list*l;
	if(!fsh_check(f))RET(EBADF);
	if(!name||!name[0])RET(EINVAL);
	if(!(l=list_search_one(
		f->on_close,
		hand_on_close_cmp,
		(void*)name
	)))RET(ENOENT);
	if(list_obj_del(
		&f->on_close,l,
		list_default_free
	)!=0)RET(ENOENT);
	RET(0);
}

int fs_add_on_close_locked(fsh*f,const char*name,fs_handle_close*hand,void*data){
	size_t c=0;
	struct fsh_hand_on_close p;
	if(!fsh_check(f))RET(EBADF);
	if(!hand)RET(EINVAL);
	memset(&p,0,sizeof(p));
	if(name){
		if(!name[0])RET(EINVAL);
		if(strlen(name)>=sizeof(p.name))RET(ENAMETOOLONG);
		strncpy(p.name,name,sizeof(p.name)-1);
		if(list_search_one(
			f->on_close,
			hand_on_close_cmp,
			(void*)name
		))RET(EEXIST);
	}else do{
		memset(p.name,0,sizeof(p.name));
		snprintf(
			p.name,sizeof(p.name)-1,
			"hand-%zu",c++
		);
	}while(list_search_one(
		f->on_close,
		hand_on_close_cmp,
		p.name
	));
	p.callback=hand;
	p.user_data=data;
	if(list_obj_add_new_dup(
		&f->on_close,&p,sizeof(p)
	)!=0)EXRET(ENOMEM);
	RET(0);
}

int fs_open_with(fsh**nf,fs_file_info*info,fs_file_flag flag){
	if(!nf||!info||!info->name[0])RET(EINVAL);
	if(!fsh_check(info->parent))RET(EBADF);
	return fs_open(info->parent,nf,info->name,flag);
}
