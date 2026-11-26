#include <qnu/blk_cache.h>
#include <qnu/blk.h>
#include <qnu/log.h>
#define CACHE_N 32
typedef struct { char name[16]; u64 lba; u8 data[512]; bool valid; u64 age; } cache_ent_t;
static cache_ent_t cache[CACHE_N]; static u64 tick=0;
static int nameeq(const char*a,const char*b){int i=0;while(a[i]&&b[i]){if(a[i]!=b[i])return 0;i++;}return a[i]==b[i];}
int blk_cache_read(const char *name,u64 lba,u32 count,void*buf){
    for(u32 i=0;i<count;i++){
        u8 *dst=(u8*)buf+i*512; bool hit=false;
        for(int c=0;c<CACHE_N;c++){ if(cache[c].valid && cache[c].lba==lba+i && nameeq(cache[c].name,name)){ for(int j=0;j<512;j++)dst[j]=cache[c].data[j]; cache[c].age=++tick; hit=true; break; } }
        if(hit) continue;
        if(blk_read(name,lba+i,1,dst)<0) return -1;
        int slot=0; u64 min=~0ULL; for(int c=0;c<CACHE_N;c++){ if(!cache[c].valid){slot=c;break;} if(cache[c].age<min){min=cache[c].age;slot=c;} }
        cache[slot].valid=true; cache[slot].lba=lba+i; cache[slot].age=++tick; int k=0; for(;k<15&&name[k];k++)cache[slot].name[k]=name[k]; cache[slot].name[k]=0; for(int j=0;j<512;j++)cache[slot].data[j]=dst[j];
    }
    return 0;
}
void blk_cache_flush(void){ for(int i=0;i<CACHE_N;i++) cache[i].valid=false; }
