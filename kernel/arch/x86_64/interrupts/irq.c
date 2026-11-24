#include <qnu/types.h>
#include <qnu/log.h>
#include <qnu/io.h>
#include <qnu/lapic.h>
#include <qnu/ioapic.h>

struct interrupt_frame {
    u64 rax, rbx, rcx, rdx, rsi, rdi, rbp;
    u64 r8, r9, r10, r11, r12, r13, r14, r15;
    u64 vector;
    u64 error_code;
    u64 rip, cs, rflags, rsp, ss;
};

void pic_eoi(u8 irq);
void pit_tick(void);
void keyboard_irq(void);
void sched_tick(void);
bool lapic_timer_active(void);

#define MAX_IRQ 16
static void (*irq_handlers[MAX_IRQ])(void);

void irq_register(u8 irq, void (*handler)(void)) {
    if (irq < MAX_IRQ) irq_handlers[irq] = handler;
}

void irq_handler(struct interrupt_frame *f) {
    u8 irq = (u8)(f->vector - 32);
    if (irq < MAX_IRQ && irq_handlers[irq]) irq_handlers[irq]();
    if (irq == 0) { pit_tick(); sched_tick(); }
    if (irq == 1) { keyboard_irq(); }
    if (ioapic_active()) lapic_eoi(); else pic_eoi(irq);
}
