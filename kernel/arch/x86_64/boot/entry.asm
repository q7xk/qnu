BITS 64
DEFAULT REL

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
global _start
extern kmain

_start:
    cli
    lea rsp, [stack_top]
    xor rbp, rbp
    call kmain
.hang:
    cli
    hlt
    jmp .hang
