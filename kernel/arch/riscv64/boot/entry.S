.section .text
.global _start
_start:
    la sp, stack_top
    call kmain
1:  wfi
    j 1b

.section .bss
.align 16
stack_bottom:
    .skip 65536
stack_top:
