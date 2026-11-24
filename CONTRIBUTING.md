# Contributing to QNU

## Principles

- Keep QNU independent: do not copy code from existing kernels.
- Prefer small, tested, documented changes.
- Keep architecture-specific code under `kernel/arch/<arch>/`.
- Document FFI boundaries and subsystem ownership.

## Development

1. Build with `make`.
2. Test with `make run`.
3. Use serial logs for debugging.
4. Add or update tests in `tests/` where possible.

## Code Style

- C: C11, freestanding, explicit types, clear error handling.
- Assembly: NASM x86_64, minimal and documented.
- Rust: `no_std`, `panic=abort`, explicit ABI.
- Zig: freestanding, explicit ABI, minimal runtime.
