#include <qnu/blk.h>
#include <qnu/pci.h>
#include <qnu/mm.h>
#include <qnu/log.h>
#include <qnu/boot.h>
#include <qnu/types.h>

#define PAGE 4096
#define HBA_PxCMD_ST 0x0001
#define HBA_PxCMD_FRE 0x0010
#define HBA_PxCMD_FR 0x4000
#define HBA_PxCMD_CR 0x8000
#define HBA_PxIS_TFES (1u<<30)
#define ATA_CMD_READ_DMA_EXT 0x25
#define ATA_CMD_WRITE_DMA_EXT 0x35
#define ATA_CMD_IDENTIFY 0xEC

typedef struct { u32 dba,dbau,rsv0,dbc; } __attribute__((packed)) hba_prdt_t;
typedef struct { u8 fis[64]; u8 atapi[16]; u8 rsv[48]; hba_prdt_t prdt[8]; } __attribute__((packed)) hba_cmd_table_t;
typedef struct { u16 flags,prdtl; u32 prdbc; u32 ctba,ctbau; u32 rsv1[4]; } __attribute__((packed)) hba_cmd_header_t;
typedef struct {
    u32 clb,clbu,fb,fbu,is,ie,cmd,rsv0,tfd,sig,ssts,sctl,serr,sact,ci,sntf,fbs; u32 rsv1[11]; u32 vendor[4];
} __attribute__((packed)) hba_port_t;
typedef struct { u32 cap,ghc,is,pi,vs,ccc_ctl,ccc_pts,em_loc,em_ctl,cap2,bohc; u8 rsv[0x100-0x2C]; hba_port_t ports[32]; } __attribute__((packed)) hba_mem_t;

static blk_dev_t ahci_devs[8]; static int ahci_count=0; static u64 hhdm;

static void stop_port(hba_port_t*p){ p->cmd &= ~HBA_PxCMD_ST; p->cmd &= ~HBA_PxCMD_FRE; int s=0; while((p->cmd & (HBA_PxCMD_FR|HBA_PxCMD_CR)) && s++<100000); }
static void start_port(hba_port_t*p){ int s=0; while((p->cmd & HBA_PxCMD_CR)&&s++<100000); p->cmd |= HBA_PxCMD_FRE; p->cmd |= HBA_PxCMD_ST; }

typedef struct { hba_port_t *port; hba_cmd_header_t *cmdlst; u8 *fis; hba_cmd_table_t *cmdtbl; u8 *dma; u32 dma_phys; } ahci_port_t;
static ahci_port_t ports[8];

static int ahci_rw(hba_port_t *port, hba_cmd_header_t *cmdlst, hba_cmd_table_t *cmdtbl, u8 *dma, u32 dma_phys, u64 lba, u32 count, u32 write, u32 port_idx){
    /* use slot 0 */
    hba_cmd_header_t *hdr=&cmdlst[0];
    hdr->flags = 5; if(write) hdr->flags |= 0x40;
    hdr->prdtl = 1;
    hdr->ctba = dma_phys ? 0 : 0; /* set below */
    /* command table physical */
    u64 ct_phys = (u64)cmdtbl - hhdm;
    hdr->ctba = (u32)ct_phys; hdr->ctbau = (u32)(ct_phys >> 32);
    /* clear table */
    for(int i=0;i<(int)sizeof(hba_cmd_table_t);i++) ((u8*)cmdtbl)[i]=0;
    /* fis */
    cmdtbl->fis[0]=0x27; cmdtbl->fis[1]=0x80; cmdtbl->fis[2]=write?ATA_CMD_WRITE_DMA_EXT:ATA_CMD_READ_DMA_EXT; cmdtbl->fis[3]=0;
    cmdtbl->fis[4]=lba&0xFF; cmdtbl->fis[5]=(lba>>8)&0xFF; cmdtbl->fis[6]=(lba>>16)&0xFF; cmdtbl->fis[7]=0x40; cmdtbl->fis[8]=(lba>>24)&0xFF; cmdtbl->fis[9]=(lba>>32)&0xFF; cmdtbl->fis[10]=(lba>>40)&0xFF;
    cmdtbl->fis[12]=count&0xFF; cmdtbl->fis[13]=(count>>8)&0xFF;
    /* prdt */
    u32 bytes=count*512; 
    /* data buffer physical */ 
    u64 data_phys = (u64)dma - hhdm;
    cmdtbl->prdt[0].dba=(u32)data_phys; cmdtbl->prdt[0].dbau=(u32)(data_phys>>32); cmdtbl->prdt[0].rsv0=0; cmdtbl->prdt[0].dbc=(bytes-1);
    /* issue */
    port->is = 0xFFFFFFFF; port->serr=0xFFFFFFFF; port->ci=1;
    int s=0; while((port->ci & 1) && s++<1000000){ if(port->is & HBA_PxIS_TFES){ log_write("[ahci] task file error\n"); return -1; } }
    if(port->is & HBA_PxIS_TFES) return -1;
    (void)port_idx; return 0;
}

static int ahci_read(blk_dev_t*d,u64 lba,u32 count,void*buf){
    ahci_port_t *p=(ahci_port_t*)d->priv; u8*b=(u8*)buf;
    /* copy through DMA bounce buffer (one sector at a time for simplicity) */
    for(u32 i=0;i<count;i++){
        if(ahci_rw(p->port,p->cmdlst,p->cmdtbl,p->dma,0,lba+i,1,0,0)<0) return -1;
        for(int j=0;j<512;j++) b[i*512+j]=p->dma[j];
    }
    return 0;
}
static int ahci_write(blk_dev_t*d,u64 lba,u32 count,const void*buf){
    ahci_port_t *p=(ahci_port_t*)d->priv; const u8*b=(const u8*)buf;
    for(u32 i=0;i<count;i++){
        for(int j=0;j<512;j++) p->dma[j]=b[i*512+j];
        if(ahci_rw(p->port,p->cmdlst,p->cmdtbl,p->dma,0,lba+i,1,1,0)<0) return -1;
    }
    return 0;
}

void ahci_init(void){
    hhdm=boot_info_get()->hhdm_offset; ahci_count=0;
    for(u16 bus=0;bus<256;bus++) for(u8 dev=0;dev<32;dev++) for(u8 fn=0;fn<8;fn++){
        u32 id=pci_read32((u8)bus,dev,fn,0); if((id&0xFFFF)==0xFFFF) continue;
        u32 cls=pci_read32((u8)bus,dev,fn,0x08); u8 cclass=(cls>>24)&0xFF, sub=(cls>>16)&0xFF, progif=(cls>>8)&0xFF;
        if(!(cclass==0x01 && sub==0x06 && progif==0x01)) continue; /* SATA AHCI */
        u32 bar5=pci_read32((u8)bus,dev,fn,0x24); if(bar5&1) continue; u64 mmio=bar5 & ~0xFULL; 
        vmm_map(mmio,mmio,0x3);
        hba_mem_t *hba=(hba_mem_t*)mmio;
        /* enable AHCI */
        hba->ghc |= (1u<<31); /* AE */
        u32 pi=hba->pi;
        for(int port=0;port<32;port++){
            if(!(pi & (1u<<port))) continue; hba_port_t*p=&hba->ports[port];
            /* check device present */
            u32 ssts=p->ssts; u32 det=ssts&0xF; if(det!=3) continue;
            stop_port(p);
            /* allocate port memory */
            u8 *mem=(u8*)pmm_alloc_pages(4); if(!mem) continue; u64 mphys=(u64)mem-hhdm;
            hba_cmd_header_t *cmdlst=(hba_cmd_header_t*)mem; /* 1 page */
            u8 *fis=mem+PAGE; /* fis receive */
            hba_cmd_table_t *cmdtbl=(hba_cmd_table_t*)(mem+2*PAGE);
            u8 *dma=mem+3*PAGE; /* 1 page dma */
            for(int k=0;k<PAGE;k++) mem[k]=0; for(int k=0;k<PAGE;k++) mem[PAGE+k]=0;
            p->clb=(u32)mphys; p->clbu=(u32)(mphys>>32); p->fb=(u32)(mphys+PAGE); p->fbu=(u32)((mphys+PAGE)>>32);
            /* command table for slot 0 */
            cmdlst[0].ctba=(u32)(mphys+2*PAGE); cmdlst[0].ctbau=(u32)((mphys+2*PAGE)>>32);
            start_port(p);
            if(ahci_count>=8) break;
            blk_dev_t*d=&ahci_devs[ahci_count]; ahci_port_t *ap=&ports[ahci_count];
            ap->port=p; ap->cmdlst=cmdlst; ap->fis=fis; ap->cmdtbl=cmdtbl; ap->dma=dma; ap->dma_phys=0;
            d->name[0]='s';d->name[1]='a';d->name[2]='t';d->name[3]='a';d->name[4]='0'+(ahci_count);d->name[5]=0; d->sectors=0; d->read=ahci_read; d->write=ahci_write; d->priv=ap;
            blk_register(d); ahci_count++;
            log_printf("[OK] AHCI SATA device %s port=%d\n", d->name, port);
        }
    }
    if(ahci_count==0) log_write("[AHCI] no AHCI SATA devices\n"); else log_printf("[OK] AHCI initialized (%d devices)\n", ahci_count);
}
