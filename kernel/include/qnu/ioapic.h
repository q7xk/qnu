#ifndef QNU_IOAPIC_H
#define QNU_IOAPIC_H
#include <qnu/types.h>
void ioapic_init(void);
void ioapic_set_irq(u8 irq, u8 vector, bool masked);
bool ioapic_active(void);
#endif
