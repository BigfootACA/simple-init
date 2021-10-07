#ifdef ENABLE_GUI
#define _GNU_SOURCE
#include<errno.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<dirent.h>
#include<sys/stat.h>
#include<sys/statfs.h>
#include"lvgl.h"
#include"logger.h"
#include"defines.h"
#include"gui/fsext.h"
#define TAG "filesystem"
bool fsext_is_multi=false;

struct fs_root{
	char*root;
	int fd;
	bool debug;
};

static lv_res_t errno_to_lv_res(int err){
	switch(err){
		case 0:return LV_FS_RES_OK;
		case EACCES:
		case EPERM:
		case EROFS:
		case ENOKEY:
		case EISDIR:
		case ENOTDIR:
		case ECONNREFUSED:
			return LV_FS_RES_DENIED;
		case ENXIO:
		case ENOENT:
		case ELOOP:
		case EHOSTUNREACH:
			return LV_FS_RES_NOT_EX;
		case ENFILE:
		case EMFILE:
		case ETXTBSY:
		case ENETDOWN:
		case ECONNABORTED:
		case ECONNRESET:
		case EINPROGRESS:
			return LV_FS_RES_BUSY;
		case EBADF:
		case EINVAL:
		case ESPIPE:
		case ENAMETOOLONG:
			return LV_FS_RES_INV_PARAM;
		case EOVERFLOW:
		case ENOMEM:
		case ENOSR:
		case ERANGE:
		case EFBIG:
		case ENOBUFS:
			return LV_FS_RES_OUT_OF_MEM;
		case EPIPE:
		case EUCLEAN:
		case ENETRESET:
		case ENETUNREACH:
			return LV_FS_RES_FS_ERR;
		case ETIME:
		case ETIMEDOUT:
			return LV_FS_RES_TOUT;
		case ENOSPC:
		case EDQUOT:
			return LV_FS_RES_FULL;
		case EAGAIN:
			return LV_FS_RES_LOCKED;
		case EPROTONOSUPPORT:
		case ESOCKTNOSUPPORT:
		case ENOSYS:
		case ENOTSUP:
			return LV_FS_RES_NOT_IMP;
		case EFAULT:
		case EDEADLK:
		case ECOMM:
		case EPROTO:
		case EBADMSG:
		case EMEDIUMTYPE:
		case ENOMEDIUM:
		case EIO:
			return LV_FS_RES_HW_ERR;
		default:
			return LV_FS_RES_UNKNOWN;
	}
}

static bool fs_ready_cb(struct _lv_fs_drv_t*drv){
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(fs->fd>=0)return true;
	errno=0;
	fs->fd=openat(AT_FDCWD,fs->root,O_DIRECTORY|O_RDONLY);
	if(fs->debug){
		if(fs->fd<0)telog_warn(
			"ready open %s (%c)",
			fs->root,drv->letter
		);
		else tlog_debug(
			"%s (%c) ready",
			fs->root,drv->letter
		);
	}
	return errno==0&&fs->fd>=0;
}

static lv_res_t fs_open_cb(
	struct _lv_fs_drv_t*drv,
	void*file_p,
	const char*path,
	lv_fs_mode_t mode
){
	int flags,fd;
	if(!drv||!file_p||!path)return LV_FS_RES_INV_PARAM;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs)return LV_FS_RES_INV_PARAM;
	flags=O_CLOEXEC;
	switch(mode){
		case LV_FS_MODE_RD:flags|=O_RDONLY;break;
		case LV_FS_MODE_WR:flags|=O_RDWR|O_CREAT;break;
		default:return LV_FS_RES_INV_PARAM;
	}
	do{errno=0;fd=openat(fs->fd,path,flags);}
	while(fd<0&&errno==EINTR);
	if(fs->debug&&fd<0)telog_warn(
		"%s: open %c:%s mode %d",
		fs->root,
		drv->letter,
		path,mode
	);
	if(fd>=0)*(int*)((lv_fs_file_t*)file_p)=fd;
	return errno_to_lv_res(errno);
}

static lv_res_t fs_close_cb(
	struct _lv_fs_drv_t*drv,
	void*file_p
){
	if(!drv||!file_p)return LV_FS_RES_INV_PARAM;
	int fd=*(int*)((lv_fs_file_t*)file_p);
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs)return LV_FS_RES_INV_PARAM;
	do{errno=0;close(fd);}
	while(errno==EINTR);
	if(fs->debug&&errno>0)telog_warn(
		"%s: close %c:#%d",
		fs->root,drv->letter,fd
	);
	return errno_to_lv_res(errno);
}

static lv_res_t fs_remove_cb(
	struct _lv_fs_drv_t*drv,
	const char*fn
){
	if(!drv||!fn)return LV_FS_RES_INV_PARAM;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs)return LV_FS_RES_INV_PARAM;
	do{errno=0;unlinkat(fs->fd,fn,0);}
	while(errno==EINTR);
	if(fs->debug&&errno>0)telog_warn(
		"%s: unlink %c:%s",
		fs->root,drv->letter,fn
	);
	return errno_to_lv_res(errno);
}

static lv_res_t fs_get_type_cb(
	struct _lv_fs_drv_t*drv,
	const char*fn,
	enum item_type*type
){
	if(!drv||!fn||!type)return LV_FS_RES_INV_PARAM;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs)return LV_FS_RES_INV_PARAM;
	struct stat st;
	do{errno=0;fstatat(fs->fd,fn,&st,AT_SYMLINK_NOFOLLOW);}
	while(errno==EINTR);
	if(errno>0){
		if(fs->debug)telog_warn(
			"%s: stat %c:%s",
			fs->root,drv->letter,fn
		);
		return errno_to_lv_res(errno);
	}
	switch(st.st_mode&S_IFMT){
		case S_IFDIR:*type=TYPE_DIR;break;
		case S_IFREG:*type=TYPE_FILE;break;
		case S_IFIFO:*type=TYPE_FIFO;break;
		case S_IFLNK:*type=TYPE_LINK;break;
		case S_IFCHR:*type=TYPE_CHAR;break;
		case S_IFBLK:*type=TYPE_BLOCK;break;
		case S_IFSOCK:*type=TYPE_SOCK;break;
		default:return LV_FS_RES_UNKNOWN;
	}
	return LV_FS_RES_OK;
}

static bool fs_is_dir_cb(
	struct _lv_fs_drv_t*drv,
	const char*fn
){
	if(!drv||!fn)return LV_FS_RES_INV_PARAM;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs)return LV_FS_RES_INV_PARAM;
	struct stat st={0};
	do{errno=0;fstatat(fs->fd,fn,&st,0);}
	while(errno==EINTR);
	if(errno>0){
		if(fs->debug)telog_warn(
			"%s: stat  %c:%s",
			fs->root,drv->letter,fn
		);
		return false;
	}
	return S_ISDIR(st.st_mode);
}

static lv_res_t fs_read_cb(
	struct _lv_fs_drv_t*drv,
	void*file_p,
	void*buf,
	uint32_t btr,
	uint32_t*br
){
	if(!drv||!file_p||!buf||!br)return LV_FS_RES_INV_PARAM;
	int fd=*(int*)((lv_fs_file_t*)file_p);
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs)return LV_FS_RES_INV_PARAM;
	ssize_t ret;
	do{errno=0;ret=read(fd,buf,btr);}
	while(ret<0&&errno==EINTR);
	if(fs->debug&&errno>0)telog_warn(
		"%s: read %c:#%d %d",
		fs->root,drv->letter,fd,btr
	);
	if(ret>=0)*br=ret;
	return errno_to_lv_res(errno);
}

static lv_res_t fs_write_cb(
	struct _lv_fs_drv_t*drv,
	void*file_p,
	const void*buf,
	uint32_t btw,
	uint32_t*bw
){
	if(!drv||!file_p||!buf||!bw)return LV_FS_RES_INV_PARAM;
	int fd=*(int*)((lv_fs_file_t*)file_p);
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs)return LV_FS_RES_INV_PARAM;
	ssize_t ret;
	do{errno=0;ret=write(fd,buf,btw);}
	while(ret<0&&errno==EINTR);
	if(fs->debug&&errno>0)telog_warn(
		"%s: write %c:#%d %d",
		fs->root,drv->letter,fd,btw
	);
	if(ret>=0)*bw=ret;
	return errno_to_lv_res(errno);
}

static lv_res_t fs_seek_cb(
	struct _lv_fs_drv_t*drv,
	void*file_p,
	uint32_t pos
){
	if(!drv||!file_p)return LV_FS_RES_INV_PARAM;
	int fd=*(int*)((lv_fs_file_t*)file_p),ret;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs)return LV_FS_RES_INV_PARAM;
	do{errno=0;ret=lseek(fd,pos,SEEK_SET);}
	while(ret<0&&errno==EINTR);
	if(fs->debug&&errno>0)telog_warn(
		"%s: lseek %c:#%d %d",
		fs->root,drv->letter,fd,pos
	);
	return errno_to_lv_res(errno);
}

static lv_res_t fs_tell_cb(
	struct _lv_fs_drv_t*drv,
	void*file_p,
	uint32_t*pos_p
){
	if(!drv||!file_p||!pos_p)return LV_FS_RES_INV_PARAM;
	int fd=*(int*)((lv_fs_file_t*)file_p);
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs)return LV_FS_RES_INV_PARAM;
	off_t ret;
	do{errno=0;ret=lseek(fd,0,SEEK_CUR);}
	while(ret<0&&errno==EINTR);
	if(fs->debug&&errno>0)telog_warn(
		"%s: tell %c:#%d",
		fs->root,drv->letter,fd
	);
	if(ret>=0)*pos_p=ret;
	return errno_to_lv_res(errno);
}

static lv_res_t fs_trunc_cb(
	struct _lv_fs_drv_t*drv,
	void*file_p
){
	if(!drv||!file_p)return LV_FS_RES_INV_PARAM;
	int fd=*(int*)((lv_fs_file_t*)file_p),ret;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs)return LV_FS_RES_INV_PARAM;
	off_t cur;
	do{errno=0;cur=lseek(fd,0,SEEK_CUR);}
	while(cur<0&&errno==EINTR);
	if(fs->debug&&errno>0)telog_warn(
		"%s: trunc tell %c:#%d",
		fs->root,drv->letter,fd
	);
	if(cur<0)return errno_to_lv_res(errno);
	do{errno=0;ret=ftruncate(fd,cur);}
	while(ret<0&&errno==EINTR);
	if(fs->debug&&errno>0)telog_warn(
		"%s: trunc %c:#%d %ld",
		fs->root,drv->letter,fd,cur
	);
	return errno_to_lv_res(errno);
}

static lv_res_t fs_size_cb(
	struct _lv_fs_drv_t*drv,
	void*file_p,
	uint32_t*size_p
){
	if(!drv||!file_p||!size_p)return LV_FS_RES_INV_PARAM;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs)return LV_FS_RES_INV_PARAM;
	int fd=*(int*)((lv_fs_file_t*)file_p),ret;
	struct stat st;
	do{errno=0;ret=fstat(fd,&st);}
	while(ret<0&&errno==EINTR);
	if(fs->debug&&errno>0)telog_warn(
		"%s: stat %c:#%d",
		fs->root,drv->letter,fd
	);
	if(ret==0)*size_p=st.st_size;
	return errno_to_lv_res(errno);
}

static lv_res_t fs_rename_cb(
	struct _lv_fs_drv_t*drv,
	const char*oldname,
	const char*newname
){
	if(!drv||!oldname||!newname)return LV_FS_RES_INV_PARAM;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs)return LV_FS_RES_INV_PARAM;
	int ret;
	do{
		errno=0;
		ret=renameat(
			fs->fd,oldname,
			fs->fd,newname
		);
	}while(ret<0&&errno==EINTR);
	if(fs->debug&&errno>0)telog_warn(
		"%s: rename %c:%s %c:%s",
		fs->root,
		drv->letter,oldname,
		drv->letter,newname
	);
	return errno_to_lv_res(errno);
}

static lv_res_t fs_free_space_cb(
	struct _lv_fs_drv_t*drv,
	uint32_t*total_p,
	uint32_t*free_p
){
	if(!drv||!total_p||!free_p)return LV_FS_RES_INV_PARAM;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs)return LV_FS_RES_INV_PARAM;
	int ret;
	struct statfs st;
	do{errno=0;ret=fstatfs(fs->fd,&st);}
	while(ret<0&&errno==EINTR);
	if(fs->debug&&errno>0)telog_warn(
		"%s: statfs %c:",
		fs->root,drv->letter
	);
	if(ret==0){
		*total_p=st.f_bsize*st.f_frsize;
		*free_p=st.f_bavail&st.f_frsize;
	}
	return errno_to_lv_res(errno);
}

static lv_res_t fs_dir_open_cb(
	struct _lv_fs_drv_t*drv,
	void*rddir_p,
	const char*path
){
	if(!drv||!rddir_p||!path)return LV_FS_RES_INV_PARAM;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	if(!fs)return LV_FS_RES_INV_PARAM;
	int fd;
	DIR*d;
	if(path[0]==0)path="/";
	do{
		errno=0;
		fd=openat(fs->fd,path,O_DIRECTORY|O_RDONLY);
	}while(fd<0&&errno==EINTR);
	if(fs->debug&&errno>0)telog_warn(
		"%s: dir open %c:%s",
		fs->root,drv->letter,path
	);
	if(fd<0)return errno_to_lv_res(errno);
	do{errno=0;d=fdopendir(fd);}
	while(!d&&errno==EINTR);
	if(fs->debug&&errno>0)telog_warn(
		"%s: fdopendir %c:%s(%d)",
		fs->root,drv->letter,path,fd
	);
	if(d)((lv_fs_dir_t*)rddir_p)->dir_d=d;
	else close(fd);
	return errno_to_lv_res(errno);
}

static lv_res_t fs_dir_read_cb(
	struct _lv_fs_drv_t*drv,
	void*rddir_p,
	char*fn
){
	if(!drv||!rddir_p||!fn)return LV_FS_RES_INV_PARAM;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	DIR*dp=(DIR*)((lv_fs_dir_t*)rddir_p)->dir_d;
	if(!fs||!dp)return LV_FS_RES_INV_PARAM;
	int ret;
	struct dirent*d;
	for(;;){
		do{
			errno=0;
			d=readdir(dp);
		}while(!d&&errno==EINTR);
		ret=errno;
		if(fs->debug&&errno>0)telog_warn(
			"%s: readdir %c:#%p",
			fs->root,drv->letter,dp
		);
		if(d){
			if(strncmp(
				d->d_name,"..",
				strlen(d->d_name)
			)==0)continue;
			sprintf(
				fn,"%s%s",
				d->d_type==DT_DIR?"/":"",
				d->d_name
			);
		}else *fn=0;
		break;
	}
	return errno_to_lv_res(ret);
}

static lv_res_t fs_dir_close_cb(
	struct _lv_fs_drv_t*drv,
	void*rddir_p
){
	if(!drv||!rddir_p)return LV_FS_RES_INV_PARAM;
	struct fsext*fse=drv->user_data;
	struct fs_root*fs=fse->user_data;
	DIR*dp=(DIR*)((lv_fs_dir_t*)rddir_p)->dir_d;
	if(!fs||!dp)return LV_FS_RES_INV_PARAM;
	int ret;
	do{errno=0;ret=closedir(dp);}
	while(ret<0&&errno==EINTR);
	if(fs->debug&&errno>0)telog_warn(
		"%s: closedir %c:#%p",
		fs->root,drv->letter,dp
	);
	return errno_to_lv_res(errno);
}

int init_lvgl_fs(char letter,char*root,bool debug){
	lv_fs_drv_t*drv;
	struct fs_root*fs;
	struct fsext*fse;
	if(!root)ERET(EINVAL);
	if(!(fs=malloc(sizeof(struct fs_root))))return -errno;
	if(!(fse=malloc(sizeof(struct fsext))))return -errno;
	memset(fs,0,sizeof(struct fs_root));
	memset(fse,0,sizeof(struct fsext));
	if(!(fs->root=strdup(root))){
		free(fs);
		return -errno;
	}
	if(!(drv=malloc(sizeof(lv_fs_drv_t)))){
		free(fs->root);
		free(fs);
		return -errno;
	}
	fs->debug=debug;
	fs->fd=-1;
	lv_fs_drv_init(drv);
	fse->get_type_cb=fs_get_type_cb;
	fse->is_dir_cb=fs_is_dir_cb;
	fse->user_data=fs;
	drv->user_data=fse;
	drv->letter=letter;
	drv->file_size=sizeof(int);
	drv->rddir_size=sizeof(DIR*);
	drv->ready_cb=fs_ready_cb;
	drv->open_cb=fs_open_cb;
	drv->close_cb=fs_close_cb;
	drv->remove_cb=fs_remove_cb;
	drv->read_cb=fs_read_cb;
	drv->write_cb=fs_write_cb;
	drv->seek_cb=fs_seek_cb;
	drv->tell_cb=fs_tell_cb;
	drv->trunc_cb=fs_trunc_cb;
	drv->size_cb=fs_size_cb;
	drv->rename_cb=fs_rename_cb;
	drv->free_space_cb=fs_free_space_cb;
	drv->dir_open_cb=fs_dir_open_cb;
	drv->dir_read_cb=fs_dir_read_cb;
	drv->dir_close_cb=fs_dir_close_cb;
	lv_fs_drv_register(drv);
	return 0;
}
#endif
