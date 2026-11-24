BITS 64
DEFAULT REL
section .bss
global syscall_kernel_rsp
syscall_kernel_rsp: resq 1
global syscall_user_rsp
syscall_user_rsp: resq 1
global proc_kernel_return_rsp
proc_kernel_return_rsp: resq 1
global proc_exit_requested
proc_exit_requested: resq 1

section .text
global syscall_entry
global enter_user
extern syscall_dispatch
extern tss_set_rsp0

; syscall: rax=num, rdi,rsi,rdx,r10,r8,r9 args; rcx=rip, r11=rflags
syscall_entry:
    swapgs
    mov [rel syscall_user_rsp], rsp
    mov rsp, [rel syscall_kernel_rsp]
    push qword [rel syscall_user_rsp]
    push r11
    push rcx
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    push r9
    push r8
    push r10
    push rdx
    push rsi
    push rdi
    mov rdi, rax        ; n
    mov rsi, [rsp+0]    ; a1
    mov rdx, [rsp+8]    ; a2
    mov rcx, [rsp+16]   ; a3
    mov r8,  [rsp+24]   ; a4
    mov r9,  [rsp+32]   ; a5
    call syscall_dispatch
    add rsp, 48
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    pop rcx
    pop r11
    pop rsp
    ; if exit requested, return to kernel exec context
    cmp qword [rel proc_exit_requested], 0
    je .do_sysret
    mov qword [rel proc_exit_requested], 0
    mov rsp, [rel proc_kernel_return_rsp]
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret
.do_sysret:
    swapgs
    o64 sysret

; void enter_user(u64 rip, u64 rsp)
; rdi=rip, rsi=rsp
enter_user:
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    mov [rel proc_kernel_return_rsp], rsp
    mov qword [rel proc_exit_requested], 0
    ; build iretq frame: SS, RSP, RFLAGS, CS, RIP
    push qword 0x1B      ; SS = user data | 3
    push rsi             ; RSP
    push qword 0x202     ; RFLAGS (IF)
    push qword 0x23      ; CS = user code | 3
    push rdi             ; RIP
    iretq
