/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"aboot.h"
#include<stddef.h>
#include<stdint.h>
#ifdef ENABLE_UEFI
#include"uefi.h"
#include<limits.h>
#include<Library/BaseLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Protocol/SimpleFileSystem.h>
#include<Protocol/BlockIo.h>
#include<Guid/FileInfo.h>
#else
#include<fcntl.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/mman.h>
#endif
#define align(val,alg) ((val)+(((alg)-(val))&((alg)-1)))
#define KERN_OFF abootimg_get_kernel_offset
#define KERN_SIZ abootimg_get_kernel_size
#define KERN_END abootimg_get_kernel_end
#define KERN_ADD abootimg_get_kernel_address
#define RAMDISK_OFF abootimg_get_ramdisk_offset
#define RAMDISK_SIZ abootimg_get_ramdisk_size
#define RAMDISK_END abootimg_get_ramdisk_end
#define RAMDISK_ADD abootimg_get_ramdisk_address
#define SEC_OFF abootimg_get_second_offset
#define SEC_SIZ abootimg_get_second_size
#define SEC_END abootimg_get_second_end
#define SEC_ADD abootimg_get_second_address

// android boot image header
#define ABOOT_MAGIC "ANDROID!"
typedef struct aboot_header{
	uint8_t  magic[8];
	uint32_t kernel_size;
	uint32_t kernel_address;
	uint32_t ramdisk_size;
	uint32_t ramdisk_address;
	uint32_t second_size;
	uint32_t second_address;
	uint32_t tags_address;
	uint32_t page_size;
	uint32_t unused;
	uint32_t os_version;
	char     name[16];
	char     cmdline[512];
	uint32_t id[32];
}aboot_header;

typedef struct aboot_image{
	aboot_header head;
	void*kernel;
	void*ramdisk;
	void*second;
}aboot_image;


bool abootimg_check_page(size_t p){
	if(p<=sizeof(aboot_header))return false;
	if((p&(p-1)))return false;
	return true;
}

static aboot_image*abootimg_allocate(){
	aboot_image*img=malloc(sizeof(aboot_image));
	if(!img)return NULL;
	memset(img,0,sizeof(aboot_image));
	return img;
}

static bool abootimg_check_header(aboot_image*img){
	if(!img)return false;
	if(memcmp(
		img->head.magic,
		ABOOT_MAGIC,
		sizeof(img->head.magic)
	)!=0)return false;
	if(!abootimg_check_page(img->head.page_size))return false;
	return true;
}

static uint32_t get_aligned_size(aboot_image*img,uint32_t size){
	return align(size,abootimg_get_page_size(img));
}

static bool parse_image(aboot_image*img,void*file,size_t len){
	if(!abootimg_check_header(img))return false;
	#define DO_LOAD(type)\
		if(abootimg_get_##type##_size(img)>0){\
			if(abootimg_get_##type##_end(img)>len)return false;\
			if(!(img->type=malloc(abootimg_get_##type##_size(img))))return false;\
			memcpy(\
				img->type,\
				file+abootimg_get_##type##_offset(img),\
				abootimg_get_##type##_size(img)\
			);\
		}
	DO_LOAD(kernel)
	DO_LOAD(ramdisk)
	DO_LOAD(second)
	#undef DO_LOAD
	return true;
}

bool abootimg_is_empty(aboot_image*img){
	return img&&!img->kernel&&!img->ramdisk&&!img->second;
}

bool abootimg_is_invalid(aboot_image*img){
	if(!abootimg_check_header(img))return true;
	if(!img->kernel)return true;
	return false;
}

uint32_t abootimg_get_kernel_offset(aboot_image*img){
	return abootimg_get_page_size(img);
}

uint32_t abootimg_get_ramdisk_offset(aboot_image*img){
	return KERN_OFF(img)+get_aligned_size(img,KERN_SIZ(img));
}

uint32_t abootimg_get_second_offset(aboot_image*img){
	return RAMDISK_OFF(img)+get_aligned_size(img,RAMDISK_SIZ(img));
}

uint32_t abootimg_get_image_size(aboot_image*img){
	return SEC_OFF(img)+get_aligned_size(img,SEC_SIZ(img));
}

aboot_image*abootimg_new_image(){
	aboot_image*img=abootimg_allocate();
	if(!img)return NULL;
	memcpy(img->head.magic,ABOOT_MAGIC,sizeof(img->head.magic));
	img->head.page_size=4096;
	return img;
}

void abootimg_free(aboot_image*img){
	if(!img)return;
	if(img->kernel)free(img->kernel);
	if(img->ramdisk)free(img->ramdisk);
	if(img->second)free(img->second);
	free(img);
}

aboot_image*abootimg_load_from_memory(void*file,size_t len){
	if(!file||len<=0)return NULL;
	aboot_image*img=abootimg_allocate();
	if(!img)goto fail;
	if(len<=sizeof(aboot_header))goto fail;
	memcpy(&img->head,file,sizeof(aboot_header));
	if(len<abootimg_get_image_size(img))goto fail;
	if(!parse_image(img,file,len))goto fail;
	return img;
	fail:if(img)abootimg_free(img);
	return NULL;
}

bool abootimg_generate(aboot_image*img,void**output,uint32_t*len){
	if(!img||!output||!len)return false;
	size_t size=abootimg_get_image_size(img);
	if(*len&&*len>size)size=*len;
	else *len=size;
	if(!(*output=malloc(size)))return false;
	memset(*output,0,size);
	memcpy(*output,&img->head,sizeof(aboot_header));
	#define DO_GEN(type)\
		if(img->type)memcpy(\
			(*output)+abootimg_get_##type##_offset(img),\
			img->type,abootimg_get_##type##_size(img)\
		);
	DO_GEN(kernel)
	DO_GEN(ramdisk)
	DO_GEN(second)
	#undef DO_GEN
	return true;
}

aboot_image*abootimg_load_from_fsh(fsh*f){
	int r=0;
	void*buf=NULL;
	size_t len=0;
	aboot_image*img=NULL;
	if(!f)return NULL;
	fs_seek(f,0,SEEK_SET);
	r=fs_read_all(f,&buf,&len);
	if(r!=0||!buf)return NULL;
	img=abootimg_load_from_memory(buf,len);
	free(buf);
	return img;
}

aboot_image*abootimg_load_from_url(url*u){
	int r=0;
	fsh*f=NULL;
	aboot_image*img=NULL;
	if(!u)return NULL;
	r=fs_open_uri(&f,u,FILE_FLAG_READ);
	if(r!=0||!f)return NULL;
	img=abootimg_load_from_fsh(f);
	fs_close(&f);
	return img;
}

aboot_image*abootimg_load_from_url_path(const char*path){
	url*u=NULL;
	aboot_image*img=NULL;
	if(!path)return NULL;
	if(!(u=url_parse_new(path,0)))return NULL;
	img=abootimg_load_from_url(u);
	url_free(u);
	return img;
}

bool abootimg_save_to_fsh(aboot_image*img,fsh*f){
	int r;
	void*out=NULL;
	uint32_t len=0;
	if(!img||!f)return false;
	if(!abootimg_generate(img,&out,&len))return false;
	fs_seek(f,0,SEEK_SET);
	fs_set_size(f,len);
	r=fs_full_write(f,out,len);
	free(out);
	return r==0;
}

bool abootimg_save_to_url(aboot_image*img,url*u){
	int r=0;
	bool ret;
	fsh*f=NULL;
	if(!img||!u)return false;
	r=fs_open_uri(&f,u,FILE_FLAG_WRITE);
	if(r!=0||!f)return false;
	ret=abootimg_save_to_fsh(img,f);
	fs_close(&f);
	return ret;
}

bool abootimg_save_to_url_path(aboot_image*img,const char*path){
	bool ret;
	url*u=NULL;
	if(!img||!path)return false;
	if(!(u=url_parse_new(path,0)))return false;
	ret=abootimg_save_to_url(img,u);
	url_free(u);
	return ret;
}

#ifdef ENABLE_UEFI
aboot_image*abootimg_load_from_blockio(EFI_BLOCK_IO_PROTOCOL*bio){
	if(!bio)return NULL;
	UINTN size=0;
	void*cont=NULL;
	aboot_image*img=NULL;
	UINT32 mid=bio->Media->MediaId;
	if(!(img=abootimg_new_image()))return NULL;
	size=align(sizeof(aboot_header),bio->Media->BlockSize);
	if(!(cont=AllocateZeroPool(size)))goto fail;
	if(EFI_ERROR(bio->ReadBlocks(bio,mid,0,size,cont)))goto fail;
	CopyMem(&img->head,cont,sizeof(aboot_header));
	FreePool(cont);
	cont=NULL;
	if(!abootimg_check_header(img))goto fail;
	size=align(abootimg_get_image_size(img),bio->Media->BlockSize);
	if(!(cont=AllocateZeroPool(size)))goto fail;
	if(EFI_ERROR(bio->ReadBlocks(bio,mid,0,size,cont)))goto fail;
	if(!parse_image(img,cont,size))goto fail;
	FreePool(cont);
	return img;
	fail:
	if(img)abootimg_free(img);
	if(cont)FreePool(cont);
	return NULL;
}

bool abootimg_save_to_blockio(aboot_image*img,EFI_BLOCK_IO_PROTOCOL*bio){
	if(!bio)return false;
	uint32_t len=0;
	void*cont=NULL;
	EFI_STATUS st;
	UINT32 mid=bio->Media->MediaId;
	len=align(abootimg_get_image_size(img),bio->Media->BlockSize);
	if(!abootimg_generate(img,&cont,&len))return false;
	st=bio->WriteBlocks(bio,mid,0,len,cont);
	bio->FlushBlocks(bio);
	FreePool(cont);
	return !EFI_ERROR(st);
}

aboot_image*abootimg_load_from_fp(EFI_FILE_PROTOCOL*fp){
	if(!fp)return NULL;
	UINTN read=0;
	void*cont=NULL;
	aboot_image*img=NULL;
	EFI_FILE_INFO*info=NULL;
	if(EFI_ERROR(efi_file_get_file_info(fp,NULL,&info)))goto fail;
	read=info->FileSize;
	if(!(cont=AllocateZeroPool(info->FileSize)))goto fail;
	if(EFI_ERROR(fp->Read(fp,&read,cont)))goto fail;
	if(read!=info->FileSize)goto fail;
	img=abootimg_load_from_memory(cont,info->FileSize);
	FreePool(cont);
	FreePool(info);
	return img;
	fail:
	if(img)abootimg_free(img);
	if(info)FreePool(info);
	if(cont)FreePool(cont);
	return NULL;
}

aboot_image*abootimg_load_from_wfile(EFI_FILE_PROTOCOL*root,CHAR16*path){
	if(!root||!path)return NULL;
	aboot_image*img=NULL;
	EFI_FILE_PROTOCOL*file=NULL;
	if(EFI_ERROR(root->Open(
		root,&file,path,
		EFI_FILE_MODE_READ,0
	)))return NULL;
	img=abootimg_load_from_fp(file);
	file->Close(file);
	return img;
}

aboot_image*abootimg_load_from_file(EFI_FILE_PROTOCOL*root,char*path){
	if(!root||!path)return NULL;
	aboot_image*ret=NULL;
	UINTN ws=PATH_MAX*sizeof(CHAR16);
	CHAR16*wp=AllocateZeroPool(ws);
	if(wp){
		AsciiStrToUnicodeStrS(path,wp,ws/sizeof(CHAR16));
		ret=abootimg_load_from_wfile(root,wp);
		FreePool(wp);
	}
	return ret;
}

bool abootimg_save_to_fp(aboot_image*img,EFI_FILE_PROTOCOL*fp){
	if(!img||!fp)return false;
	void*out=NULL;
	EFI_STATUS st;
	uint32_t len=0;
	EFI_FILE_INFO*info=NULL;
	UINTN infos=0,write=0;
	if(!abootimg_generate(img,&out,&len))return false;
	write=(UINTN)len;
	fp->SetPosition(fp,0);
	if(!EFI_ERROR(efi_file_get_file_info(fp,&infos,&info))){
		info->FileSize=write;
		fp->SetInfo(fp,&gEfiFileInfoGuid,infos,info);
	}
	if(info)FreePool(info);
	st=fp->Write(fp,&write,out);
	free(out);
	return !EFI_ERROR(st)&&write==(UINTN)len;
}

bool abootimg_save_to_wfile(aboot_image*img,EFI_FILE_PROTOCOL*root,CHAR16*path){
	if(!root||!path)return false;
	bool ret=false;
	EFI_FILE_PROTOCOL*file=NULL;
	if(EFI_ERROR(root->Open(
		root,&file,path,
		EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE,0
	)))return false;
	ret=abootimg_save_to_fp(img,file);
	file->Close(file);
	return ret;
}

bool abootimg_save_to_file(aboot_image*img,EFI_FILE_PROTOCOL*root,char*path){
	if(!root||!path)return false;
	bool ret=false;
	UINTN ws=PATH_MAX*sizeof(CHAR16);
	CHAR16*wp=AllocateZeroPool(ws);
	if(wp){
		AsciiStrToUnicodeStrS(path,wp,ws/sizeof(CHAR16));
		ret=abootimg_save_to_wfile(img,root,wp);
		FreePool(wp);
	}
	return ret;
}

#define ABOOTIMG_LOAD_SAVE(tag) \
	bool abootimg_load_##tag##_from_blockio(aboot_image*img,EFI_BLOCK_IO_PROTOCOL*bio){\
		if(!bio)return false;\
		void*cont=NULL;\
		bool ret=false;\
		UINT32 mid=bio->Media->MediaId;\
		UINTN size=(bio->Media->LastBlock-1)*bio->Media->BlockSize;\
		if(size>=UINT32_MAX)goto fail;\
		if(!(cont=AllocateZeroPool(size)))goto fail;\
		if(EFI_ERROR(bio->ReadBlocks(bio,mid,0,size,cont)))goto fail;\
		ret=abootimg_set_##tag(img,cont,(uint32_t)size);\
		fail:\
		if(cont)FreePool(cont);\
		return ret;\
	}\
	bool abootimg_load_##tag##_from_fp(aboot_image*img,EFI_FILE_PROTOCOL*fp){\
		if(!fp)return false;\
		UINTN read=0;\
		void*cont=NULL;\
		EFI_FILE_INFO*info=NULL;\
		if(EFI_ERROR(efi_file_get_file_info(fp,NULL,&info)))goto fail;\
		read=info->FileSize;\
		if(!(cont=AllocateZeroPool(info->FileSize)))goto fail;\
		if(EFI_ERROR(fp->Read(fp,&read,cont)))goto fail;\
		if(read!=info->FileSize)goto fail;\
		bool ret=abootimg_set_##tag(img,cont,info->FileSize);\
		FreePool(cont);\
		FreePool(info);\
		return ret;\
		fail:\
		if(info)FreePool(info);\
		if(cont)FreePool(cont);\
		return false;\
	}\
	bool abootimg_load_##tag##_from_wfile(aboot_image*img,EFI_FILE_PROTOCOL*root,CHAR16*path){\
		if(!root||!path)return false;\
		EFI_FILE_PROTOCOL*file=NULL;\
		if(EFI_ERROR(root->Open(\
			root,&file,path,\
			EFI_FILE_MODE_READ,0\
		)))return false;\
		bool ret=abootimg_load_##tag##_from_fp(img,file);\
		file->Close(file);\
		return ret;\
	}\
	bool abootimg_load_##tag##_from_file(aboot_image*img,EFI_FILE_PROTOCOL*root,char*path){\
		if(!root||!path)return false;\
		bool ret=false;\
		UINTN ws=PATH_MAX*sizeof(CHAR16);\
		CHAR16*wp=AllocateZeroPool(ws);\
		if(wp){\
			AsciiStrToUnicodeStrS(path,wp,ws/sizeof(CHAR16));\
			ret=abootimg_load_##tag##_from_wfile(img,root,wp);\
			FreePool(wp);\
		}\
		return ret;\
	}\
	bool abootimg_save_##tag##_to_blockio(aboot_image*img,EFI_BLOCK_IO_PROTOCOL*bio){\
		if(!bio)return false;\
		EFI_STATUS st;\
		UINT32 mid=bio->Media->MediaId;\
		UINTN len=abootimg_get_##tag##_size(img);\
		st=bio->WriteBlocks(bio,mid,0,len,img->tag);\
		bio->FlushBlocks(bio);\
		return !EFI_ERROR(st);\
	}\
	bool abootimg_save_##tag##_to_fp(aboot_image*img,EFI_FILE_PROTOCOL*fp){\
		if(!img||!fp||!img->tag)return false;\
		EFI_STATUS st;\
		uint32_t len;\
		EFI_FILE_INFO*info=NULL;\
		UINTN infos=0,write=0;\
		len=abootimg_get_##tag##_size(img);\
		write=(UINTN)len;\
		fp->SetPosition(fp,0);\
		if(!EFI_ERROR(efi_file_get_file_info(fp,&infos,&info))){\
			info->FileSize=write;\
			fp->SetInfo(fp,&gEfiFileInfoGuid,infos,info);\
		}\
		if(info)FreePool(info);\
		st=fp->Write(fp,&write,img->tag);\
		return !EFI_ERROR(st)&&write==(UINTN)len;\
	}\
	bool abootimg_save_##tag##_to_wfile(aboot_image*img,EFI_FILE_PROTOCOL*root,CHAR16*path){\
		if(!root||!path)return false;\
		bool ret=false;\
		EFI_FILE_PROTOCOL*file=NULL;\
		if(EFI_ERROR(root->Open(\
			root,&file,path,\
			EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE,0\
		)))return false;\
		ret=abootimg_save_##tag##_to_fp(img,file);\
		file->Close(file);\
		return ret;\
	}\
	bool abootimg_save_##tag##_to_file(aboot_image*img,EFI_FILE_PROTOCOL*root,char*path){\
		if(!root||!path)return false;\
		bool ret=false;\
		UINTN ws=PATH_MAX*sizeof(CHAR16);\
		CHAR16*wp=AllocateZeroPool(ws);\
		if(wp){\
			AsciiStrToUnicodeStrS(path,wp,ws/sizeof(CHAR16));\
			ret=abootimg_save_##tag##_to_wfile(img,root,wp);\
			FreePool(wp);\
		}\
		return ret;\
	}
#else
static bool allocate_read(int fd,size_t blk,void**buf,size_t*len){
	size_t bs,mem,size;
	if(!buf||blk<=0||fd<0)return false;
	if(!(*buf=malloc(blk)))return false;
	mem=blk,bs=blk,size=0;
	while(1){
		if(bs<=0){
			mem+=blk,bs+=blk;
			void*b=realloc(*buf,mem);
			if(!b)goto ef;
			*buf=b;
		}
		ssize_t r=read(fd,(*buf)+size,bs);
		if(r<0)goto ef;
		else if(r==0)break;
		size+=r,bs-=r;
	}
	if(len)*len=size;
	return true;
	ef:
	if(!*buf)free(*buf);
	size=0,*buf=NULL;
	return false;
}

aboot_image*abootimg_load_from_fd(int fd){
	if(fd<0)return NULL;
	void*map=NULL;
	struct stat st;
	size_t len=0;
	aboot_image*img=NULL;
	if(fstat(fd,&st)==0){
		if(st.st_size>0)len=st.st_size;
		else if(st.st_blksize>0)len=st.st_size;
	}
	lseek(fd,0,SEEK_SET);
	if(len>0){
		if((size_t)len<=sizeof(aboot_header))return NULL;
		if(!(map=mmap(0,len,PROT_READ,MAP_PRIVATE,fd,0)))return NULL;
		img=abootimg_load_from_memory(map,len);
		munmap(map,len);
	}else{
		if(!allocate_read(fd,0x100000,&map,&len))return NULL;
		img=abootimg_load_from_memory(map,len);
		free(map);
	}
	return img;
}

bool abootimg_save_to_fd(aboot_image*img,int fd){
	if(!img||fd<0)return false;
	void*out=NULL;
	uint32_t len=0;
	if(!abootimg_generate(img,&out,&len))return false;
	lseek(fd,0,SEEK_SET);
	ftruncate(fd,len);
	ssize_t r=write(fd,out,len);
	free(out);
	return r==len;
}

aboot_image*abootimg_load_from_file(int cfd,const char*file){
	if(!file)return false;
	int fd=openat(cfd,file,O_RDONLY);
	if(fd<0)return false;
	aboot_image*img=abootimg_load_from_fd(fd);
	close(fd);
	return img;
}

bool abootimg_save_to_file(aboot_image*img,int cfd,const char*file){
	if(!img||!file)return false;
	int fd=openat(cfd,file,O_WRONLY|O_CREAT,0644);
	if(fd<0)return false;
	bool ret=abootimg_save_to_fd(img,fd);
	close(fd);
	return ret;
}

#define ABOOTIMG_LOAD_SAVE(tag)\
	bool abootimg_save_##tag##_to_fd(aboot_image*img,int fd){\
		if(!img||fd<0||!img->tag)return false;\
		ssize_t s=(ssize_t)abootimg_get_##tag##_size(img);\
		lseek(fd,0,SEEK_SET);\
		ftruncate(fd,s);\
		return write(fd,img->tag,s)==s;\
	}\
	bool abootimg_save_##tag##_to_file(aboot_image*img,int cfd,const char*file){\
		if(!img||!file||!img->tag)return false;\
		int fd=openat(cfd,file,O_WRONLY|O_CREAT,0644);\
		if(fd<0)return false;\
		bool ret=abootimg_save_##tag##_to_fd(img,fd);\
		close(fd);\
		return ret;\
	}\
	bool abootimg_load_##tag##_from_fd(aboot_image*img,int fd){\
		if(!img||fd<0)return false;\
                bool ret;\
                size_t len=0;\
		void*map=NULL;\
		struct stat st;\
		if(fstat(fd,&st)==0){\
			if(st.st_size>0)len=st.st_size;\
			else if(st.st_blksize>0)len=st.st_size;\
		}\
		lseek(fd,0,SEEK_SET);\
		if(len>0){\
			if(!(map=mmap(0,len,PROT_READ,MAP_PRIVATE,fd,0)))return NULL;\
			ret=abootimg_set_##tag(img,map,len);\
			munmap(map,len);\
		}else{\
			if(!allocate_read(fd,0x100000,&map,&len))return NULL;\
			ret=abootimg_set_##tag(img,map,len);\
			free(map);\
		}\
		return ret;\
	}\
	bool abootimg_load_##tag##_from_file(aboot_image*img,int cfd,const char*file){\
		if(!img||!file)return false;\
		int fd=openat(cfd,file,O_RDONLY);\
		if(fd<0)return false;\
		bool ret=abootimg_load_##tag##_from_fd(img,fd);\
		close(fd);\
		return ret;\
	}
#endif
#define ABOOTIMG_FSH_LOAD_SAVE(tag)\
	bool abootimg_save_##tag##_to_fsh(aboot_image*img,fsh*f){\
		if(!img||!f||!img->tag)return false;\
		ssize_t s=(ssize_t)abootimg_get_##tag##_size(img);\
		fs_seek(f,0,SEEK_SET);\
		fs_set_size(f,s);\
		return fs_full_write(f,img->tag,s)==0;\
	}\
	bool abootimg_save_##tag##_to_url(aboot_image*img,url*u){\
		int r=0;\
		bool ret;\
		fsh*f=NULL;\
		if(!img||!u)return false;\
		r=fs_open_uri(&f,u,FILE_FLAG_WRITE);\
		if(r!=0||!f)return false;\
		ret=abootimg_save_##tag##_to_fsh(img,f);\
		fs_close(&f);\
		return ret;\
	}\
	bool abootimg_save_##tag##_to_url_path(aboot_image*img,const char*path){\
		bool ret;\
		url*u=NULL;\
		if(!img||!path)return false;\
		if(!(u=url_parse_new(path,0)))return false;\
		ret=abootimg_save_##tag##_to_url(img,u);\
		url_free(u);\
		return ret;\
	}\
        bool abootimg_load_##tag##_from_fsh(aboot_image*img,fsh*f){\
                bool ret;\
		int r=0;\
		void*buf=NULL;\
		size_t len=0;\
		if(!img||!f)return false;\
		fs_seek(f,0,SEEK_SET);\
		r=fs_read_all(f,&buf,&len);\
		if(r!=0||!buf)return false;\
		ret=abootimg_set_##tag(img,buf,len);\
		free(buf);\
		return ret;\
	}\
	bool abootimg_load_##tag##_from_url(aboot_image*img,url*u){\
                int r=0;\
                bool ret;\
                fsh*f=NULL;\
                if(!u)return false;\
                r=fs_open_uri(&f,u,FILE_FLAG_READ);\
                if(r!=0||!f)return false;\
                ret=abootimg_load_##tag##_from_fsh(img,f);\
                fs_close(&f);\
                return ret;\
        }\
	bool abootimg_load_##tag##_from_url_path(aboot_image*img,const char*path){\
		bool ret;\
		url*u=NULL;\
		if(!img||!path)return false;\
		if(!(u=url_parse_new(path,0)))return false;\
		ret=abootimg_load_##tag##_from_url(img,u);\
		url_free(u);\
		return ret;\
	}
#define ABOOTIMG_GET_SET(tag)\
	bool abootimg_set_##tag(aboot_image*img,void*tag,uint32_t len){\
		if(!img||!tag||len<=0)return false;\
		if(img->tag)free(img->tag);\
		img->tag=NULL,img->head.tag##_size=0;\
		if(!(img->tag=malloc(len)))return false;\
		memcpy(img->tag,tag,len);\
		img->head.tag##_size=len;\
		return true;\
	}\
	uint32_t abootimg_get_##tag(aboot_image*img,void**tag){\
		if(!img||!tag)return 0;\
		*tag=img->tag;\
		return abootimg_get_##tag##_size(img);\
	}\
	uint32_t abootimg_get_##tag##_end(aboot_image*img){\
		return abootimg_get_##tag##_offset(img)+abootimg_get_##tag##_size(img);\
	}\
	bool abootimg_copy_##tag(aboot_image*img,void*dest,size_t buf_len){\
		if(!img||!dest||buf_len<abootimg_get_##tag##_size(img))return 0;\
		memcpy(dest,img->tag,abootimg_get_##tag##_size(img));\
		return abootimg_get_##tag##_size(img);\
	}\
	bool abootimg_have_##tag(aboot_image*img){\
		return img&&img->tag;\
	}
#define ABOOTIMG_GET_VAR(type,key,def)\
	type abootimg_get_##key(aboot_image*img){return img?img->head.key:def;}
#define ABOOTIMG_SET_VAR(key)\
	void abootimg_set_##key(aboot_image*img,uint32_t key){if(img)img->head.key=key;}
#define ABOOTIMG_SET_STRING(key) \
	void abootimg_set_##key(aboot_image*img,const char*key){\
		if(!img)return;\
		memset(img->head.key,0,sizeof(img->head.key));\
		if(key)strncpy(img->head.key,key,sizeof(img->head.key)-1);\
	}
#define ABOOTIMG_GETSET_VAR(key) ABOOTIMG_GET_VAR(uint32_t,key,0) ABOOTIMG_SET_VAR(key)
#define ABOOTIMG_GETSET_STRING(key) ABOOTIMG_GET_VAR(const char*,key,NULL) ABOOTIMG_SET_STRING(key)
#define ABOOTIMG_CONT(tag) ABOOTIMG_GET_SET(tag) ABOOTIMG_LOAD_SAVE(tag) ABOOTIMG_FSH_LOAD_SAVE(tag)

ABOOTIMG_CONT(kernel)
ABOOTIMG_CONT(ramdisk)
ABOOTIMG_CONT(second)
ABOOTIMG_GETSET_STRING(name)
ABOOTIMG_GETSET_STRING(cmdline)
ABOOTIMG_GETSET_VAR(kernel_size)
ABOOTIMG_GETSET_VAR(ramdisk_size)
ABOOTIMG_GETSET_VAR(second_size)
ABOOTIMG_GETSET_VAR(kernel_address)
ABOOTIMG_GETSET_VAR(ramdisk_address)
ABOOTIMG_GETSET_VAR(second_address)
ABOOTIMG_GETSET_VAR(tags_address)
ABOOTIMG_GETSET_VAR(page_size)
