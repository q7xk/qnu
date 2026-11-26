#ifndef QNU_BLK_H
#define QNU_BLK_H
#include <qnu/types.h>
#define BLK_SECTOR 512
typedef struct blk_dev {
    char name[16];
    u64 sectors;
    int (*read)(struct blk_dev*, u64 lba, u32 count, void *buf);
    int (*write)(struct blk_dev*, u64 lba, u32 count, const void *buf);
    void *priv;
} blk_dev_t;
int blk_register(blk_dev_t *dev);
blk_dev_t *blk_get(const char *name);
int blk_read(const char *name, u64 lba, u32 count, void *buf);
void blk_init(void);
#endif
