BITS 64
DEFAULT REL
section .text
global gdt_flush
global tss_flush

gdt_flush:
    lgdt [rdi]
    push 0x08
    lea rax, [rel .reload]
    push rax
    retfq
.reload:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov ax, 0x0
    mov fs, ax
    mov gs, ax
    ret

tss_flush:
    mov ax, 0x28
    ltr ax
    ret
