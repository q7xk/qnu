#include <qnu/types.h>
#include <qnu/log.h>

struct gdt_entry {
    u16 limit_low;
    u16 base_low;
    u8  base_mid;
    u8  access;
    u8  gran;
    u8  base_high;
} __attribute__((packed));

struct tss_entry {
    u16 limit_low;
    u16 base_low;
    u8  base_mid;
    u8  access;
    u8  gran;
    u8  base_high;
    u32 base_upper;
    u32 reserved;
} __attribute__((packed));

struct gdt_ptr {
    u16 limit;
    u64 base;
} __attribute__((packed));

struct tss {
    u32 reserved0;
    u64 rsp0;
    u64 rsp1;
    u64 rsp2;
    u64 reserved1;
    u64 ist1;
    u64 ist2;
    u64 ist3;
    u64 ist4;
    u64 ist5;
    u64 ist6;
    u64 ist7;
    u64 reserved2;
    u16 reserved3;
    u16 iopb_offset;
} __attribute__((packed));

static struct {
    struct gdt_entry null;
    struct gdt_entry kernel_code;
    struct gdt_entry kernel_data;
    struct gdt_entry user_code;
    struct gdt_entry user_data;
    struct tss_entry tss;
} __attribute__((packed)) gdt;

static struct gdt_ptr gdt_ptr;
static struct tss tss;

extern void gdt_flush(u64);
extern void tss_flush(void);

static void set_gdt_entry(struct gdt_entry *e, u8 access, u8 gran) {
    e->limit_low = 0;
    e->base_low = 0;
    e->base_mid = 0;
    e->access = access;
    e->gran = gran;
    e->base_high = 0;
}

void gdt_init(void) {
    set_gdt_entry(&gdt.null, 0, 0);
    set_gdt_entry(&gdt.kernel_code, 0x9A, 0x20);
    set_gdt_entry(&gdt.kernel_data, 0x92, 0x00);
    set_gdt_entry(&gdt.user_code, 0xFA, 0x20);
    set_gdt_entry(&gdt.user_data, 0xF2, 0x00);

    __builtin_memset(&tss, 0, sizeof(tss));
    tss.iopb_offset = sizeof(tss);

    u64 base = (u64)&tss;
    u32 limit = sizeof(tss) - 1;
    gdt.tss.limit_low = limit & 0xFFFF;
    gdt.tss.base_low = base & 0xFFFF;
    gdt.tss.base_mid = (base >> 16) & 0xFF;
    gdt.tss.access = 0x89;
    gdt.tss.gran = (limit >> 16) & 0x0F;
    gdt.tss.base_high = (base >> 24) & 0xFF;
    gdt.tss.base_upper = (base >> 32) & 0xFFFFFFFF;
    gdt.tss.reserved = 0;

    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (u64)&gdt;
    gdt_flush((u64)&gdt_ptr);
    tss_flush();
}
