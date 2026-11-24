# QNU Roadmap

## 0.0.1 (current)

- [x] UEFI boot via Limine
- [x] Core C kernel initialization
- [x] NASM CPU/interrupt/context stubs
- [x] Rust `no_std` component
- [x] Zig freestanding component
- [x] GDT/IDT/exception handling
- [x] PMM/VMM/heap
- [x] PIC + PIT + keyboard IRQ
- [x] Basic kernel scheduler foundation

## 0.0.2+

- [ ] Better physical/virtual memory region tracking
- [ ] Robust kernel heap (slab/buddy/hybrid)
- [ ] APIC/HPET timer support
- [ ] SMP bring-up
- [ ] ACPI table parsing
- [ ] PCI/PCIe enumeration
- [ ] Disk drivers (AHCI/NVMe)
- [ ] VFS and filesystem support
- [ ] Userspace, syscalls, ELF loading
- [ ] Networking stack
- [ ] USB, graphics, sound
