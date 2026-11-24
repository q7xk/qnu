# QNU Kernel 0.0.6

**An independent kernel, built from the ground up.**

QNU is an independent, open-source operating system kernel written from scratch in C, Assembly, Rust, and Zig. It is a modular monolithic kernel targeting `x86_64` with UEFI boot via Limine.

QNU is not a fork, derivative, or modification of Linux, FreeBSD, OpenBSD, NetBSD, XNU, GNU Hurd, MINIX, or any other existing operating system kernel.

## Status

- Version: `0.0.6`
- Stage: Experimental / Pre-alpha
- Architecture: `x86_64`
- Boot: `UEFI/BIOS` (Limine)
- Kernel: Modular monolithic
- License: `GPL-2.0-only`

## 0.0.6 Features

- Everything in 0.0.4.x
- Btrfs-like RAMFS: subvolumes, CoW file writes, CRC32 checksums, snapshot
- `/home/kullanici` default layout
- Shell FS commands: `ls`, `cat`, `mkdir`, `subvol`, `snap`, `info`
- WiFi/BLE: PCI detection/reporting only (no full stack yet)

## Project Structure

See [ARCHITECTURE.md](ARCHITECTURE.md) for details.

## Build & Run

See [BUILDING.md](BUILDING.md).

## Roadmap

See [ROADMAP.md](ROADMAP.md).

## License

GPL-2.0-only. See [LICENSE](LICENSE).
