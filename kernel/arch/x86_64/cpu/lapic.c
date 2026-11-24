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
#define LAPIC_TIMER_VEC 48

static volatile u32 *lapic;
static bool enabled = false;
static bool timer_running = false;
extern u64 pit_get_ticks(void);
extern void sched_tick(void);
extern void pit_tick(void);

static inline u64 rdmsr(u32 msr) { u32 lo,hi; __asm__ volatile("rdmsr":"=a"(lo),"=d"(hi):"c"(msr)); return ((u64)hi<<32)|lo; }
static inline void wrmsr(u32 msr,u64 val){ u32 lo=val&0xFFFFFFFF,hi=val>>32; __asm__ volatile("wrmsr"::"a"(lo),"d"(hi),"c"(msr)); }
static u32 lr(u32 r){ return lapic[r/4]; }
static void lw(u32 r,u32 v){ lapic[r/4]=v; (void)lapic[0]; }

u32 lapic_id(void){ return enabled ? (lr(LAPIC_REG_ID)>>24) : 0; }
void lapic_eoi(void){ if(enabled) lw(LAPIC_REG_EOI,0); }

void lapic_init(void){
    if(!cpu_has_apic()){ log_write("[LAPIC] not present\n"); return; }
    u64 base=rdmsr(IA32_APIC_BASE_MSR);
    u64 phys=base&0xFFFFF000ULL;
    base|=(1u<<11);
    wrmsr(IA32_APIC_BASE_MSR,base);
    u64 hhdm=boot_info_get()->hhdm_offset;
    vmm_map(phys+hhdm,phys,0x3);
    lapic=(volatile u32*)(phys+hhdm);
    lw(LAPIC_REG_TPR,0);
    lw(LAPIC_REG_SVR,0x1FF);
    enabled=true;
    log_printf("[OK] LAPIC initialized (id=%u)\n", lapic_id());
}

void lapic_timer_init(u32 hz){
    if(!enabled) return;
    if(hz==0) hz=100;
    /* calibrate with PIT ticks */
    lw(LAPIC_REG_TIMER_DIV,0x3);
    lw(LAPIC_REG_LVT_TIMER, 0x10000 | LAPIC_TIMER_VEC); /* masked */
    lw(LAPIC_REG_TIMER_INITCNT, 0xFFFFFFFF);
    u64 start = pit_get_ticks();
    while (pit_get_ticks() == start) { __asm__ volatile("hlt"); }
    start = pit_get_ticks();
    lw(LAPIC_REG_TIMER_INITCNT, 0xFFFFFFFF);
    while (pit_get_ticks() < start + 10) { __asm__ volatile("hlt"); } /* ~100ms */
    u32 elapsed = 0xFFFFFFFF - lr(LAPIC_REG_TIMER_CURCNT);
    u32 ticks_per_10ms = elapsed;
    if (ticks_per_10ms == 0) ticks_per_10ms = 100000;
    u32 init = ticks_per_10ms / (hz / 10);
    if (init == 0) init = 1000;
    lw(LAPIC_REG_LVT_TIMER, LAPIC_TIMER_VEC); /* periodic, unmasked */
    lw(LAPIC_REG_TIMER_DIV,0x3);
    lw(LAPIC_REG_TIMER_INITCNT, init);
    timer_running = true;
    log_printf("[OK] LAPIC timer running (%u Hz)\n", hz);
}

bool lapic_timer_active(void){ return timer_running; }

void lapic_timer_handler(void *frame){
    (void)frame;
    pit_tick();
    lapic_eoi();
    sched_tick();
}
