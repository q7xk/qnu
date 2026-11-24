#include <qnu/types.h>
#include <qnu/log.h>
#include <qnu/panic.h>

struct interrupt_frame {
    u64 rax, rbx, rcx, rdx, rsi, rdi, rbp;
    u64 r8, r9, r10, r11, r12, r13, r14, r15;
    u64 vector;
    u64 error_code;
    u64 rip, cs, rflags, rsp, ss;
};

static const char *exc_names[32] = {
    "Divide-by-zero","Debug","NMI","Breakpoint","Overflow","Bound Range","Invalid Opcode",
    "Device Not Available","Double Fault","Coprocessor Segment Overrun","Invalid TSS",
    "Segment Not Present","Stack-Segment Fault","General Protection Fault","Page Fault",
    "Reserved","x87 Floating-Point","Alignment Check","Machine Check","SIMD FP",
    "Virtualization","Control Protection","Reserved","Reserved","Reserved","Reserved",
    "Reserved","Reserved","Hypervisor Injection","VMM Communication","Security","Reserved"
};

void isr_handler(struct interrupt_frame *f) {
    if (f->vector < 32) {
        log_printf("\n[EXCEPTION] %s (#%lu) err=%lx rip=%lx rsp=%lx\n",
            exc_names[f->vector], f->vector, f->error_code, f->rip, f->rsp);
        if (f->vector == 14) {
            u64 cr2;
            __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
            log_printf("CR2=%lx\n", cr2);
        }
        panic("Unhandled CPU exception");
    }
}
