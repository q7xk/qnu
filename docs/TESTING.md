# QNU Testing

## Automated boot smoke test

Run:

```bash
./scripts/test_boot.sh
```

The script builds the kernel, creates a UEFI FAT image, boots QEMU with OVMF, and checks serial output for required initialization messages.

## Manual checks

- Boot process: `make run`
- CPU/GDT/IDT: boot log shows CPU and interrupt initialization
- Memory: PMM/VMM/Heap log messages
- Timer: PIT initialized message and IRQ0 ticks
- Scheduler: scheduler initialized and idle thread created
- Panic: trigger via test exception (development only)

## Debugging

Serial output is written to COM1 and can be captured with `-serial stdio` in QEMU.
