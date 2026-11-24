#ifndef QNU_LAPIC_H
#define QNU_LAPIC_H
#include <qnu/types.h>
void lapic_init(void);
void lapic_eoi(void);
void lapic_timer_init(u32 hz);
u32 lapic_id(void);
#endif
