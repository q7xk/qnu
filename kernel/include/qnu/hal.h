#ifndef QNU_HAL_H
#define QNU_HAL_H
#include <qnu/types.h>
void hal_init(void);
void hal_cpu_halt(void);
void hal_irq_enable(void);
void hal_irq_disable(void);
u64 hal_timer_ticks(void);
#endif
