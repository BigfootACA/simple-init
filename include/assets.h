/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef ASSETS_H
#define ASSETS_H
#include<stdbool.h>
#include<sys/types.h>
#ifndef uid_t
typedef unsigned uid_t;
#endif
#ifndef gid_t
typedef unsigned gid_t;
#endif

// generic entry metadata
struct entry{
	struct entry_dir*parent;
	char name[256];
	mode_t mode;
	uid_t owner;
	gid_t group;
	struct timespec atime;
	struct timespec mtime;
};
typedef struct entry entry;

// file entry metadata
struct entry_file{
	struct entry info;
	dev_t dev;
	char*content;
	size_t offset;
	size_t length;
};
typedef struct entry_dir entry_dir;

// folder entry metadata
struct entry_dir{
	struct entry info;
	struct entry_dir**subdirs;
	struct entry_file**subfiles;
};
typedef struct entry_file entry_file;

// BUILD/rootfs.c: generic rootfs
extern entry_dir assets_rootfs;

// src/assets/assets.c: set file owner and mode
extern int set_assets_file_info(int fd,entry_file*file);

// src/assets/assets.c: write file, call set_assets_file_info if pres=true
extern int write_assets_file(int fd,entry_file*file,bool pres);

// src/assets/assets.c: create and write file
extern int create_assets_file(int dfd,entry_file*file,bool pres,bool override);

// src/assets/assets.c: create and write folder and children items
extern int create_assets_dir(int dfd,entry_dir*dir,bool override);

// src/assets/assets.c: get file by path in an assets
extern entry_file*get_assets_file(entry_dir*dir,const char*path);

// src/assets/assets.c: get folder by path in an assets
extern entry_dir*get_assets_dir(entry_dir*dir,const char*path);

// src/assets/assets.c: check folder is out of bound
extern bool asset_dir_check_out_bound(entry_dir*bound,entry_dir*target);

// src/assets/assets.c: check file is out of bound
extern bool asset_file_check_out_bound(entry_dir*bound,entry_file*target);

// src/assets/assets.c: get root of an asset folder entry
extern entry_dir*asset_dir_get_root(entry_dir*target);

// src/assets/assets.c: get root of an asset file entry
extern entry_dir*asset_file_get_root(entry_file*target);

#ifdef _LIST_H

// src/assets/assets.c: read folder path as list
extern list*asset_dir_get_path_list(entry_dir*target,entry_dir*end);

// src/assets/assets.c: read file path as list
extern list*asset_file_get_path_list(entry_file*target,entry_dir*end);

#endif

// src/assets/assets.c: read folder path as string
extern char*asset_dir_get_path(entry_dir*target,entry_dir*end,char*buffer,size_t len);

// src/assets/assets.c: read file path as string
extern char*asset_file_get_path(entry_file*target,entry_dir*end,char*buffer,size_t len);

// get from rootfs
#define rootfs_get_assets_file(paths)get_assets_file(&assets_rootfs,paths)
#define rootfs_get_assets_dir(paths)get_assets_dir(&assets_rootfs,paths)

// declare general folder type
#define ASSET_DIR &(entry_dir)

// declare general file type
#define ASSET_FILE &(entry_file)

// declare general sub folder type
#define ASSET_SUBDIRS .subdirs=(entry_dir*[])

// declare general sub files type
#define ASSET_SUBFILES .subfiles=(entry_file*[])

// declare simple entry metadata
#define ASSET_SIMPLE_INFO(_name,_mode).info.name=(_name),.info.mode=_mode

// declare simple folder
#define ASSET_SIMPLE_DIR(_name,_mode) ASSET_DIR{\
	ASSET_SIMPLE_INFO(_name,(_mode)|S_IFDIR),\
	.subdirs=NULL,\
	.subfiles=NULL\
}

// declare general symbolic link
#define ASSET_SYMLINK(link,dest) ASSET_FILE{\
	ASSET_SIMPLE_INFO(link,0755|S_IFLNK),\
	.content=(dest),\
}

// declare general file
#define ASSET_SIMPLE_FILE(_name,_mode,_content) ASSET_FILE{\
	ASSET_SIMPLE_INFO(_name,(_mode)|S_IFREG),\
	.content=(_content),\
}

// declare general file with file length
#define ASSET_SIMPLE_SFILE(_name,_mode,_content,_length) ASSET_FILE{\
	ASSET_SIMPLE_INFO(_name,(_mode)|S_IFREG),\
	.content=(char*)(_content),\
        .length=(_length)\
}
#endif
