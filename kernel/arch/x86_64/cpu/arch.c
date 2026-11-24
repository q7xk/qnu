#include <qnu/arch.h>
#include <qnu/log.h>

void gdt_init(void);
void idt_init(void);
void pic_init(void);

void arch_cpu_init(void) {
    gdt_init();
    idt_init();
}

void interrupts_init(void) {
    pic_init();
    __asm__ volatile ("sti");
}
