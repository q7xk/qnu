#include <qnu/part.h>
#include <qnu/blk.h>
#include <qnu/log.h>
static u16 rd16(const u8*p){ return p[0] | (p[1]<<8); }
static u32 rd32(const u8*p){ return p[0] | (p[1]<<8) | (p[2]<<16) | ((u32)p[3]<<24); }
static u64 rd64(const u8*p){ return (u64)rd32(p) | ((u64)rd32(p+4)<<32); }
int part_scan_mbr(const char *blk, partition_t *parts, int max){
    u8 sec[512]; if(blk_read(blk,0,1,sec)<0) return -1; if(sec[510]!=0x55||sec[511]!=0xAA) return -1;
    int n=0; for(int i=0;i<4 && n<max;i++){ u8*e=sec+446+i*16; if(e[4]==0) continue; parts[n].type=e[4]; parts[n].first_lba=rd32(e+8); parts[n].sectors=rd32(e+12); parts[n].gpt=false; n++; }
    log_printf("[part] MBR entries=%d\n", n); return n;
}
int part_scan_gpt(const char *blk, partition_t *parts, int max){
    u8 hdr[512]; if(blk_read(blk,1,1,hdr)<0) return -1; if(hdr[0]!='E'||hdr[1]!='F'||hdr[2]!='I'||hdr[3]!=' '||hdr[4]!='P'||hdr[5]!='A'||hdr[6]!='R'||hdr[7]!='T') return -1;
    u64 ent_lba=rd64(hdr+72); u32 num=rd32(hdr+80); u32 entsz=rd32(hdr+84); int n=0;
    for(u32 i=0;i<num && n<max;i++){ u8 ent[512]; u64 off=ent_lba+(u64)(i*entsz/512); u32 within=(i*entsz)%512; if(blk_read(blk,off,1,ent)<0) break; u8*e=ent+within; /* type guid first 16 bytes, nonzero */ bool used=false; for(int k=0;k<16;k++) if(e[k]) used=true; if(!used) continue; parts[n].first_lba=rd64(e+32); u64 last=rd64(e+40); parts[n].sectors=last+1-parts[n].first_lba; parts[n].type=1; parts[n].gpt=true; n++; }
    log_printf("[part] GPT entries=%d\n", n); return n;
}
