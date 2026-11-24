# QNU Roadmap

## 0.0.3 (current)

- [x] UEFI boot via Limine
- [x] Core C kernel initialization
- [x] NASM CPU/interrupt/context stubs
- [x] Rust `no_std` component
- [x] Zig freestanding component
- [x] GDT/IDT/exception handling
- [x] PMM/VMM/heap
- [x] PIC + PIT + keyboard IRQ
- [x] Basic kernel scheduler foundation

## 0.0.3 (current)

- [x] ACPI RSDP/XSDT parsing and table enumeration
- [x] CPUID CPU vendor/feature detection
- [x] LAPIC detection and initialization (MMIO mapping, SVR/EOI)
- [x] Serial debug shell (help/version/mem/cpu/acpi/ticks/reboot/clear)
- [x] Keyboard input routed to shell
- [x] Memory stats reporting (PMM free pages, boot memory totals)
- [~] APIC timer (calibration/fallback groundwork; PIT still drives scheduler)
- [ ] Robust kernel heap (slab/buddy/hybrid)
- [ ] Full SMP bring-up

## 0.0.3 (current)

- [x] MADT parsing (LAPIC/IOAPIC/CPU count)
- [x] PCI bus enumeration (config space scan, shell `pci`)
- [x] Memory map and heap stats shell commands (`memmap`, `heap`)
- [x] Keyboard shift/capslock handling
- [x] Shell command set expanded
- [~] LAPIC timer calibrated groundwork (PIT still drives scheduler)
- [ ] IOAPIC IRQ routing
- [ ] HPET/APIC timer as scheduler source

## 0.0.4+

- [ ] APIC timer / HPET as scheduler time source
- [ ] IOAPIC interrupt routing
- [ ] Better physical/virtual memory region tracking
- [ ] Disk drivers (AHCI/NVMe)
- [ ] VFS and filesystem support
- [ ] Userspace, syscalls, ELF loading
- [ ] Networking stack
- [ ] USB, graphics, sound
