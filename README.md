# QNU Kernel 0.0.4.5

**An independent kernel, built from the ground up.**

QNU is an independent, open-source operating system kernel written from scratch in C, Assembly, Rust, and Zig. It is a modular monolithic kernel targeting `x86_64` with UEFI boot via Limine.

QNU is not a fork, derivative, or modification of Linux, FreeBSD, OpenBSD, NetBSD, XNU, GNU Hurd, MINIX, or any other existing operating system kernel.

## Status

- Version: `0.0.4.5`
- Stage: Experimental / Pre-alpha
- Architecture: `x86_64`
- Boot: `UEFI/BIOS` (Limine)
- Kernel: Modular monolithic
- License: `GPL-2.0-only`

## 0.0.4.5 Features

- Everything in 0.0.3
- LAPIC timer calibrated and used as scheduler tick source
- IOAPIC initialized; keyboard IRQ routed through IOAPIC
- Heap: slab+free-list allocator
- VFS foundation: ramfs, `ls`, `cat`
- Syscall interface: MSRs configured, dispatch table, basic fd/syscall handlers
- Unix-like foundation started (not full userland/process isolation yet)

## Project Structure

See [ARCHITECTURE.md](ARCHITECTURE.md) for details.

## Build & Run

See [BUILDING.md](BUILDING.md).

## Roadmap

See [ROADMAP.md](ROADMAP.md).

## License

GPL-2.0-only. See [LICENSE](LICENSE).
