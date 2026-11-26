#include <qnu/blk.h>
#include <qnu/mm.h>
#include <qnu/log.h>
static blk_dev_t rd;
static u8 *data;
static int rd_read(blk_dev_t*d,u64 lba,u32 count,void*buf){ (void)d; for(u32 i=0;i<count;i++){ u8*dst=(u8*)buf+i*512; u8*src=data+(lba+i)*512; for(int j=0;j<512;j++) dst[j]=src[j]; } return 0; }
static int rd_write(blk_dev_t*d,u64 lba,u32 count,const void*buf){ (void)d; for(u32 i=0;i<count;i++){ u8*src=(u8*)buf+i*512; u8*dst=data+(lba+i)*512; for(int j=0;j<512;j++) dst[j]=src[j]; } return 0; }
void ramdisk_init(void){ u64 size=1024*1024; data=(u8*)kmalloc(size); if(!data) return; for(u64 i=0;i<size;i++) data[i]=0; rd=(blk_dev_t){.name="ram0",.sectors=size/512,.read=rd_read,.write=rd_write,.priv=0}; blk_register(&rd); }
