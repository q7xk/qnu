NAME := qnu
KERNEL := build/$(NAME).elf
ISO := build/$(NAME).iso
IMG := build/$(NAME).img

NASM ?= /tmp/opencode/qnu-tools/extract/nasm/usr/bin/nasm
CC := clang
LD := ld.lld
RUSTC := rustc
ZIG ?= /tmp/opencode/qnu-tools/zig-x86_64-linux-0.15.2/zig

CFLAGS := -std=c11 -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone \
	-mno-mmx -mno-sse -mno-sse2 -mcmodel=kernel -Wall -Wextra -O2 \
	-Ikernel/include -Ikernel/drivers
LDFLAGS := -nostdlib -static -z max-page-size=0x1000 -T linker/linker.ld

C_SRCS := $(shell find kernel -name '*.c')
ASM_SRCS := $(shell find kernel -name '*.asm')
C_OBJS := $(patsubst %.c,build/%.o,$(C_SRCS))
ASM_OBJS := $(patsubst %.asm,build/%.o,$(ASM_SRCS))
RUST_LIB := build/rust/libqnu_rust.a
ZIG_OBJ := build/zig/init.o

.PHONY: all clean run run-uefi dirs

all: $(KERNEL)

dirs:
	@mkdir -p $(dir $(C_OBJS) $(ASM_OBJS)) build/rust build/zig build/iso/boot/limine build/iso/EFI/BOOT

build/%.o: %.c | dirs
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: %.asm | dirs
	@mkdir -p $(dir $@)
	$(NASM) -f elf64 $< -o $@

$(RUST_LIB): kernel/rust/src/lib.rs | dirs
	$(RUSTC) --target x86_64-unknown-none -C opt-level=2 -C panic=abort --crate-type staticlib $< -o $@

$(ZIG_OBJ): kernel/zig/src/init.zig | dirs
	$(ZIG) build-obj $< -target x86_64-freestanding -O ReleaseSmall -mcmodel=kernel -femit-bin=$@

$(KERNEL): $(ASM_OBJS) $(C_OBJS) $(RUST_LIB) $(ZIG_OBJ) linker/linker.ld | dirs
	$(LD) $(LDFLAGS) $(ASM_OBJS) $(C_OBJS) $(ZIG_OBJ) $(RUST_LIB) -o $@
	@echo "Built $(KERNEL)"

$(IMG): $(KERNEL)
	@mkdir -p build/iso/boot/limine build/iso/EFI/BOOT
	cp $(KERNEL) build/iso/boot/qnu.elf
	cp boot/config/limine.conf build/iso/boot/limine/limine.conf
	cp /usr/share/limine/limine-bios.sys build/iso/boot/limine/ 2>/dev/null || true
	cp /usr/share/limine/limine-bios-cd.bin build/iso/boot/limine/ 2>/dev/null || true
	cp /usr/share/limine/limine-uefi-cd.bin build/iso/boot/limine/ 2>/dev/null || true
	cp /usr/share/limine/BOOTX64.EFI build/iso/EFI/BOOT/BOOTX64.EFI
	cp /usr/share/limine/BOOTIA32.EFI build/iso/EFI/BOOT/BOOTIA32.EFI 2>/dev/null || true
	# Create FAT image for UEFI boot
	dd if=/dev/zero of=$(IMG) bs=1M count=64 status=none
	LC_ALL=C mformat -i $(IMG) -F ::
	LC_ALL=C mmd -i $(IMG) ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine ::/limine
	LC_ALL=C mcopy -i $(IMG) build/iso/boot/qnu.elf ::/boot/qnu.elf
	LC_ALL=C mcopy -i $(IMG) boot/config/limine.conf ::/boot/limine/
	LC_ALL=C mcopy -i $(IMG) boot/config/limine.conf ::/limine/
	LC_ALL=C mcopy -i $(IMG) boot/config/limine.conf ::/EFI/BOOT/
	LC_ALL=C mcopy -i $(IMG) /usr/share/limine/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
	@echo "Built $(IMG)"

run: $(IMG)
	@QEMU=$${QEMU:-qemu-system-x86_64}; \
	OVMF=$${OVMF:-/tmp/opencode/qnu-tools/extract/ovmf/usr/share/edk2/x64/OVMF_CODE.4m.fd}; \
	OVMF_VARS=$${OVMF_VARS:-/tmp/opencode/qnu-tools/extract/ovmf/usr/share/edk2/x64/OVMF_VARS.4m.fd}; \
	cp $$OVMF_VARS build/OVMF_VARS.fd; \
	$$QEMU -machine q35 -m 256M -serial stdio -no-reboot -no-shutdown \
		-drive if=pflash,format=raw,readonly=on,file=$$OVMF \
		-drive if=pflash,format=raw,file=build/OVMF_VARS.fd \
		-drive format=raw,file=$(IMG)

clean:
	rm -rf build
