#include <qnu/pci.h>
#include <qnu/io.h>
#include <qnu/log.h>

typedef struct { u8 bus,dev,func,cls,sub; u16 vend,dev_id; } pci_dev_t;
static pci_dev_t devs[128];
static int count = 0;

static u32 pci_read(u8 bus, u8 dev, u8 func, u8 off) {
    u32 addr = (1u<<31) | ((u32)bus<<16) | ((u32)dev<<11) | ((u32)func<<8) | (off & 0xFC);
    __asm__ volatile("outl %0, %1" :: "a"(addr), "Nd"((u16)0xCF8));
    u32 val; __asm__ volatile("inl %1, %0" : "=a"(val) : "Nd"((u16)0xCFC)); return val;
}

static void check_func(u8 bus, u8 dev, u8 func) {
    u32 id = pci_read(bus, dev, func, 0);
    if ((id & 0xFFFF) == 0xFFFF) return;
    u32 cls = pci_read(bus, dev, func, 0x08);
    u8 class_id = (cls >> 24) & 0xFF;
    u8 subclass = (cls >> 16) & 0xFF;
    if (count < 128) { devs[count]=(pci_dev_t){bus,dev,func,class_id,subclass,(u16)(id&0xFFFF),(u16)((id>>16)&0xFFFF)}; }
    log_printf(" PCI %x:%x.%u vend=%x dev=%x class=%x:%x\n", bus, dev, func, id & 0xFFFF, (id>>16)&0xFFFF, class_id, subclass);
    count++;
}

void pci_init(void) {
    count = 0;
    for (u16 bus = 0; bus < 256; bus++) {
        for (u8 dev = 0; dev < 32; dev++) {
            u32 id = pci_read((u8)bus, dev, 0, 0);
            if ((id & 0xFFFF) == 0xFFFF) continue;
            u32 hdr = pci_read((u8)bus, dev, 0, 0x0C);
            u8 mf = ((hdr >> 16) & 0x80) != 0;
            check_func((u8)bus, dev, 0);
            if (mf) for (u8 f = 1; f < 8; f++) check_func((u8)bus, dev, f);
        }
    }
    log_printf("[OK] PCI enumerated (%d devices)\n", count);
}

void pci_dump(void) { pci_init(); }

void pci_wireless_dump(void) {
    int found=0;
    for(int i=0;i<count && i<128;i++){
        if(devs[i].cls==0x02 && devs[i].sub==0x80){ log_printf(" WiFi PCI: %x:%x.%u vend=%x dev=%x\n", devs[i].bus,devs[i].dev,devs[i].func,devs[i].vend,devs[i].dev_id); found++; }
        if(devs[i].cls==0x0D && devs[i].sub==0x11){ log_printf(" Bluetooth PCI: %x:%x.%u vend=%x dev=%x\n", devs[i].bus,devs[i].dev,devs[i].func,devs[i].vend,devs[i].dev_id); found++; }
    }
    if(!found) log_write(" No WiFi/BLE PCI controllers detected. USB wireless devices unsupported (no USB stack).\n");
}


void pci_usb_dump(void){
    int found=0;
    for(int i=0;i<count && i<128;i++){
        if(devs[i].cls==0x0C && devs[i].sub==0x03){ log_printf(" USB controller: %x:%x.%u vend=%x dev=%x progif? xHCI/EHCI/UHCI\n", devs[i].bus,devs[i].dev,devs[i].func,devs[i].vend,devs[i].dev_id); found++; }
    }
    if(!found) log_write(" No USB controllers detected\n");
}
void pci_storage_dump(void){
    int found=0;
    for(int i=0;i<count && i<128;i++){
        if(devs[i].cls==0x01){ log_printf(" Storage controller: %x:%x.%u vend=%x dev=%x class=%x:%x\n", devs[i].bus,devs[i].dev,devs[i].func,devs[i].vend,devs[i].dev_id,devs[i].cls,devs[i].sub); found++; }
    }
    if(!found) log_write(" No storage controllers detected\n");
}

u32 pci_read32(u8 bus,u8 dev,u8 func,u8 off){ return pci_read(bus,dev,func,off); }
void pci_write32(u8 bus,u8 dev,u8 func,u8 off,u32 val){ u32 addr=(1u<<31)|((u32)bus<<16)|((u32)dev<<11)|((u32)func<<8)|(off&0xFC); __asm__ volatile("outl %0,%1"::"a"(addr),"Nd"((u16)0xCF8)); __asm__ volatile("outl %0,%1"::"a"(val),"Nd"((u16)0xCFC)); }
