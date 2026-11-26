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

static void wr16(u8*p,u16 v){ p[0]=v&0xFF; p[1]=(v>>8)&0xFF; }
static void wr32(u8*p,u32 v){ p[0]=v&0xFF; p[1]=(v>>8)&0xFF; p[2]=(v>>16)&0xFF; p[3]=(v>>24)&0xFF; }
static int write_sec(fat_t*f,u64 lba,const void*buf){ blk_dev_t*d=blk_get(f->blk); if(!d||!d->write) return -1; return d->write(d,lba,1,buf); }
static int fat_set(fat_t*f,u32 clus,u32 val){
    u32 off=clus*4; u64 sector=f->first+f->rsvd+(off/512); u8 sec[512]; if(read_sec(f,sector,sec)<0) return -1;
    wr32(sec+(off%512), (rd32(sec+(off%512))&0xF0000000)|(val&0x0FFFFFFF));
    return write_sec(f,sector,sec);
}
static u32 alloc_cluster(fat_t*f){
    u64 total_fat_sec=f->fatsz; for(u64 s=0;s<total_fat_sec;s++){ u8 sec[512]; if(read_sec(f,f->first+f->rsvd+s,sec)<0) return 0; for(u32 o=0;o+4<=512;o+=4){ u32 v=rd32(sec+o)&0x0FFFFFFF; if(v==0){ u32 clus=(u32)((s*512+o)/4); if(clus>=2){ fat_set(f,clus,0x0FFFFFFF); return clus; } } } }
    return 0;
}
int fat32_write_file(const char *blk,u64 first_lba,const char *name,const void *buf,u64 len){
    fat_t f; u8 sec[512]; f.blk=blk; f.first=first_lba;
    if(read_sec(&f,first_lba,sec)<0) return -1; f.bps=rd16(sec+11); f.spc=sec[13]; f.rsvd=rd16(sec+14); f.nfats=sec[16]; f.fatsz=rd32(sec+36); f.root_clus=rd32(sec+44);
    if(f.bps!=512||f.spc==0||f.fatsz==0) return -1;
    char want[16]; upper(want,name,16);
    /* read root cluster */
    u8 *clbuf=(u8*)kmalloc((u64)f.spc*512); if(!clbuf) return -1;
    if(read_cluster(&f,f.root_clus,clbuf)<0){kfree(clbuf);return -1;}
    u32 ent_off=0; bool found=false; u32 old_clus=0;
    for(u32 off=0;off+32<=(u32)f.spc*512;off+=32){ u8*e=clbuf+off; if(e[0]==0){ ent_off=off; break; } if(e[0]==0xE5){ ent_off=off; break; } if(e[11]==0x0F) continue; char n[16]; int ni=0; for(int i=0;i<8&&e[i]!=' ';i++)n[ni++]=e[i]; if(e[8]!=' '){n[ni++]='.';for(int i=8;i<11&&e[i]!=' ';i++)n[ni++]=e[i];} n[ni]=0; upper(n,n,16); int ok=1;for(int wi=0;n[wi]||want[wi];wi++)if(n[wi]!=want[wi]){ok=0;break;} if(ok){ found=true; ent_off=off; old_clus=rd16(e+26)|((u32)rd16(e+20)<<16); break; } }
    /* free old chain */
    u32 c=old_clus; int guard=0; while(c>=2 && c<0x0FFFFFF8 && guard++<10000){ u32 n=fat_next(&f,c); fat_set(&f,c,0); c=n; }
    /* allocate chain and write data */
    u32 first=0, prev=0; u64 written=0; u32 clus_bytes=(u32)f.spc*512;
    u8 *zero=(u8*)kmalloc(clus_bytes); if(!zero){kfree(clbuf);return -1;} for(u32 i=0;i<clus_bytes;i++)zero[i]=0;
    while(written<len){
        u32 clus=alloc_cluster(&f); if(!clus){kfree(zero);kfree(clbuf);return -1;}
        if(!first) first=clus; if(prev) fat_set(&f,prev,clus);
        u64 lba=f.first + f.rsvd + (u64)f.nfats*f.fatsz + (u64)(clus-2)*f.spc;
        u32 chunk=clus_bytes; if(chunk>len-written) chunk=(u32)(len-written);
        /* write cluster */
        u8 *tmp=(u8*)kmalloc(clus_bytes); if(!tmp){kfree(zero);kfree(clbuf);return -1;} for(u32 i=0;i<clus_bytes;i++) tmp[i]=zero[i];
        for(u32 i=0;i<chunk;i++) tmp[i]=((const u8*)buf)[written+i];
        blk_dev_t*d=blk_get(f.blk); if(d&&d->write){ for(u32 s=0;s<f.spc;s++) d->write(d,lba+s,1,tmp+s*512); }
        kfree(tmp); written+=chunk; prev=clus;
    }
    kfree(zero);
    if(prev) fat_set(&f,prev,0x0FFFFFFF);
    /* write directory entry */
    u8*e=clbuf+ent_off; for(int i=0;i<32;i++) e[i]=0; int ni=0; for(int i=0;i<11;i++) e[i]=' ';
    /* 8.3 */
    int dot=-1; for(int i=0;want[i];i++) if(want[i]=='.') dot=i;
    if(dot<0){ for(int i=0;want[i]&&i<8;i++) e[i]=want[i]; } else { for(int i=0;i<dot&&i<8;i++) e[i]=want[i]; for(int i=0;dot+1+i<(int)16&&want[dot+1+i]&&i<3;i++) e[8+i]=want[dot+1+i]; }
    e[11]=0x20; wr16(e+26,first&0xFFFF); wr16(e+20,(first>>16)&0xFFFF); wr32(e+28,(u32)len);
    /* write root cluster back */
    u64 lba=f.first + f.rsvd + (u64)f.nfats*f.fatsz + (u64)(f.root_clus-2)*f.spc;
    blk_dev_t*d=blk_get(f.blk); if(d&&d->write){ for(u32 s=0;s<f.spc;s++) d->write(d,lba+s,1,clbuf+s*512); }
    kfree(clbuf); return 0;
}

int fat32_list_root(const char *blk,u64 first_lba,char names[][64],int max){
    fat_t f; u8 sec[512]; f.blk=blk; f.first=first_lba; if(read_sec(&f,first_lba,sec)<0) return -1;
    f.bps=rd16(sec+11); f.spc=sec[13]; f.rsvd=rd16(sec+14); f.nfats=sec[16]; f.fatsz=rd32(sec+36); f.root_clus=rd32(sec+44); if(f.bps!=512||f.spc==0||f.fatsz==0) return -1;
    u8 *clbuf=(u8*)kmalloc((u64)f.spc*512); if(!clbuf) return -1; if(read_cluster(&f,f.root_clus,clbuf)<0){kfree(clbuf);return -1;}
    int n=0; for(u32 off=0;off+32<=(u32)f.spc*512 && n<max;off+=32){ u8*e=clbuf+off; if(e[0]==0) break; if(e[0]==0xE5) continue; if(e[11]==0x0F) continue; if(e[11]&0x10) continue; int ni=0; for(int i=0;i<8&&e[i]!=' ';i++) names[n][ni++]=e[i]; if(e[8]!=' '){ names[n][ni++]='.'; for(int i=8;i<11&&e[i]!=' ';i++) names[n][ni++]=e[i]; } names[n][ni]=0; n++; }
    kfree(clbuf); return n;
}
