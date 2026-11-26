#ifndef QNU_FAT32_H
#define QNU_FAT32_H
#include <qnu/types.h>
int fat32_read_file(const char *blk,u64 first_lba,const char *name,void *buf,u64 max,u64 *out_size);
int fat32_fsck(const char *blk,u64 first_lba);
#endif
