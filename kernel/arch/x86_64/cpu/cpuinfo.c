#include <qnu/cpu.h>
#include <qnu/log.h>

static char vendor[13];
static bool has_apic = false;

static inline void cpuid(u32 leaf, u32 *a, u32 *b, u32 *c, u32 *d) {
    __asm__ volatile("cpuid" : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d) : "a"(leaf), "c"(0));
}

void cpu_info_init(void) {
    u32 a,b,c,d;
    cpuid(0, &a, &b, &c, &d);
    *(u32*)(vendor+0) = b;
    *(u32*)(vendor+4) = d;
    *(u32*)(vendor+8) = c;
    vendor[12] = 0;
    cpuid(1, &a, &b, &c, &d);
    has_apic = (d & (1u << 9)) != 0;
    log_printf("[OK] CPU detected: %s apic=%s\n", vendor, has_apic ? "yes" : "no");
}

void cpu_info_dump(void) {
    log_printf("CPU vendor: %s\n", vendor);
    log_printf("APIC: %s\n", has_apic ? "present" : "absent");
}

bool cpu_has_apic(void) { return has_apic; }
