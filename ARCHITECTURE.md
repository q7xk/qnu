# QNU Architecture

## Overview

QNU 0.0.4.7 is a modular monolithic kernel for x86_64. Core subsystems run in kernel space and communicate through internal C interfaces. Architecture-specific code is isolated under `kernel/arch/x86_64/`.

## Language Responsibilities

| Language | Responsibility | Key Files |
|---|---|---|
| C | Core kernel, MM, interrupts, scheduler, drivers, HAL, logging | `kernel/core`, `kernel/mm`, `kernel/sched`, `kernel/drivers`, `kernel/hal`, `kernel/arch/x86_64/cpu`, `kernel/arch/x86_64/interrupts` |
| NASM Assembly | Kernel entry, GDT/TSS flush, ISR/IRQ stubs, context switch | `kernel/arch/x86_64/boot/entry.asm`, `kernel/arch/x86_64/cpu/gdt_flush.asm`, `kernel/arch/x86_64/interrupts/isr_stubs.asm`, `kernel/arch/x86_64/context/context.asm` |
| Rust (`no_std`) | Memory-safe kernel component initialization | `kernel/rust/src/lib.rs` |
| Zig (freestanding) | Low-level systems component initialization | `kernel/zig/src/init.zig` |

## Boot Flow

1. Limine loads `boot/qnu.elf` via UEFI.
2. `kernel/arch/x86_64/boot/entry.asm` sets up stack and calls `kmain`.
3. `boot_init()` validates Limine base revision and parses boot info.
4. Serial and framebuffer console are initialized.
5. CPU/GDT/IDT are initialized.
6. Rust and Zig components are initialized.
7. PMM, VMM, heap are initialized.
8. PIC/IRQ/timer/keyboard are initialized.
9. Scheduler is initialized and kernel enters idle loop.

## Memory Management

- **PMM**: Bitmap allocator over Limine usable memory map. Returns HHDM-mapped physical pages.
- **VMM**: 4-level x86_64 paging, reuses Limine PML4, supports page mapping with `invlpg`.
- **Heap**: Simple linked-list kernel heap with coalescing, backed by PMM+VMM mapped pages.

## Interrupts

- IDT contains 32 CPU exception vectors and 16 PIC IRQ vectors.
- Common ASM stubs save registers and dispatch to C handlers.
- Exceptions panic with vector, error code, RIP/RSP, and CR2 for page faults.
- IRQ 0 drives PIT ticks and scheduler tick; IRQ 1 handles PS/2 keyboard scancodes.

## Scheduler

- Basic kernel task structure with states: READY, RUNNING, SLEEPING, DEAD.
- Round-robin preemption on PIT ticks.
- Context switch saves/restores callee-saved registers and swaps RSP.

## HAL

`kernel/hal/hal.c` provides a small abstraction for CPU halt, IRQ enable/disable, and timer tick access.

## FFI Boundaries

- C calls Rust via `extern void rust_kernel_init(void);`.
- C calls Zig via `extern void zig_kernel_init(void);`.
- Rust/Zig call C logging via `extern fn log_write(s: *const u8) void` / C ABI.
- All cross-language symbols use C ABI and no name mangling on exported init functions.
