#include <qnu/io.h>
#include <qnu/types.h>
#include <qnu/log.h>

static volatile u64 ticks = 0;

void pit_tick(void) { ticks++; }
u64 pit_get_ticks(void) { return ticks; }

void pit_init(u32 hz) {
    if (hz == 0) hz = 100;
    u32 divisor = 1193182 / hz;
    outb(0x43, 0x36);
    outb(0x40, (u8)(divisor & 0xFF));
    outb(0x40, (u8)((divisor >> 8) & 0xFF));
}
