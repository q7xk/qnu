BITS 64
DEFAULT REL
section .text
global syscall_entry
extern syscall_dispatch

syscall_entry:
    swapgs
    push rcx      ; user rip
    push r11      ; user rflags
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov rcx, r10  ; arg4
    ; args: rdi=n, rsi=a1, rdx=a2, r10->rcx=a3, r8=a4, r9=a5  (SysV-ish, but we got from syscall regs)
    ; on syscall: rax=n, rdi=a1,rsi=a2,rdx=a3,r10=a4,r8=a5,r9=a6
    push r9
    push r8
    mov r9, r8    ; a5
    mov r8, r10   ; a4
    mov rcx, rdx  ; a3
    mov rdx, rsi  ; a2
    mov rsi, rdi  ; a1
    mov rdi, rax  ; n
    pop r8
    pop r9
    call syscall_dispatch
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    pop r11
    pop rcx
    swapgs
    o64 sysret
