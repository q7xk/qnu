# QNU Kernel 0.0.9

**An independent kernel, built from the ground up.**

QNU is an independent, open-source operating system kernel written from scratch in C, Assembly, Rust, and Zig. It is a modular monolithic kernel targeting `x86_64` with UEFI boot via Limine.

QNU is not a fork, derivative, or modification of Linux, FreeBSD, OpenBSD, NetBSD, XNU, GNU Hurd, MINIX, or any other existing operating system kernel.

## Status

- Version: `0.0.9`
- Stage: Experimental / Pre-alpha
- Architecture: `x86_64`
- Boot: `UEFI/BIOS` (Limine)
- Kernel: Modular monolithic
- License: `GPL-2.0-only`

## 0.0.9 Features

- Per-process address space isolation: separate PML4, kernel-half sharing, CR3 switch on exec
- Ring 3 usermode (`iretq`), TSS.RSP0, per-process kernel syscall stack
- ELF64 loader (PT_LOAD segments, user mappings)
- Syscall ABI expanded: `read/write/open/close/fstat/ioctl/pipe/dup/dup2/brk/getpid/exit/wait4(stub)/kill/uname/getcwd/chdir/getppid/arch_prctl/getdents64/clock_gettime(stub)/exit_group`
- `/dev/null`, `/dev/zero`, `/dev/console`, `/proc` VFS nodes; basic pipe/dup fd types
- Process basics: process table, pid/ppid, fd table, exec/exit/kill/ps scaffolding
- Minimal in-tree userland libc/syscall wrappers; static ELF64 binaries in `/bin` (`hello`, `uname`, `echo`, `cat`)
- Shell `exec` command runs real ELF binaries from VFS
- GNU/FreeBSD-like shell builtins retained
- **Limitations**: no preemptive user multitasking/fork/wait yet; no full POSIX signals/tty/job control; no real ext/minix disk FS; no real GNU/FreeBSD ports

## Project Structure

See [ARCHITECTURE.md](ARCHITECTURE.md) for details.

## Build & Run

See [BUILDING.md](BUILDING.md).

## Roadmap

See [ROADMAP.md](ROADMAP.md).

## License

GPL-2.0-only. See [LICENSE](LICENSE).
