#include <qnu/blk.h>
#include <qnu/log.h>
#define MAX_BLK 8
static blk_dev_t *devs[MAX_BLK]; static int count=0;
int blk_register(blk_dev_t *dev){ if(count>=MAX_BLK) return -1; devs[count++]=dev; log_printf("[blk] registered %s sectors=%lu\n", dev->name, dev->sectors); return 0; }
blk_dev_t *blk_get(const char *name){ for(int i=0;i<count;i++){ int j=0; while(devs[i]->name[j]&&name[j]&&devs[i]->name[j]==name[j])j++; if(devs[i]->name[j]==0&&name[j]==0) return devs[i]; } return 0; }
int blk_read(const char *name,u64 lba,u32 count_,void*buf){ blk_dev_t*d=blk_get(name); if(!d||!d->read) return -1; return d->read(d,lba,count_,buf); }
void blk_init(void){ log_ok("Block layer initialized"); }

int blk_read_async(const char *name,u64 lba,u32 count,void *buf,blk_async_cb cb,void *arg){ int r=blk_read(name,lba,count,buf); if(cb) cb(r,arg); return r; }
