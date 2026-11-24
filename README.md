# QNU Kernel 0.0.1

**An independent kernel, built from the ground up.**

QNU is an independent, open-source operating system kernel written from scratch in C, Assembly, Rust, and Zig. It is a modular monolithic kernel targeting `x86_64` with UEFI boot via Limine.

QNU is not a fork, derivative, or modification of Linux, FreeBSD, OpenBSD, NetBSD, XNU, GNU Hurd, MINIX, or any other existing operating system kernel.

## Status

- Version: `0.0.1`
- Stage: Experimental / Pre-alpha
- Architecture: `x86_64`
- Boot: `UEFI` (Limine)
- Kernel: Modular monolithic
- License: `GPL-2.0-only`

## 0.0.1 Features

- UEFI boot through Limine
- Serial output (COM1) and framebuffer console
- GDT/TSS setup
- IDT and CPU exception handling
- PIC initialization and IRQ dispatch
- PIT timer (100 Hz)
- PS/2 keyboard interrupt handler
- Physical memory manager (bitmap PMM)
- Virtual memory manager (4-level paging)
- Kernel heap allocator
- Basic round-robin kernel thread scheduler
- Rust `no_std` kernel component
- Zig freestanding kernel component

## Project Structure

See [ARCHITECTURE.md](ARCHITECTURE.md) for details.

## Build & Run

See [BUILDING.md](BUILDING.md).

## Roadmap

See [ROADMAP.md](ROADMAP.md).

## License

GPL-2.0-only. See [LICENSE](LICENSE).
