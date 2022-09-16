/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _FSDRV_H
#define _FSDRV_H
#include"filesystem.h"
#include"lock.h"
typedef void fs_initiator_function(bool deinit);
typedef struct fsh fsh;
typedef struct fsdrv fsdrv;
typedef struct fsvol fsvol;
typedef struct fsvol_private_info fsvol_private_info;
extern mutex_t fsdrv_lock;
extern mutex_t fsvol_lock;
extern mutex_t fsvol_info_lock;
extern list*fs_drivers;
extern list*fs_volumes;
extern list*fs_volume_infos;
extern fs_initiator_function*fs_initiator[];
extern const fsdrv fsdrv_template;
extern bool fsvol_private_info_check(const fsvol_private_info*info);
extern fsh*fsh_get_new(const fsdrv*drv,url*uri,void**data,size_t ds,fs_file_flag flag);
extern void fsdrv_initialize();
extern void fsdrv_deinitialize();
extern int fs_flush_locked(fsh*f);
extern int fs_exists_locked(fsh*f,const char*uri,bool*exists,bool lock);
extern int fs_open_locked(fsh*f,fsh**nf,const char*uri,fs_file_flag flag,bool lock);
extern int fs_readdir_locked(fsh*f,fs_file_info*info);
extern int fs_read_locked(fsh*f,void*buffer,size_t btr,size_t*br);
extern int fs_write_locked(fsh*f,void*buffer,size_t btw,size_t*bw);
extern int fs_wait_locked(fsh**gots,fsh**waits,size_t cnt,long timeout,fs_wait_flag flag,bool lock);
extern int fs_seek_locked(fsh*f,size_t pos,int whence);
extern int fs_tell_locked(fsh*f,size_t*pos);
extern int fs_map_locked(fsh*f,void**buffer,size_t off,size_t*size,fs_file_flag flag);
extern int fs_unmap_locked(fsh*f,void*buffer,size_t size);
extern int fs_get_info_locked(fsh*f,fs_file_info*info);
extern int fs_get_type_locked(fsh*f,fs_type*type);
extern int fs_get_size_locked(fsh*f,size_t*out);
extern int fs_get_name_locked(fsh*f,char*buff,size_t buff_len);
extern int fs_set_size_locked(fsh*f,size_t size);
extern int fs_get_url_locked(fsh*f,url**out);
extern int fs_get_path_locked(fsh*f,char*buff,size_t len);
extern int fs_get_path_alloc_locked(fsh*f,char**buff);
extern int fs_print_locked(fsh*f,const char*str);
extern int fs_println_locked(fsh*f,const char*str);
extern int fs_printf_locked(fsh*f,const char*format,...);
extern int fs_vprintf_locked(fsh*f,const char*format,va_list ap);
extern int fs_full_write_locked(fsh*f,void*buffer,size_t btw);
extern int fs_read_alloc_locked(fsh*f,void**buffer,size_t btr,size_t*br);
extern int fs_full_read_locked(fsh*f,void*buffer,size_t btr);
extern int fs_full_read_alloc_locked(fsh*f,void**buffer,size_t btr);
extern int fs_read_all_locked(fsh*f,void**buffer,size_t*size);
extern int fs_read_to_locked(fsh*f,fsh*t,size_t size,size_t*sent);
extern int fs_read_to_fd_locked(fsh*f,int fd,size_t size,size_t*sent);
extern int fs_full_read_to_locked(fsh*f,fsh*t,size_t size);
extern int fs_full_read_to_fd_locked(fsh*f,int fd,size_t size);
extern int fs_read_all_to_locked(fsh*f,fsh*t,size_t*size);
extern int fs_read_all_to_fd_locked(fsh*f,int fd,size_t*size);
extern int fs_get_features_locked(fsh*f,fs_feature*features);
extern int fs_rename_locked(fsh*f,const char*to);
extern int fs_rename_uri_locked(fsh*f,url*to);
extern int fs_rename_at_locked(fsh*f,const char*from,const char*to);
extern int fs_ioctl_va_locked(fsh*f,fs_ioctl_id id,va_list args);
extern int fs_ioctl_locked(fsh*f,fs_ioctl_id id,...);
extern int fs_del_on_close_locked(fsh*f,const char*name);
extern int fs_add_on_close_locked(fsh*f,const char*name,fs_handle_close*hand,void*data);
extern void fsdrv_posix_close(const fsdrv*drv,fsh*f);
extern int fsdrv_posix_read(const fsdrv*drv,fsh*f,void*buffer,size_t btr,size_t*br);
extern int fsdrv_posix_write(const fsdrv*drv,fsh*f,void*buffer,size_t btw,size_t*bw);
extern int fsdrv_posix_wait(const fsdrv*drv,fsh**gots,fsh**waits,size_t cnt,long timeout,fs_wait_flag flag);
extern const fsdrv*fsdrv_lookup(url*u);
extern bool fsdrv_check(const fsdrv*drv);
extern int fsdrv_register(fsdrv*drv);
extern int fsdrv_register_dup(fsdrv*drv);
extern const fsdrv*fsdrv_lookup_by_protocol(const char*name);
extern fsvol_private_info*fsvol_info_new(const fsvol*vol,void**data,const char*id,const char*name,const char*title,fsvol_feature features);
extern void fsvol_info_delete(fsvol_private_info*info);
extern int fsvol_add_volume(fsvol_private_info*info);
extern int fsvol_add_volume_dup(fsvol_private_info*info);
extern int fsvol_register(fsvol*drv);
extern int fsvol_register_dup(fsvol*drv);
static inline __attribute__((used)) fsvol_private_info*fsvolp_lookup_by_id(const char*id){return (fsvol_private_info*)fsvol_lookup_by_id(id);}
static inline __attribute__((used)) fsvol_private_info*fsvolp_lookup_by_name(const char*name){return (fsvol_private_info*)fsvol_lookup_by_name(name);}
static inline __attribute__((used)) fsvol_private_info*fsvolp_lookup_by_fsid(int64_t id1,int64_t id2){return (fsvol_private_info*)fsvol_lookup_by_fsid(id1,id2);}
static inline __attribute__((used)) fsvol_private_info*fsvolp_lookup_by_fs_label(const char*name){return (fsvol_private_info*)fsvol_lookup_by_fs_label(name);}
static inline __attribute__((used)) fsvol_private_info*fsvolp_lookup_by_part_label(const char*name){return (fsvol_private_info*)fsvol_lookup_by_part_label(name);}
static inline __attribute__((used)) fsvol_private_info**fsvolp_get_by_driver_name(const char*name){return (fsvol_private_info**)fsvol_get_by_driver_name(name);}
static inline __attribute__((used)) fsvol_private_info**fsvolp_get_volumes(){return (fsvol_private_info**)fsvol_get_volumes();}
typedef bool(*fs_drv_is_compatible)(const fsdrv*drv,url*uri);
typedef void(*fs_drv_close)(const fsdrv*drv,fsh*f);
typedef int(*fs_drv_flush)(const fsdrv*drv,fsh*f);
typedef int(*fs_drv_open)(const fsdrv*drv,fsh*nf,url*uri,fs_file_flag flag);
typedef int(*fs_drv_read)(const fsdrv*drv,fsh*f,void*buffer,size_t btr,size_t*br);
typedef int(*fs_drv_read_all)(const fsdrv*drv,fsh*f,void**buffer,size_t*br);
typedef int(*fs_drv_readdir)(const fsdrv*drv,fsh*f,fs_file_info*info);
typedef int(*fs_drv_write)(const fsdrv*drv,fsh*f,void*buffer,size_t btw,size_t*bw);
typedef int(*fs_drv_seek)(const fsdrv*drv,fsh*f,size_t pos,int whence);
typedef int(*fs_drv_tell)(const fsdrv*drv,fsh*f,size_t*pos);
typedef int(*fs_drv_map)(const fsdrv*drv,fsh*f,void**buffer,size_t off,size_t*size,fs_file_flag flag);
typedef int(*fs_drv_unmap)(const fsdrv*drv,fsh*f,void*buffer,size_t size);
typedef int(*fs_drv_get_info)(const fsdrv*drv,fsh*f,fs_file_info*info);
typedef int(*fs_drv_get_type)(const fsdrv*drv,fsh*f,fs_type*type);
typedef int(*fs_drv_get_size)(const fsdrv*drv,fsh*f,size_t*out);
typedef int(*fs_drv_get_name)(const fsdrv*drv,fsh*f,char*buff,size_t buff_len);
typedef int(*fs_drv_get_features)(const fsdrv*drv,fsh*f,fs_feature*features);
typedef int(*fs_drv_resize)(const fsdrv*drv,fsh*f,size_t size);
typedef int(*fs_drv_rename)(const fsdrv*drv,fsh*f,url*to);
typedef int(*fs_drv_ioctl)(const fsdrv*drv,fsh*f,fs_ioctl_id id,va_list args);
typedef int(*fs_drv_wait)(const fsdrv*drv,fsh**gots,fsh**waits,size_t cnt,long timeout,fs_wait_flag flag);
typedef int(*fs_drv_uri_parse)(const fsdrv*drv,fsh*f,const char*uri,url*u);
typedef int(*fs_drv_getcwd)(const fsdrv*drv,char*buff,size_t buff_len);
typedef int(*fs_vol_update)(const fsvol*drv,fsvol_private_info*info);
typedef int(*fs_vol_clean)(const fsvol*drv,fsvol_private_info*info);
typedef int(*fs_vol_open)(const fsvol*drv,fsvol_private_info*info,fsh**hand);
typedef int(*fs_vol_scan)(const fsvol*drv);
struct fsdrv{
	char magic[8];
	const fsdrv*base;
	void*data;
	size_t hand_data_size;
	bool readonly_fs;
	time_t cache_info_time;
	char protocol[256];
	char base_path[PATH_MAX];
	fs_feature features;
	fs_drv_is_compatible is_compatible;
	fs_drv_close close;
	fs_drv_flush flush;
	fs_drv_open open;
	fs_drv_read read;
	fs_drv_read_all read_all;
	fs_drv_readdir readdir;
	fs_drv_write write;
	fs_drv_seek seek;
	fs_drv_tell tell;
	fs_drv_map map;
	fs_drv_unmap unmap;
	fs_drv_get_info get_info;
	fs_drv_get_type get_type;
	fs_drv_get_size get_size;
	fs_drv_get_name get_name;
	fs_drv_get_features get_features;
	fs_drv_resize resize;
	fs_drv_rename rename;
	fs_drv_ioctl ioctl;
	fs_drv_wait wait;
	fs_drv_getcwd getcwd;
	fs_drv_uri_parse uri_parse;
};

struct fsvol_private_info{
	fsvol_info info;
	const fsvol*vol;
	void*data;
};

struct fsvol{
	char magic[8];
	char name[32];
	size_t info_data_size;
	fs_vol_clean clean;
	fs_vol_update update;
	fs_vol_scan scan;
	fs_vol_open open;
};

struct fsh_hand_on_close{
	char name[256-sizeof(void*)*2];
	fs_handle_close*callback;
	void*user_data;
};

struct fsh{
	char magic[8];
	void*data;
	int fd;
	mutex_t lock;
	const fsdrv*driver;
	fs_file_flag flags;
	time_t cache_info_time;
	fs_file_info cached_info;
	list*on_close;
	url*uri;
	char*url;
};
#endif
