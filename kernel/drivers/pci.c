#include <qnu/pci.h>
#include <qnu/io.h>
#include <qnu/log.h>

static u32 pci_read(u8 bus, u8 dev, u8 func, u8 off) {
    u32 addr = (1u<<31) | ((u32)bus<<16) | ((u32)dev<<11) | ((u32)func<<8) | (off & 0xFC);
    outb(0xCF8, 0); /* dummy */
    __asm__ volatile("outl %0, %1" :: "a"(addr), "Nd"((u16)0xCF8));
    u32 val;
    __asm__ volatile("inl %1, %0" : "=a"(val) : "Nd"((u16)0xCFC));
    return val;
}

static void pci_write(u32 addr, u32 val) {
    __asm__ volatile("outl %0, %1" :: "a"(addr), "Nd"((u16)0xCF8));
    __asm__ volatile("outl %0, %1" :: "a"(val), "Nd"((u16)0xCFC));
}

static int count = 0;

static void check_func(u8 bus, u8 dev, u8 func) {
    u32 id = pci_read(bus, dev, func, 0);
    if ((id & 0xFFFF) == 0xFFFF) return;
    u32 cls = pci_read(bus, dev, func, 0x08);
    u8 class_id = (cls >> 24) & 0xFF;
    u8 subclass = (cls >> 16) & 0xFF;
    log_printf(" PCI %x:%x.%u vend=%x dev=%x class=%x:%x\n",
        bus, dev, func, id & 0xFFFF, (id>>16)&0xFFFF, class_id, subclass);
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
