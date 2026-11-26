OUTPUT_FORMAT(elf64-littleaarch64)
OUTPUT_ARCH(aarch64)
ENTRY(_start)
PHDRS { text PT_LOAD FLAGS(5); rodata PT_LOAD FLAGS(4); data PT_LOAD FLAGS(6); }
SECTIONS {
  . = 0xffffffff80000000;
  .text : { *(.text .text.*) } :text
  . = ALIGN(CONSTANT(MAXPAGESIZE));
  .rodata : { *(.rodata .rodata.*) } :rodata
  .limine_requests : { KEEP(*(.limine_requests_start)) KEEP(*(.limine_requests)) KEEP(*(.limine_requests_end)) } :rodata
  . = ALIGN(CONSTANT(MAXPAGESIZE));
  .data : { *(.data .data.*) } :data
  .bss : { *(COMMON) *(.bss .bss.*) } :data
  /DISCARD/ : { *(.eh_frame*) *(.note .note.*) *(.comment) }
}
