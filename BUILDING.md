# Building QNU

## Dependencies

- `clang` (freestanding C compiler)
- `ld.lld`
- `nasm`
- `rustc` with `x86_64-unknown-none` target
- `zig` (0.15.x tested)
- `mtools`
- `qemu-system-x86_64`
- OVMF/edk2 x64 firmware
- Limine boot files (`/usr/share/limine/BOOTX64.EFI`)

## Build

```bash
make
```

Override tool paths if needed:

```bash
make NASM=/path/to/nasm ZIG=/path/to/zig
```

## Create boot image

```bash
make build/qnu.img
```

## Run in QEMU (UEFI)

```bash
make run QEMU=qemu-system-x86_64 OVMF=/path/to/OVMF_CODE.fd OVMF_VARS=/path/to/OVMF_VARS.fd
```

## Clean

```bash
make clean
```

## Notes

- The build targets freestanding `x86_64`.
- Rust is compiled as `staticlib` with `panic=abort`.
- Zig is compiled as a freestanding object with `-mcmodel=kernel`.
- The resulting kernel is linked into `build/qnu.elf`.
