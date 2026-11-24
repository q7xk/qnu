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

static struct sdt_header *xsdt;
static u32 sdt_count;
static u64 hhdm;

static bool checksum_ok(void *ptr, usize len) {
    u8 sum = 0;
    u8 *p = (u8 *)ptr;
    for (usize i = 0; i < len; i++) sum += p[i];
    return sum == 0;
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
    if (!xsdt || !checksum_ok(xsdt, xsdt->length)) {
        log_write("[ACPI] XSDT/RSDT invalid\n");
        xsdt = NULL;
        return;
    }
    u32 entry_size = (xsdt->signature[0] == 'X') ? 8 : 4;
    sdt_count = (xsdt->length - sizeof(struct sdt_header)) / entry_size;
    log_printf("[OK] ACPI initialized (%u tables)\n", sdt_count);
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
}
