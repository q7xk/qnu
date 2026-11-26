.section .text
.global _start
_start:
    /* Limine aarch64 entry: x0 = 0, stack set by bootloader? we set our own */
    ldr x1, =stack_top
    mov sp, x1
    bl kmain
1:  wfe
    b 1b

.section .bss
.align 16
stack_bottom:
    .skip 65536
stack_top:
