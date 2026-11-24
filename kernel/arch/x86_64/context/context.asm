BITS 64
DEFAULT REL
section .text
global context_switch

; void context_switch(u64 *old_rsp, u64 new_rsp)
; rdi = &old_rsp, rsi = new_rsp
context_switch:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov [rdi], rsp
    mov rsp, rsi
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret
