#ifndef QNU_ARCH_H
#define QNU_ARCH_H
#include <qnu/types.h>
void arch_cpu_init(void);
void gdt_init(void);
void idt_init(void);
void interrupts_init(void);
void pit_init(u32 hz);
void pic_init(void);
#endif
