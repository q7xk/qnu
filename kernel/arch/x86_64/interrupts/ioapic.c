#include <qnu/ioapic.h>
#include <qnu/acpi.h>
#include <qnu/boot.h>
#include <qnu/mm.h>
#include <qnu/log.h>

#define IOREGSEL 0x00
#define IOWIN    0x10

static volatile u32 *ioapic;
static bool active = false;

static void write_reg(u8 reg, u32 val) { ioapic[IOREGSEL/4] = reg; ioapic[IOWIN/4] = val; }
static u32 read_reg(u8 reg) { ioapic[IOREGSEL/4] = reg; return ioapic[IOWIN/4]; }

void ioapic_set_irq(u8 irq, u8 vector, bool masked) {
    if (!active) return;
    u32 low = vector;
    if (masked) low |= (1u << 16);
    u32 high = 0; /* dest APIC 0 */
    write_reg(0x10 + irq*2, low);
    write_reg(0x10 + irq*2 + 1, high);
}

bool ioapic_active(void) { return active; }

void ioapic_init(void) {
    u32 phys = acpi_madt_ioapic();
    if (!phys) { log_write("[IOAPIC] not found\n"); return; }
    u64 hhdm = boot_info_get()->hhdm_offset;
    vmm_map((u64)phys + hhdm, phys, 0x3);
    ioapic = (volatile u32 *)((u64)phys + hhdm);
    u32 ver = read_reg(1);
    u32 max_redir = ((ver >> 16) & 0xFF);
    for (u32 i = 0; i <= max_redir && i < 24; i++) ioapic_set_irq(i, 32+i, true);
    /* route keyboard IRQ1 */
    ioapic_set_irq(1, 33, false);
    active = true;
    log_printf("[OK] IOAPIC initialized (redir=%u)\n", max_redir + 1);
}
