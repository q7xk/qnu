#include <qnu/acpi.h>
#include <qnu/boot.h>
#include <qnu/boot_internal.h>
#include <qnu/log.h>
#include <qnu/panic.h>
#include <qnu/mm.h>

struct rsdp {
    char signature[8];
    u8 checksum;
    char oemid[6];
    u8 revision;
    u32 rsdt_address;
    u32 length;
    u64 xsdt_address;
    u8 ext_checksum;
    u8 reserved[3];
} __attribute__((packed));

struct sdt_header {
    char signature[4];
    u32 length;
    u8 revision;
    u8 checksum;
    char oemid[6];
    char oem_table_id[8];
    u32 oem_revision;
    u32 creator_id;
    u32 creator_revision;
} __attribute__((packed));

struct madt {
    struct sdt_header hdr;
    u32 lapic_addr;
    u32 flags;
} __attribute__((packed));

struct madt_entry {
    u8 type;
    u8 length;
} __attribute__((packed));

static struct sdt_header *xsdt;
static u32 sdt_count;
static u64 hhdm;
static u32 madt_lapic_addr;
static u32 madt_ioapic_addr;
static u32 madt_cpu_count;
static bool madt_found = false;

static bool checksum_ok(void *ptr, usize len) {
    u8 sum = 0; u8 *p = (u8 *)ptr;
    for (usize i = 0; i < len; i++) sum += p[i];
    return sum == 0;
}

static void parse_madt(struct madt *m) {
    madt_found = true;
    madt_lapic_addr = m->lapic_addr;
    madt_cpu_count = 0;
    madt_ioapic_addr = 0;
    u8 *p = (u8 *)m + sizeof(struct madt);
    u8 *end = (u8 *)m + m->hdr.length;
    while (p + sizeof(struct madt_entry) <= end) {
        struct madt_entry *e = (struct madt_entry *)p;
        if (e->length < 2) break;
        if (e->type == 0 && e->length >= 8) { /* Local APIC */
            u8 flags = *(p + 4);
            if (flags & 1) madt_cpu_count++;
        } else if (e->type == 1 && e->length >= 12) { /* IOAPIC */
            madt_ioapic_addr = *(u32 *)(p + 4);
        } else if (e->type == 5 && e->length >= 12) { /* LAPIC address override */
            madt_lapic_addr = (u32)(*(u64 *)(p + 4));
        }
        p += e->length;
    }
    log_printf("[OK] MADT parsed (cpus=%u lapic=%x ioapic=%x)\n", madt_cpu_count, madt_lapic_addr, madt_ioapic_addr);
}

void acpi_init(void) {
    hhdm = boot_info_get()->hhdm_offset;
    void *rsdp_phys = boot_rsdp();
    if (!rsdp_phys) { log_write("[ACPI] RSDP not found\n"); return; }
    u64 rp = (u64)rsdp_phys & ~0xFFFULL;
    vmm_map(rp + hhdm, rp, 0x3);
    struct rsdp *r = (struct rsdp *)((u64)rsdp_phys + hhdm);
    if (!checksum_ok(r, 20)) { log_write("[ACPI] RSDP checksum invalid\n"); return; }
    u64 xsdt_phys = (r->revision >= 2 && r->xsdt_address) ? r->xsdt_address : (u64)r->rsdt_address;
    vmm_map((xsdt_phys & ~0xFFFULL) + hhdm, xsdt_phys & ~0xFFFULL, 0x3);
    xsdt = (struct sdt_header *)(xsdt_phys + hhdm);
    if (!xsdt || !checksum_ok(xsdt, xsdt->length)) { log_write("[ACPI] XSDT/RSDT invalid\n"); xsdt = NULL; return; }
    u32 entry_size = (xsdt->signature[0] == 'X') ? 8 : 4;
    sdt_count = (xsdt->length - sizeof(struct sdt_header)) / entry_size;
    log_printf("[OK] ACPI initialized (%u tables)\n", sdt_count);
    struct madt *m = (struct madt *)acpi_find_sdt("APIC");
    if (m) parse_madt(m);
}

void *acpi_find_sdt(const char sig[4]) {
    if (!xsdt) return NULL;
    u32 entry_size = (xsdt->signature[0] == 'X') ? 8 : 4;
    u8 *entries = (u8 *)xsdt + sizeof(struct sdt_header);
    for (u32 i = 0; i < sdt_count; i++) {
        u64 addr = 0;
        if (entry_size == 8) addr = *(u64 *)(entries + i * 8);
        else addr = *(u32 *)(entries + i * 4);
        vmm_map((addr & ~0xFFFULL) + hhdm, addr & ~0xFFFULL, 0x3);
        struct sdt_header *h = (struct sdt_header *)(addr + hhdm);
        bool match = true;
        for (int j = 0; j < 4; j++) if (h->signature[j] != sig[j]) match = false;
        if (match) return h;
    }
    return NULL;
}

void acpi_dump(void) {
    if (!xsdt) { log_write("ACPI: unavailable\n"); return; }
    log_printf("ACPI tables (%u):\n", sdt_count);
    u32 entry_size = (xsdt->signature[0] == 'X') ? 8 : 4;
    u8 *entries = (u8 *)xsdt + sizeof(struct sdt_header);
    for (u32 i = 0; i < sdt_count; i++) {
        u64 addr = entry_size == 8 ? *(u64 *)(entries + i * 8) : *(u32 *)(entries + i * 4);
        vmm_map((addr & ~0xFFFULL) + hhdm, addr & ~0xFFFULL, 0x3);
        struct sdt_header *h = (struct sdt_header *)(addr + hhdm);
        char sig[5] = {h->signature[0], h->signature[1], h->signature[2], h->signature[3], 0};
        log_printf(" - %s len=%u\n", sig, h->length);
    }
    if (madt_found) log_printf("MADT: cpus=%u lapic=%x ioapic=%x\n", madt_cpu_count, madt_lapic_addr, madt_ioapic_addr);
}

u32 acpi_madt_lapic(void) { return madt_lapic_addr; }
u32 acpi_madt_ioapic(void) { return madt_ioapic_addr; }
u32 acpi_madt_cpus(void) { return madt_cpu_count; }
