#include <qnu/blk.h>
#include <qnu/pci.h>
#include <qnu/mm.h>
#include <qnu/log.h>
#include <qnu/boot.h>
#include <qnu/io.h>
#include <qnu/types.h>

#define VIRTIO_VENDOR 0x1AF4
#define VIRTIO_BLK_DEV 0x1001
#define VRING_ALIGN 4096
#define PAGE 4096

/* virtio legacy regs */
#define VIRTIO_PCI_HOST_FEATURES 0x00
#define VIRTIO_PCI_GUEST_FEATURES 0x04
#define VIRTIO_PCI_QUEUE_PFN 0x08
#define VIRTIO_PCI_QUEUE_NUM 0x0C
#define VIRTIO_PCI_QUEUE_SEL 0x0E
#define VIRTIO_PCI_QUEUE_NOTIFY 0x10
#define VIRTIO_PCI_STATUS 0x12
#define VIRTIO_STATUS_ACK 1
#define VIRTIO_STATUS_DRIVER 2
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FAILED 0x80

#define VRING_DESC_F_NEXT 1
#define VRING_DESC_F_WRITE 2
#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1

typedef struct { u64 addr; u32 len; u16 flags; u16 next; } __attribute__((packed)) vring_desc_t;
typedef struct { u16 flags; u16 idx; u16 ring[]; } __attribute__((packed)) vring_avail_t;
typedef struct { u32 id; u32 len; } __attribute__((packed)) vring_used_elem_t;
typedef struct { u16 flags; u16 idx; vring_used_elem_t ring[]; } __attribute__((packed)) vring_used_t;
typedef struct { u32 type; u32 reserved; u64 sector; } __attribute__((packed)) virtio_blk_req_t;

static blk_dev_t vdev;
static u16 iobase;
static u16 num;
static vring_desc_t *desc;
static vring_avail_t *avail;
static vring_used_t *used;
static volatile u16 last_used;
static u64 hhdm;

static inline u8 vin8(u16 off){ return inb(iobase+off); }
static inline u16 vin16(u16 off){ return inw(iobase+off); }
static inline u32 vin32(u16 off){ return inl(iobase+off); }
static inline void vout8(u16 off,u8 v){ outb(iobase+off,v); }
static inline void vout16(u16 off,u16 v){ outw(iobase+off,v); }
static inline void vout32(u16 off,u32 v){ outl(iobase+off,v); }

static int virtio_transfer(u64 sector,u32 count,void *buf,int is_write){
    u8 *b=(u8*)buf;
    for(u32 i=0;i<count;i++){
        u8 *databuf=(u8*)pmm_alloc_page(); if(!databuf) return -1;
        if(is_write) for(int j=0;j<512;j++) databuf[j]=b[i*512+j];
        u64 data_phys=(u64)databuf - hhdm;
        virtio_blk_req_t *req=(virtio_blk_req_t*)pmm_alloc_page(); if(!req){pmm_free_page(databuf);return -1;}
        u8 *status=(u8*)pmm_alloc_page(); if(!status){pmm_free_page(databuf);pmm_free_page(req);return -1;}
        req->type=is_write?VIRTIO_BLK_T_OUT:VIRTIO_BLK_T_IN; req->reserved=0; req->sector=sector+i; *status=0xFF;
        u64 req_phys=(u64)req-hhdm; u64 status_phys=(u64)status-hhdm;
        desc[0].addr=req_phys; desc[0].len=sizeof(virtio_blk_req_t); desc[0].flags=VRING_DESC_F_NEXT; desc[0].next=1;
        desc[1].addr=data_phys; desc[1].len=512; desc[1].flags=(is_write?0:VRING_DESC_F_WRITE)|VRING_DESC_F_NEXT; desc[1].next=2;
        desc[2].addr=status_phys; desc[2].len=1; desc[2].flags=VRING_DESC_F_WRITE; desc[2].next=0;
        u16 idx=avail->idx;
        avail->ring[idx % num]=0;
        __asm__ volatile("" ::: "memory");
        avail->idx = idx+1;
        __asm__ volatile("" ::: "memory");
        vout16(VIRTIO_PCI_QUEUE_NOTIFY,0);
        u32 spin=0; while(used->idx==last_used && spin<1000000) spin++;
        if(used->idx!=last_used){ last_used=used->idx; }
        if(!is_write) for(int j=0;j<512;j++) b[i*512+j]=databuf[j];
        pmm_free_page(status); pmm_free_page(req); pmm_free_page(databuf);
        if(spin>=1000000) return -1;
    }
    return 0;
}

static int vb_read(blk_dev_t*d,u64 lba,u32 count,void*buf){ (void)d; return virtio_transfer(lba,count,buf,0); }
static int vb_write(blk_dev_t*d,u64 lba,u32 count,const void*buf){ (void)d; return virtio_transfer(lba,count,(void*)buf,1); }

void virtio_blk_init(void){
    /* find virtio-blk PCI */
    int found=0; u8 bus=0,dev=0,func=0;
    for(u16 b=0;b<256 && !found;b++) for(u8 d=0;d<32 && !found;d++){
        u32 id=pci_read32((u8)b,d,0,0); if((id&0xFFFF)!=VIRTIO_VENDOR) continue;
        if(((id>>16)&0xFFFF)!=VIRTIO_BLK_DEV) continue;
        bus=(u8)b; dev=d; func=0; found=1;
    }
    if(!found){ log_write("[virtio-blk] no virtio-blk-pci device\n"); return; }
    u32 bar0=pci_read32(bus,dev,func,0x10);
    if(!(bar0&1)){ log_write("[virtio-blk] BAR0 not IO space; modern virtio not yet supported\n"); return; }
    iobase=(u16)(bar0 & ~3u);
    hhdm=boot_info_get()->hhdm_offset;
    vout8(VIRTIO_PCI_STATUS,0);
    vout8(VIRTIO_PCI_STATUS,VIRTIO_STATUS_ACK);
    vout8(VIRTIO_PCI_STATUS,VIRTIO_STATUS_ACK|VIRTIO_STATUS_DRIVER);
    u32 hf=vin32(VIRTIO_PCI_HOST_FEATURES); vout32(VIRTIO_PCI_GUEST_FEATURES, hf & 0); /* no features */
    vout16(VIRTIO_PCI_QUEUE_SEL,0); num=vin16(VIRTIO_PCI_QUEUE_NUM); if(num==0) num=8;
    /* allocate contiguous 2-page queue */
    u8 *q=(u8*)pmm_alloc_pages(2); if(!q){ log_write("[virtio-blk] no queue mem\n"); return; }
    u64 qphys=(u64)q-hhdm;
    desc=(vring_desc_t*)q;
    avail=(vring_avail_t*)(q + sizeof(vring_desc_t)*num);
    used=(vring_used_t*)(q + VRING_ALIGN);
    last_used=0;
    vout32(VIRTIO_PCI_QUEUE_PFN,(u32)(qphys/PAGE));
    vout8(VIRTIO_PCI_STATUS,VIRTIO_STATUS_ACK|VIRTIO_STATUS_DRIVER|VIRTIO_STATUS_DRIVER_OK);
    /* capacity at device config offset 0x14 (after generic regs) */
    u64 sectors=0;
    u32 lo=vin32(0x14); u32 hi=vin32(0x18); sectors=((u64)hi<<32)|lo; if(sectors==0) sectors=2048;
    vdev.name[0]='v';vdev.name[1]='d';vdev.name[2]='a';vdev.name[3]=0; vdev.sectors=sectors; vdev.read=vb_read; vdev.write=vb_write; vdev.priv=0;
    blk_register(&vdev);
    log_printf("[OK] virtio-blk initialized (%s sectors=%lu iobase=%x num=%u)\n", vdev.name, sectors, iobase, num);
}
