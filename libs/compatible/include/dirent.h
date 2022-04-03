#ifndef SIMPLE_INIT_DIRENT_H
#define SIMPLE_INIT_DIRENT_H
#include "compatible.h"
#include "sys/types.h"
enum d_type{
	DT_UNKNOWN = 0,
	DT_FIFO    = 1,
	DT_CHR     = 2,
	DT_DIR     = 4,
	DT_BLK     = 6,
	DT_REG     = 8,
	DT_LNK     = 10,
	DT_SOCK    = 12,
	DT_WHT     = 14
};
#define DT_UNKNOWN DT_UNKNOWN
#define DT_FIFO    DT_FIFO
#define DT_CHR     DT_CHR
#define DT_DIR     DT_DIR
#define DT_BLK     DT_BLK
#define DT_REG     DT_REG
#define DT_LNK     DT_LNK
#define DT_SOCK    DT_SOCK
#define DT_WHT     DT_WHT
struct dirent{
	ino_t d_ino;
    	off_t d_off;
	unsigned short int d_reclen;
	enum d_type d_type;
	char d_name[256];
};
#endif //SIMPLE_INIT_DIRENT_H
