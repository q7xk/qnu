#ifndef QNU_BLK_CACHE_H
#define QNU_BLK_CACHE_H
#include <qnu/types.h>
int blk_cache_read(const char *name,u64 lba,u32 count,void*buf);
void blk_cache_flush(void);
#endif
