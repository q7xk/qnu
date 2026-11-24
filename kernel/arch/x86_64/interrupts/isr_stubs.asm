BITS 64
DEFAULT REL
section .text

extern isr_handler
extern irq_handler

%macro PUSH_REGS 0
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax
%endmacro

%macro POP_REGS 0
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
%endmacro

common_isr_stub:
    PUSH_REGS
    mov rdi, rsp
    call isr_handler
    POP_REGS
    add rsp, 16
    iretq

common_irq_stub:
    PUSH_REGS
    mov rdi, rsp
    call irq_handler
    POP_REGS
    add rsp, 16
    iretq

global isr0
isr0:
    push qword 0
    push qword 0
    jmp common_isr_stub
global isr1
isr1:
    push qword 0
    push qword 1
    jmp common_isr_stub
global isr2
isr2:
    push qword 0
    push qword 2
    jmp common_isr_stub
global isr3
isr3:
    push qword 0
    push qword 3
    jmp common_isr_stub
global isr4
isr4:
    push qword 0
    push qword 4
    jmp common_isr_stub
global isr5
isr5:
    push qword 0
    push qword 5
    jmp common_isr_stub
global isr6
isr6:
    push qword 0
    push qword 6
    jmp common_isr_stub
global isr7
isr7:
    push qword 0
    push qword 7
    jmp common_isr_stub
global isr8
isr8:
    push qword 8
    jmp common_isr_stub
global isr9
isr9:
    push qword 0
    push qword 9
    jmp common_isr_stub
global isr10
isr10:
    push qword 10
    jmp common_isr_stub
global isr11
isr11:
    push qword 11
    jmp common_isr_stub
global isr12
isr12:
    push qword 12
    jmp common_isr_stub
global isr13
isr13:
    push qword 13
    jmp common_isr_stub
global isr14
isr14:
    push qword 14
    jmp common_isr_stub
global isr15
isr15:
    push qword 0
    push qword 15
    jmp common_isr_stub
global isr16
isr16:
    push qword 0
    push qword 16
    jmp common_isr_stub
global isr17
isr17:
    push qword 17
    jmp common_isr_stub
global isr18
isr18:
    push qword 0
    push qword 18
    jmp common_isr_stub
global isr19
isr19:
    push qword 0
    push qword 19
    jmp common_isr_stub
global isr20
isr20:
    push qword 0
    push qword 20
    jmp common_isr_stub
global isr21
isr21:
    push qword 21
    jmp common_isr_stub
global isr22
isr22:
    push qword 0
    push qword 22
    jmp common_isr_stub
global isr23
isr23:
    push qword 0
    push qword 23
    jmp common_isr_stub
global isr24
isr24:
    push qword 0
    push qword 24
    jmp common_isr_stub
global isr25
isr25:
    push qword 0
    push qword 25
    jmp common_isr_stub
global isr26
isr26:
    push qword 0
    push qword 26
    jmp common_isr_stub
global isr27
isr27:
    push qword 0
    push qword 27
    jmp common_isr_stub
global isr28
isr28:
    push qword 0
    push qword 28
    jmp common_isr_stub
global isr29
isr29:
    push qword 29
    jmp common_isr_stub
global isr30
isr30:
    push qword 30
    jmp common_isr_stub
global isr31
isr31:
    push qword 0
    push qword 31
    jmp common_isr_stub
global irq0
irq0:
    push qword 0
    push qword 32
    jmp common_irq_stub
global irq1
irq1:
    push qword 0
    push qword 33
    jmp common_irq_stub
global irq2
irq2:
    push qword 0
    push qword 34
    jmp common_irq_stub
global irq3
irq3:
    push qword 0
    push qword 35
    jmp common_irq_stub
global irq4
irq4:
    push qword 0
    push qword 36
    jmp common_irq_stub
global irq5
irq5:
    push qword 0
    push qword 37
    jmp common_irq_stub
global irq6
irq6:
    push qword 0
    push qword 38
    jmp common_irq_stub
global irq7
irq7:
    push qword 0
    push qword 39
    jmp common_irq_stub
global irq8
irq8:
    push qword 0
    push qword 40
    jmp common_irq_stub
global irq9
irq9:
    push qword 0
    push qword 41
    jmp common_irq_stub
global irq10
irq10:
    push qword 0
    push qword 42
    jmp common_irq_stub
global irq11
irq11:
    push qword 0
    push qword 43
    jmp common_irq_stub
global irq12
irq12:
    push qword 0
    push qword 44
    jmp common_irq_stub
global irq13
irq13:
    push qword 0
    push qword 45
    jmp common_irq_stub
global irq14
irq14:
    push qword 0
    push qword 46
    jmp common_irq_stub
global irq15
irq15:
    push qword 0
    push qword 47
    jmp common_irq_stub

section .rodata
global isr_stub_table
isr_stub_table:
    dq isr0
    dq isr1
    dq isr2
    dq isr3
    dq isr4
    dq isr5
    dq isr6
    dq isr7
    dq isr8
    dq isr9
    dq isr10
    dq isr11
    dq isr12
    dq isr13
    dq isr14
    dq isr15
    dq isr16
    dq isr17
    dq isr18
    dq isr19
    dq isr20
    dq isr21
    dq isr22
    dq isr23
    dq isr24
    dq isr25
    dq isr26
    dq isr27
    dq isr28
    dq isr29
    dq isr30
    dq isr31

global isr48
extern lapic_timer_handler
isr48:
    push qword 0
    push qword 48
    PUSH_REGS
    mov rdi, rsp
    call lapic_timer_handler
    POP_REGS
    add rsp, 16
    iretq
global irq_stub_table
irq_stub_table:
    dq irq0
    dq irq1
    dq irq2
    dq irq3
    dq irq4
    dq irq5
    dq irq6
    dq irq7
    dq irq8
    dq irq9
    dq irq10
    dq irq11
    dq irq12
    dq irq13
    dq irq14
    dq irq15
