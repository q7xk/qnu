# QNU Roadmap

## 0.0.4.5 (current)

- [x] UEFI boot via Limine
- [x] Core C kernel initialization
- [x] NASM CPU/interrupt/context stubs
- [x] Rust `no_std` component
- [x] Zig freestanding component
- [x] GDT/IDT/exception handling
- [x] PMM/VMM/heap
- [x] PIC + PIT + keyboard IRQ
- [x] Basic kernel scheduler foundation

## 0.0.4.5 (current)

- [x] ACPI RSDP/XSDT parsing and table enumeration
- [x] CPUID CPU vendor/feature detection
- [x] LAPIC detection and initialization (MMIO mapping, SVR/EOI)
- [x] Serial debug shell (help/version/mem/cpu/acpi/ticks/reboot/clear)
- [x] Keyboard input routed to shell
- [x] Memory stats reporting (PMM free pages, boot memory totals)
- [~] APIC timer (calibration/fallback groundwork; PIT still drives scheduler)
- [ ] Robust kernel heap (slab/buddy/hybrid)
- [ ] Full SMP bring-up

## 0.0.4.5 (current)

- [x] MADT parsing (LAPIC/IOAPIC/CPU count)
- [x] PCI bus enumeration (config space scan, shell `pci`)
- [x] Memory map and heap stats shell commands (`memmap`, `heap`)
- [x] Keyboard shift/capslock handling
- [x] Shell command set expanded
- [~] LAPIC timer calibrated groundwork (PIT still drives scheduler)
- [ ] IOAPIC IRQ routing
- [ ] HPET/APIC timer as scheduler source

## 0.0.4.5 (current)

- [x] LAPIC timer calibrated and used as scheduler tick source (PIT masked after handoff)
- [x] IOAPIC initialized and keyboard IRQ routed via IOAPIC
- [x] Heap improved with slab+free-list allocator
- [x] VFS foundation with ramfs (/, /hello.txt, ls/cat)
- [x] Syscall interface initialized (MSR LSTAR/STAR/SFMASK, dispatch table)
- [x] Basic fd table and syscall handlers (read/write/open/close/getpid/exit)
- [~] Unix-like structure: foundations laid, not full POSIX/userland
- [ ] Ring 3 usermode, ELF loader, fork/exec/wait
- [ ] Disk drivers (AHCI/NVMe), persistent FS
- [ ] SMP bring-up
- [ ] Networking, USB, graphics, sound
