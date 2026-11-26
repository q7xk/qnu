#include <qnu/blk.h>
#include <qnu/pci.h>
#include <qnu/mm.h>
#include <qnu/log.h>
#include <qnu/boot.h>
#include <qnu/types.h>

#define PAGE 4096
#define NVME_CC 0x14
#define NVME_CSTS 0x1C
#define NVME_AQA 0x24
#define NVME_ASQ 0x28
#define NVME_ACQ 0x30
#define NVME_CAP 0x00
#define NVME_CC_EN (1u<<0)
#define NVME_CSTS_RDY (1u<<0)

typedef struct { u32 opcode; u32 flags; u16 cid; u16 nsid; u64 rsv0; u64 mptr; u64 prp1; u64 prp2; u32 cdw10; u32 cdw11; u32 cdw12; u32 cdw13; u32 cdw14; u32 cdw15; } __attribute__((packed)) nvme_sqe_t;
typedef struct { u32 result; u32 rsv0; u16 sqhead; u16 sqid; u16 cid; u16 status; } __attribute__((packed)) nvme_cqe_t;

typedef struct {
    volatile u32 *mmio;
    nvme_sqe_t *asq, *iosq; nvme_cqe_t *acq, *iocq; u8 *dma;
    u16 asq_tail, acq_head, iosq_tail, iocq_head; u16 aq_id; u32 nsid; u32 cid;
    u8 *bounce;
} nvme_ctrl_t;

static blk_dev_t nvme_devs[8]; static int nvme_count=0; static u64 hhdm;

static inline u32 rd32(volatile u32 *m,u32 off){ return m[off/4]; }
static inline void wr32(volatile u32 *m,u32 off,u32 v){ m[off/4]=v; }
static inline void wr64(volatile u32 *m,u32 off,u64 v){ m[off/4]=(u32)v; m[off/4+1]=(u32)(v>>32); }

static int nvme_wait_rdy(nvme_ctrl_t*c,int want){ int s=0; while(s++<1000000){ u32 csts=rd32(c->mmio,NVME_CSTS); if((csts&NVME_CSTS_RDY)==(u32)want) return 0; } return -1; }

static int nvme_admin(nvme_ctrl_t*c, nvme_sqe_t*cmd){
    u16 slot=c->asq_tail % 64; c->asq[slot]=*cmd; c->asq[slot].cid=c->cid++;
    __asm__ volatile("":::"memory"); c->asq_tail=(c->asq_tail+1)%64; wr32(c->mmio, 0x1000, c->asq_tail); /* SQ0 tail doorbell */
    int s=0; while(s++<1000000){ nvme_cqe_t*cq=&c->acq[c->acq_head%64]; if((cq->status & 1) == c->aq_id){ /* check status */ u16 st=cq->status>>1; c->acq_head=(c->acq_head+1)%64; wr32(c->mmio, 0x1000+1*4, c->acq_head); /* CQ0 head doorbell */ return (st==0)?0:-1; } }
    return -1;
}

static int nvme_rw(nvme_ctrl_t*c,u64 lba,u32 count,u8*buf,int write){
    for(u32 i=0;i<count;i++){
        if(write) for(int j=0;j<512;j++) c->bounce[j]=buf[i*512+j];
        u64 phys=(u64)c->bounce - hhdm;
        nvme_sqe_t cmd; for(int k=0;k<(int)sizeof(cmd);k++) ((u8*)&cmd)[k]=0;
        cmd.opcode=write?1:2; cmd.nsid=c->nsid; cmd.prp1=phys; cmd.cdw10=(u32)(lba+i)&0xFFFFFFFF; cmd.cdw11=(u32)((lba+i)>>32); cmd.cdw12=0; /* nlb-1=0 */
        /* submit I/O sq1 */
        u16 slot=c->iosq_tail % 64; c->iosq[slot]=cmd; c->iosq[slot].cid=c->cid++; __asm__ volatile("":::"memory"); c->iosq_tail=(c->iosq_tail+1)%64; wr32(c->mmio, 0x1000+2*4, c->iosq_tail); /* SQ1 tail */
        int s=0; while(s++<1000000){ nvme_cqe_t*cq=&c->iocq[c->iocq_head%64]; if(cq->cid==c->iosq[slot].cid && (cq->status&1)==1){ u16 st=cq->status>>1; c->iocq_head=(c->iocq_head+1)%64; wr32(c->mmio,0x1000+3*4,c->iocq_head); if(!write) for(int j=0;j<512;j++) buf[i*512+j]=c->bounce[j]; if(st!=0) return -1; break; } } if(s>=1000000) return -1;
    }
    return 0;
}

static int nvme_read(blk_dev_t*d,u64 lba,u32 count,void*buf){ return nvme_rw((nvme_ctrl_t*)d->priv,lba,count,(u8*)buf,0); }
static int nvme_write(blk_dev_t*d,u64 lba,u32 count,const void*buf){ return nvme_rw((nvme_ctrl_t*)d->priv,lba,count,(u8*)buf,1); }

void nvme_init(void){
    hhdm=boot_info_get()->hhdm_offset; nvme_count=0;
    for(u16 bus=0;bus<256;bus++) for(u8 dev=0;dev<32;dev++) for(u8 fn=0;fn<8;fn++){
        u32 id=pci_read32((u8)bus,dev,fn,0); if((id&0xFFFF)==0xFFFF) continue;
        u32 cls=pci_read32((u8)bus,dev,fn,0x08); u8 cclass=(cls>>24)&0xFF, sub=(cls>>16)&0xFF, progif=(cls>>8)&0xFF;
        if(!(cclass==0x01 && sub==0x08 && progif==0x02)) continue; /* NVMe */
        u32 bar0=pci_read32((u8)bus,dev,fn,0x10); if(bar0&1) continue; u64 mmio=bar0 & ~0xFULL; vmm_map(mmio,mmio,0x3);
        if(nvme_count>=8) break;
        nvme_ctrl_t *c=(nvme_ctrl_t*)kmalloc(sizeof(nvme_ctrl_t)); if(!c) continue;
        for(int k=0;k<(int)sizeof(*c);k++) ((u8*)c)[k]=0; c->mmio=(volatile u32*)mmio; c->nsid=1; c->aq_id=1;
        /* disable */
        u32 cc=rd32(c->mmio,NVME_CC); wr32(c->mmio,NVME_CC, cc & ~NVME_CC_EN); nvme_wait_rdy(c,0);
        /* allocate admin + io queues */
        u8 *mem=(u8*)pmm_alloc_pages(8); if(!mem) continue; u64 mphys=(u64)mem-hhdm;
        c->asq=(nvme_sqe_t*)(mem+0*PAGE); c->acq=(nvme_cqe_t*)(mem+1*PAGE); c->iosq=(nvme_sqe_t*)(mem+2*PAGE); c->iocq=(nvme_cqe_t*)(mem+3*PAGE); c->dma=mem+4*PAGE; c->bounce=mem+5*PAGE;
        wr32(c->mmio,NVME_AQA, (63) | (63<<16)); wr64(c->mmio,NVME_ASQ, mphys+0*PAGE); wr64(c->mmio,NVME_ACQ, mphys+1*PAGE);
        /* enable */
        cc=rd32(c->mmio,NVME_CC); cc &= ~(0xF<<4); cc |= (6<<4)|(4<<20); /* IOSQES=6 IOCQES=4 */ cc |= NVME_CC_EN; wr32(c->mmio,NVME_CC,cc);
        if(nvme_wait_rdy(c,1)<0){ log_write("[nvme] not ready\n"); continue; }
        /* identify controller (best effort) */
        nvme_sqe_t idcmd; for(int k=0;k<(int)sizeof(idcmd);k++) ((u8*)&idcmd)[k]=0; idcmd.opcode=0x06; idcmd.nsid=0; idcmd.prp1=(u64)c->dma - hhdm; idcmd.cdw10=1; nvme_admin(c,&idcmd);
        /* create IO CQ + SQ */
        nvme_sqe_t cq; for(int k=0;k<(int)sizeof(cq);k++) ((u8*)&cq)[k]=0; cq.opcode=0x05; cq.prp1=mphys+3*PAGE; cq.cdw10=1 | (63<<16); cq.cdw11=1; nvme_admin(c,&cq);
        nvme_sqe_t sq; for(int k=0;k<(int)sizeof(sq);k++) ((u8*)&sq)[k]=0; sq.opcode=0x01; sq.prp1=mphys+2*PAGE; sq.cdw10=1 | (63<<16); sq.cdw11=1 | (1<<16); sq.cdw12=0; nvme_admin(c,&sq);
        /* register block */
        blk_dev_t*d=&nvme_devs[nvme_count]; d->name[0]='n';d->name[1]='v';d->name[2]='m';d->name[3]='e';d->name[4]='0'+nvme_count;d->name[5]=0; d->sectors=0; d->read=nvme_read; d->write=nvme_write; d->priv=c; blk_register(d); nvme_count++;
        log_printf("[OK] NVMe initialized (%s)\n", d->name);
    }
    if(nvme_count==0) log_write("[NVMe] no NVMe devices\n");
}
