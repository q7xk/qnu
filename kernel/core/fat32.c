#include <qnu/fat32.h>
#include <qnu/blk.h>
#include <qnu/blk_cache.h>
#include <qnu/log.h>
#include <qnu/mm.h>
static u16 rd16(const u8*p){return p[0]|(p[1]<<8);}
static u32 rd32(const u8*p){return p[0]|(p[1]<<8)|(p[2]<<16)|((u32)p[3]<<24);}
typedef struct { u16 bps,spc,rsvd,nfats,fatsz,root_clus; u64 first; const char *blk; } fat_t;
static int read_sec(fat_t*f,u64 lba,void*buf){ return blk_cache_read(f->blk,lba,1,buf); }
static u32 fat_next(fat_t*f,u32 clus){
    u32 off=clus*4; u64 sector=f->first+f->rsvd+(off/512); u8 sec[512]; if(read_sec(f,sector,sec)<0) return 0x0FFFFFFF; return rd32(sec+(off%512)) & 0x0FFFFFFF;
}
static int read_cluster(fat_t*f,u32 clus,void*buf){
    u64 lba=f->first + f->rsvd + (u64)f->nfats*f->fatsz + (u64)(clus-2)*f->spc;
    return blk_cache_read(f->blk,lba,f->spc,buf);
}
static void upper(char*d,const char*s,int max){int i=0;for(;s[i]&&i<max-1;i++){char c=s[i];if(c>='a'&&c<='z')c-=32;d[i]=c;}d[i]=0;}
int fat32_read_file(const char *blk,u64 first_lba,const char *name,void *buf,u64 max,u64 *out_size){
    fat_t f; u8 sec[512]; f.blk=blk; f.first=first_lba;
    if(read_sec(&f,first_lba,sec)<0) return -1;
    f.bps=rd16(sec+11); f.spc=sec[13]; f.rsvd=rd16(sec+14); f.nfats=sec[16]; f.fatsz=rd32(sec+36); f.root_clus=rd32(sec+44);
    if(f.bps!=512 || f.spc==0 || f.fatsz==0) return -1;
    char want[16]; upper(want,name,16);
    u32 clus=f.root_clus; int guard=0; u8 *clbuf=(u8*)kmalloc((u64)f.spc*512); if(!clbuf) return -1;
    int found=0; u32 file_clus=0; u32 file_size=0;
    while(clus>=2 && clus<0x0FFFFFF8 && guard++<1000){
        if(read_cluster(&f,clus,clbuf)<0) break;
        for(u32 off=0;off+32<=(u32)f.spc*512;off+=32){
            u8*e=clbuf+off; if(e[0]==0) break; if(e[0]==0xE5) continue; if(e[11]==0x0F) continue; /* skip LFN */
            char n[16]; int ni=0; for(int i=0;i<8&&e[i]!=' ';i++) n[ni++]=e[i]; if(e[8]!=' '){ n[ni++]='.'; for(int i=8;i<11&&e[i]!=' ';i++) n[ni++]=e[i]; } n[ni]=0; upper(n,n,16);
            { int ok=1; int wi=0; for(;n[wi]||want[wi];wi++){ if(n[wi]!=want[wi]){ok=0;break;} } if(ok){ file_clus=rd16(e+26)|((u32)rd16(e+20)<<16); file_size=rd32(e+28); found=1; break; } }
        }
        if(found) break; clus=fat_next(&f,clus);
    }
    if(!found){ kfree(clbuf); return -1; }
    u64 copied=0; clus=file_clus; guard=0;
    u8 *one=(u8*)kmalloc((u64)f.spc*512); if(!one){kfree(clbuf);return -1;}
    while(clus>=2 && clus<0x0FFFFFF8 && copied<file_size && copied<max && guard++<10000){
        if(read_cluster(&f,clus,one)<0) break;
        u32 chunk=(u32)f.spc*512; if(chunk>file_size-copied) chunk=(u32)(file_size-copied); if(chunk>max-copied) chunk=(u32)(max-copied);
        u8*dst=(u8*)buf+copied; for(u32 i=0;i<chunk;i++) dst[i]=one[i]; copied+=chunk; clus=fat_next(&f,clus);
    }
    kfree(one); kfree(clbuf); if(out_size)*out_size=file_size; return 0;
}
int fat32_fsck(const char *blk,u64 first_lba){
    u8 sec[512]; if(blk_cache_read(blk,first_lba,1,sec)<0) return -1;
    if(sec[510]!=0x55||sec[511]!=0xAA){ log_write("[fsck] bad boot sector signature\n"); return -1; }
    if(rd16(sec+11)!=512){ log_write("[fsck] unsupported bytes/sector\n"); return -1; }
    if(rd32(sec+36)==0){ log_write("[fsck] not FAT32\n"); return -1; }
    log_write("[fsck] FAT32 basic checks passed (full fsck not implemented)\n"); return 0;
}
