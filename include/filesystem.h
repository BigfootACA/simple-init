/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H
#include<errno.h>
#include<stdbool.h>
#include<sys/types.h>
#include"defines.h"
#include"url.h"

#define FS_INFO_MAGIC "!FSINFO!"
#define FS_DRIVER_MAGIC "!FSDRV!!"
#define FS_HANDLE_MAGIC "!FSHAND!"
#define FS_VOLUME_MAGIC "!FSVOL!!"
#define FS_VOL_INFO_MAGIC "!FSVOLI!"

typedef struct fsh fsh;
typedef struct fsdrv fsdrv;
typedef struct fsvol fsvol;
typedef struct fs_file_info fs_file_info;
typedef struct fsvol_info fsvol_info;
typedef enum fs_type fs_type;
typedef enum fs_feature fs_feature;
typedef enum fs_ioctl_id fs_ioctl_id;
typedef enum fs_file_flag fs_file_flag;
typedef enum fs_wait_flag fs_wait_flag;
typedef enum fsvol_feature fsvol_feature;
typedef void fs_handle_close(const char*name,fsh*f,void*data);

enum fs_type{
	FS_TYPE_NONE           = 0x00000000,
	FS_TYPE_PARENT         = 0x01000000,
	FS_TYPE_FILE           = 0x02000000,
	FS_TYPE_VOLUME         = 0x04000000,
	FS_TYPE_FILE_REG       = FS_TYPE_FILE   | 0x000001,
	FS_TYPE_FILE_FOLDER    = FS_TYPE_FILE   | 0x000002,
	FS_TYPE_FILE_LINK      = FS_TYPE_FILE   | 0x000004,
	FS_TYPE_FILE_SOCKET    = FS_TYPE_FILE   | 0x000008,
	FS_TYPE_FILE_BLOCK     = FS_TYPE_FILE   | 0x000010,
	FS_TYPE_FILE_CHAR      = FS_TYPE_FILE   | 0x000020,
	FS_TYPE_FILE_FIFO      = FS_TYPE_FILE   | 0x000040,
	FS_TYPE_FILE_WHITEOUT  = FS_TYPE_FILE   | 0x000080,
	FS_TYPE_VOLUME_VIRTUAL = FS_TYPE_VOLUME | 0x000001,
	FS_TYPE_VOLUME_HDD     = FS_TYPE_VOLUME | 0x000002,
	FS_TYPE_VOLUME_SSD     = FS_TYPE_VOLUME | 0x000004,
	FS_TYPE_VOLUME_CARD    = FS_TYPE_VOLUME | 0x000008,
	FS_TYPE_VOLUME_FLASH   = FS_TYPE_VOLUME | 0x000010,
	FS_TYPE_VOLUME_TAPE    = FS_TYPE_VOLUME | 0x000020,
	FS_TYPE_VOLUME_ROM     = FS_TYPE_VOLUME | 0x000040,
	FS_TYPE_VOLUME_CD      = FS_TYPE_VOLUME | 0x000080,
	FS_TYPE_VOLUME_USB     = FS_TYPE_VOLUME | 0x000100,
	FS_TYPE_MAX            = UINT32_MAX
};

enum fs_file_flag{
	_FILE_FLAG_NONE      = 0x0000000000000000,
	_FILE_FLAG_MODE_MASK = 0x000000000000FFFF,
	FILE_FLAG_READ       = 0x0000000000010000,
	FILE_FLAG_WRITE      = 0x0000000000020000,
	FILE_FLAG_READWRITE  = FILE_FLAG_READ|FILE_FLAG_WRITE,
	FILE_FLAG_ACCESS     = 0x0000000000040000,
	FILE_FLAG_CREATE     = 0x0000000000080000,
	FILE_FLAG_SYNC       = 0x0000000000100000,
	FILE_FLAG_DIRECT     = 0x0000000000200000,
	FILE_FLAG_APPEND     = 0x0000000000400000,
	FILE_FLAG_TRUNCATE   = 0x0000000000800000,
	FILE_FLAG_NON_BLOCK  = 0x0000000001000000,
	FILE_FLAG_FOLDER     = 0x0000000002000000,
	FILE_FLAG_EXECUTE    = 0x0000000004000000,
	FILE_FLAG_SHARED     = 0x0000000008000000,
	FILE_FLAG_PRIVATE    = 0x0000000010000000,
	FILE_FLAG_FIXED      = 0x0000000020000000,
	_FILE_FLAG_MAX       = UINT64_MAX
};

enum fs_feature{
	_FS_FEATURE_NONE          = 0x0000000000000000,
	FS_FEATURE_READABLE       = 0x0000000000000001,
	FS_FEATURE_WRITABLE       = 0x0000000000000002,
	FS_FEATURE_SEEKABLE       = 0x0000000000000004,
	FS_FEATURE_UNIX_PERM      = 0x0000000000000008,
	FS_FEATURE_UNIX_DEVICE    = 0x0000000000000010,
	FS_FEATURE_NON_BLOCK      = 0x0000000000000020,
	FS_FEATURE_HAVE_STAT      = 0x0000000000000040,
	FS_FEATURE_HAVE_SIZE      = 0x0000000000000080,
	FS_FEATURE_HAVE_PATH      = 0x0000000000000100,
	FS_FEATURE_HAVE_TIME      = 0x0000000000000200,
	FS_FEATURE_HAVE_FOLDER    = 0x0000000000000400,
	_FS_FEATURE_MAX           = UINT64_MAX,
};

enum fs_wait_flag{
	_FILE_WAIT_NONE      = 0x0000000000000000,
	FILE_WAIT_READ       = 0x0000000000000001,
	FILE_WAIT_WRITE      = 0x0000000000000002,
	FILE_WAIT_ERROR      = 0x0000000000000004,
	FILE_WAIT_IO_READ    = FILE_WAIT_READ|FILE_WAIT_ERROR,
	FILE_WAIT_IO_WRITE   = FILE_WAIT_READ|FILE_WAIT_ERROR,
	FILE_WAIT_IO         = FILE_WAIT_READ|FILE_WAIT_WRITE,
	FILE_WAIT_ALL        = FILE_WAIT_WRITE|FILE_WAIT_ERROR,
	_FILE_WAIT_MAX       = UINT64_MAX
};

enum fs_ioctl_id{
	_FS_IOCTL_NONE  = 0,
	FS_IOCTL_UEFI_GET_HANDLE,
	FS_IOCTL_UEFI_GET_FILE_PROTOCOL,
	FS_IOCTL_UEFI_GET_DEVICE_PATH,
	_FS_IOCTL_MAX   = UINT64_MAX,
};

enum fsvol_feature{
	_FSVOL_NONE      = 0x0000000000000000,
	FSVOL_READONLY   = 0x0000000000000001,
	FSVOL_BOOTABLE   = 0x0000000000000002,
	FSVOL_SYSTEM     = 0x0000000000000004,
	FSVOL_VIRTUAL    = 0x0000000000000008,
	FSVOL_HIDDEN     = 0x0000000000000010,
	FSVOL_REMOVABLE  = 0x0000000000000020,
	FSVOL_CDROM      = 0x0000000000000040,
	FSVOL_PARTITION  = 0x0000000000000080,
	FSVOL_FILES      = 0x0000000000000100,
	_FSVOL_MAX       = UINT64_MAX
};

struct fs_file_info{
	char magic[8];
	fsh*parent;
	char name[256];
	size_t size;
	time_t ctime;
	time_t mtime;
	time_t atime;
	fs_type type;
	mode_t mode;
	uid_t owner;
	gid_t group;
	dev_t device;
	fs_feature features;
	char target[
		PATH_MAX-
		sizeof(char)*8-
		sizeof(fsh*)-
		sizeof(char)*256-
		sizeof(size_t)-
		sizeof(time_t)*3-
		sizeof(fs_type)-
		sizeof(mode_t)-
		sizeof(uid_t)-
		sizeof(gid_t)-
		sizeof(dev_t)-
		sizeof(fs_feature)
	];
};

struct fsvol_info{
	char magic[8];
	fsvol_feature features;
	char id[128];
	char name[256];
	char title[256];
	struct fsvol_info_fs{
		char uuid[64];
		char type[64];
		char label[512];
		uint64_t size;
		uint64_t used;
		uint64_t avail;
	}fs;
	struct fsvol_info_part{
		char uuid[64];
		char type[64];
		char label[512];
		uint64_t size;
		uint64_t sector_size;
		uint64_t sector_count;
	}part;
	struct fsvol_info_fsid{
		int64_t id1;
		int64_t id2;
	}fsid;
	char driver[32];
};

extern void fsh_free(fsh**h);
extern bool fsh_check(fsh*f);
extern bool fsvol_check(const fsvol*drv);
extern bool fsvol_info_check(const fsvol_info*info);
extern bool fs_file_info_check(fs_file_info*f);
extern void fs_close(fsh**f);
extern int fs_flush(fsh*f);
extern int fs_exists(fsh*f,const char*uri,bool*exists);
extern int fs_exists_uri(url*uri,bool*exists);
extern int fs_open(fsh*f,fsh**nf,const char*uri,fs_file_flag flag);
extern int fs_open_uri(fsh**nf,url*uri,fs_file_flag flag);
extern int fs_read(fsh*f,void*buffer,size_t btr,size_t*br);
extern int fs_read_alloc(fsh*f,void**buffer,size_t btr,size_t*br);
extern int fs_readdir(fsh*f,fs_file_info*info);
extern int fs_write(fsh*f,void*buffer,size_t btw,size_t*bw);
extern int fs_printf(fsh*f,const char*format,...) __attribute__((format(printf,2,3)));
extern int fs_print(fsh*f,const char*str);
extern int fs_println(fsh*f,const char*str);
extern int fs_vprintf(fsh*f,const char*format,va_list ap);
extern int fs_vprintf_file(fsh*f,const char*path,const char*format,va_list ap);
extern int fs_vprintf_file_uri(url*uri,const char*format,va_list ap);
extern int fs_full_read(fsh*f,void*buffer,size_t btr);
extern int fs_full_read_alloc(fsh*f,void**buffer,size_t btr);
extern int fs_full_write(fsh*f,void*buffer,size_t btw);
extern int fs_seek(fsh*f,size_t pos,int whence);
extern int fs_tell(fsh*f,size_t*pos);
extern int fs_map(fsh*f,void**buffer,size_t off,size_t*size,fs_file_flag flag);
extern int fs_unmap(fsh*f,void*buffer,size_t size);
extern int fs_wait(fsh**gots,fsh**waits,size_t cnt,long timeout,fs_wait_flag flag);
extern int fs_get_info(fsh*f,fs_file_info*info);
extern int fs_get_type(fsh*f,fs_type*type);
extern int fs_get_size(fsh*f,size_t*out);
extern int fs_get_name(fsh*f,char*buff,size_t buff_len);
extern int fs_set_size(fsh*f,size_t size);
extern int fs_write_file_uri(url*uri,void*buffer,size_t len);
extern int fs_write_file(fsh*f,const char*path,void*buffer,size_t len);
extern int fs_printf_file_uri(url*uri,const char*format,...) __attribute__((format(printf,2,3)));
extern int fs_printf_file(fsh*f,const char*path,const char*format,...) __attribute__((format(printf,3,4)));
extern int fs_read_file_uri(url*uri,void*buffer,size_t len);
extern int fs_read_file(fsh*f,const char*path,void*buffer,size_t len);
extern int fs_read_whole_file_uri(url*uri,void**buffer,size_t*len);
extern int fs_read_whole_file(fsh*f,const char*path,void**buffer,size_t*len);
extern int fs_full_read_to(fsh*f,fsh*t,size_t size);
extern int fs_full_read_to_fd(fsh*f,int fd,size_t size);
extern int fs_read_all(fsh*f,void**buffer,size_t*size);
extern int fs_read_to(fsh*f,fsh*t,size_t size,size_t*sent);
extern int fs_read_to_fd(fsh*f,int fd,size_t size,size_t*sent);
extern int fs_read_all_to(fsh*f,fsh*t,size_t*size);
extern int fs_read_all_to_fd(fsh*f,int fd,size_t*size);
extern int fs_open_with(fsh**nf,fs_file_info*info,fs_file_flag flag);
extern int fs_get_url(fsh*f,url**out);
extern int fs_get_path(fsh*f,char*buff,size_t len);
extern int fs_get_path_alloc(fsh*f,char**buff);
extern int fs_get_features(fsh*f,fs_feature*features);
extern bool fs_file_has_feature(fsh*f,fs_feature feature);
extern int fs_rename(fsh*f,const char*to);
extern int fs_rename_uri(fsh*f,url*to);
extern int fs_rename_at(fsh*f,const char*from,const char*to);
extern int fs_ioctl_va(fsh*f,fs_ioctl_id id,va_list args);
extern int fs_ioctl(fsh*f,fs_ioctl_id id,...);
extern int fs_add_on_close(fsh*f,const char*name,fs_handle_close*hand,void*data);
extern int fs_del_on_close(fsh*f,const char*name);
static inline __attribute__((used)) bool fs_has_type(fs_type src,fs_type type){return (src&type)==type;}
static inline __attribute__((used)) bool fs_has_flag(fs_file_flag src,fs_file_flag flag){return (src&flag)==flag;}
static inline __attribute__((used)) bool fs_has_feature(fs_feature src,fs_feature feature){return (src&feature)==feature;}
static inline __attribute__((used)) bool fs_has_vol_feature(fsvol_feature src,fsvol_feature feature){return (src&feature)==feature;}
extern const char*fs_type_to_string(const fs_type);
extern const char*fs_type_to_name(const fs_type);
extern bool fs_string_to_file_type(const char*string,fs_type*);
extern bool fs_name_to_file_type(const char*name,fs_type*);
extern int fs_register_zip(fsh*f,const char*name);
extern void fsvol_rescan();
extern void fsvol_update();
extern fsvol_info*fsvol_lookup_by_id(const char*id);
extern fsvol_info*fsvol_lookup_by_name(const char*name);
extern fsvol_info*fsvol_lookup_by_fsid(int64_t id1,int64_t id2);
extern fsvol_info*fsvol_lookup_by_fs_label(const char*name);
extern fsvol_info*fsvol_lookup_by_part_label(const char*name);
extern fsvol_info**fsvol_get_by_driver_name(const char*name);
extern fsvol_info**fsvol_get_volumes();
extern int fsvol_open_volume(fsvol_info*info,fsh**nf);
#define fs_read_to_stdout(f,size,sent)fs_read_to_fd(f,STDOUT_FILENO,size,sent)
#define fs_read_to_stderr(f,size,sent)fs_read_to_fd(f,STDERR_FILENO,size,sent)
#define fs_read_all_to_stdout(f,size)fs_read_all_to_fd(f,STDOUT_FILENO,size)
#define fs_read_all_to_stderr(f,size)fs_read_all_to_fd(f,STDERR_FILENO,size)
#define fs_full_read_to_stdout(f,size)fs_full_read_to_fd(f,STDOUT_FILENO,size)
#define fs_full_read_to_stderr(f,size)fs_full_read_to_fd(f,STDERR_FILENO,size)
#endif
