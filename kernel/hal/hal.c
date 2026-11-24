#include <qnu/hal.h>
#include <qnu/types.h>

u64 pit_get_ticks(void);

void hal_init(void) {}

void hal_cpu_halt(void) { __asm__ volatile ("hlt"); }
void hal_irq_enable(void) { __asm__ volatile ("sti"); }
void hal_irq_disable(void) { __asm__ volatile ("cli"); }
u64 hal_timer_ticks(void) { return pit_get_ticks(); }
