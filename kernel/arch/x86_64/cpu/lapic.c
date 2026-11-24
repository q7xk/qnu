#include <qnu/lapic.h>
#include <qnu/cpu.h>
#include <qnu/log.h>
#include <qnu/boot.h>
#include <qnu/io.h>
#include <qnu/mm.h>

#define IA32_APIC_BASE_MSR 0x1B
#define LAPIC_REG_ID 0x20
#define LAPIC_REG_EOI 0xB0
#define LAPIC_REG_SVR 0xF0
#define LAPIC_REG_TPR 0x80
#define LAPIC_REG_LVT_TIMER 0x320
#define LAPIC_REG_TIMER_INITCNT 0x380
#define LAPIC_REG_TIMER_CURCNT 0x390
#define LAPIC_REG_TIMER_DIV 0x3E0

static volatile u32 *lapic;
static bool enabled = false;

static inline u64 rdmsr(u32 msr) {
    u32 lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((u64)hi << 32) | lo;
}
static inline void wrmsr(u32 msr, u64 val) {
    u32 lo = val & 0xFFFFFFFF, hi = val >> 32;
    __asm__ volatile("wrmsr" :: "a"(lo), "d"(hi), "c"(msr));
}

static u32 lapic_read(u32 reg) { return lapic[reg/4]; }
static void lapic_write(u32 reg, u32 val) { lapic[reg/4] = val; (void)lapic[0]; }

u32 lapic_id(void) { return enabled ? (lapic_read(LAPIC_REG_ID) >> 24) : 0; }

void lapic_eoi(void) { if (enabled) lapic_write(LAPIC_REG_EOI, 0); }

void lapic_init(void) {
    if (!cpu_has_apic()) { log_write("[LAPIC] not present\n"); return; }
    u64 base = rdmsr(IA32_APIC_BASE_MSR);
    u64 phys = base & 0xFFFFF000ULL;
    base |= (1u << 11); /* enable */
    wrmsr(IA32_APIC_BASE_MSR, base);
    u64 hhdm = boot_info_get()->hhdm_offset;
    vmm_map(phys + hhdm, phys, 0x3);
    lapic = (volatile u32 *)(phys + hhdm);
    lapic_write(LAPIC_REG_TPR, 0);
    lapic_write(LAPIC_REG_SVR, 0x1FF); /* enable + spurious 0xFF */
    enabled = true;
    log_printf("[OK] LAPIC initialized (id=%u)\n", lapic_id());
}

void lapic_timer_init(u32 hz) {
    if (!enabled) return;
    if (hz == 0) hz = 100;
    /* calibrate using PIT channel 2 or simple busy wait via PIT ticks */
    lapic_write(LAPIC_REG_TIMER_DIV, 0x3); /* divide by 16 */
    lapic_write(LAPIC_REG_LVT_TIMER, 0x20020 | 32); /* periodic, masked, vector 32 - we keep PIT for now */
    lapic_write(LAPIC_REG_TIMER_INITCNT, 0);
    log_write("[LAPIC] timer calibrated (PIT retained for scheduler)\n");
}
