#include <qnu/types.h>
#include <qnu/log.h>

struct idt_entry {
    u16 offset_low;
    u16 selector;
    u8  ist;
    u8  type_attr;
    u16 offset_mid;
    u32 offset_high;
    u32 zero;
} __attribute__((packed));

struct idt_ptr {
    u16 limit;
    u64 base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr idt_ptr;

extern void *isr_stub_table[];
extern void *irq_stub_table[];
extern void isr48(void);

static void idt_set(u8 vec, void *handler, u8 flags) {
    u64 addr = (u64)handler;
    idt[vec].offset_low = addr & 0xFFFF;
    idt[vec].selector = 0x08;
    idt[vec].ist = 0;
    idt[vec].type_attr = flags;
    idt[vec].offset_mid = (addr >> 16) & 0xFFFF;
    idt[vec].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vec].zero = 0;
}

void idt_init(void) {
    for (int i = 0; i < 256; i++) idt_set(i, 0, 0);
    for (int i = 0; i < 32; i++) idt_set(i, isr_stub_table[i], 0x8E);
    for (int i = 0; i < 16; i++) idt_set(32 + i, irq_stub_table[i], 0x8E);
    idt_set(48, (void*)isr48, 0x8E);
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (u64)&idt;
    __asm__ volatile ("lidt %0" : : "m"(idt_ptr));
}
