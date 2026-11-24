# QNU Kernel 0.0.3

**An independent kernel, built from the ground up.**

QNU is an independent, open-source operating system kernel written from scratch in C, Assembly, Rust, and Zig. It is a modular monolithic kernel targeting `x86_64` with UEFI boot via Limine.

QNU is not a fork, derivative, or modification of Linux, FreeBSD, OpenBSD, NetBSD, XNU, GNU Hurd, MINIX, or any other existing operating system kernel.

## Status

- Version: `0.0.3`
- Stage: Experimental / Pre-alpha
- Architecture: `x86_64`
- Boot: `UEFI` (Limine)
- Kernel: Modular monolithic
- License: `GPL-2.0-only`

## 0.0.3 Features

- Everything in 0.0.2
- MADT parsing: LAPIC/IOAPIC addresses and CPU count
- PCI config-space enumeration (`pci`)
- Shell additions: `heap`, `memmap`, `pci`
- Keyboard: shift and capslock handling
- Boot smoke test passes in QEMU/OVMF

## Project Structure

See [ARCHITECTURE.md](ARCHITECTURE.md) for details.

## Build & Run

See [BUILDING.md](BUILDING.md).

## Roadmap

See [ROADMAP.md](ROADMAP.md).

## License

GPL-2.0-only. See [LICENSE](LICENSE).
