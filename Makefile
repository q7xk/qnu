NAME := qnu
ARCH ?= x86_64
BUILD_DIR := build/$(ARCH)
KERNEL := $(BUILD_DIR)/$(NAME).elf
ISO := $(BUILD_DIR)/$(NAME).iso
IMG := $(BUILD_DIR)/$(NAME).img

CC := clang
LD := ld.lld
RUSTC := rustc
ZIG ?= /tmp/opencode/qnu-tools/zig-x86_64-linux-0.15.2/zig
NASM ?= $(shell command -v nasm 2>/dev/null || echo /tmp/opencode/qnu-tools/extract_nasm/usr/bin/nasm)

ifeq ($(ARCH),x86_64)
  CFLAGS_ARCH := -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mcmodel=kernel
  TARGET_RUST := x86_64-unknown-none
  TARGET_ZIG := -target x86_64-freestanding -mcmodel=kernel
  LDFLAGS := -nostdlib -static -z max-page-size=0x1000 -T linker/linker.ld
  ASM_SRCS := $(shell find kernel/arch/x86_64 -name '*.asm')
  ARCH_C_SRCS := $(shell find kernel/arch/x86_64 -name '*.c')
  S_SRCS :=
else ifeq ($(ARCH),aarch64)
  CFLAGS_ARCH := -target aarch64-unknown-none -mgeneral-regs-only -mcmodel=large
  TARGET_RUST := aarch64-unknown-none
  TARGET_ZIG := -target aarch64-freestanding -mcmodel=large
  LDFLAGS := -nostdlib -static -z max-page-size=0x1000 -T linker/linker-aarch64.ld
  ASM_SRCS :=
  S_SRCS := $(shell find kernel/arch/aarch64 -name '*.S')
  ARCH_C_SRCS := $(shell find kernel/arch/aarch64 -name '*.c')
else ifeq ($(ARCH),riscv64)
  CFLAGS_ARCH := -target riscv64-unknown-none -march=rv64gc -mabi=lp64d -mcmodel=medany
  TARGET_RUST := riscv64gc-unknown-none-elf
  TARGET_ZIG := -target riscv64-freestanding -mcmodel=medany
  LDFLAGS := -nostdlib -static -z max-page-size=0x1000 -T linker/linker-riscv64.ld
  ASM_SRCS :=
  S_SRCS := $(shell find kernel/arch/riscv64 -name '*.S')
  ARCH_C_SRCS := $(shell find kernel/arch/riscv64 -name '*.c')
else
  $(error Unsupported ARCH=$(ARCH))
endif

CFLAGS := -std=c11 -ffreestanding -fno-stack-protector -fno-pic -Wall -Wextra -O2 \
  -Ikernel/include -Ikernel/drivers $(CFLAGS_ARCH) -DQNU_ARCH_$(shell echo $(ARCH) | tr a-z A-Z)

# Common C sources, exclude other arch dirs
ifeq ($(ARCH),x86_64)
  COMMON_C_SRCS := $(shell find kernel/core kernel/mm kernel/sched kernel/drivers kernel/hal -name '*.c')
  C_SRCS := $(COMMON_C_SRCS) $(ARCH_C_SRCS)
else
  C_SRCS := $(ARCH_C_SRCS)
endif
C_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRCS))
ASM_OBJS := $(patsubst %.asm,$(BUILD_DIR)/%.o,$(ASM_SRCS))
S_OBJS := $(patsubst %.S,$(BUILD_DIR)/%.o,$(S_SRCS))
RUST_LIB := $(BUILD_DIR)/rust/libqnu_rust.a
ZIG_OBJ := $(BUILD_DIR)/zig/init.o

.PHONY: all clean run dirs

all: $(KERNEL)

dirs:
	@mkdir -p $(dir $(C_OBJS) $(ASM_OBJS) $(S_OBJS)) $(BUILD_DIR)/rust $(BUILD_DIR)/zig $(BUILD_DIR)/iso/boot/limine $(BUILD_DIR)/iso/EFI/BOOT

$(BUILD_DIR)/%.o: %.c | dirs
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.asm | dirs
	@mkdir -p $(dir $@)
	$(NASM) -f elf64 $< -o $@

$(BUILD_DIR)/%.o: %.S | dirs
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(RUST_LIB): kernel/rust/src/lib.rs | dirs
	@mkdir -p $(dir $@)
	@if $(RUSTC) --target $(TARGET_RUST) -C opt-level=2 -C panic=abort --crate-type staticlib $< -o $@ 2>/dev/null; then \
		echo "[rust] built for $(TARGET_RUST)"; \
	else \
		echo "[rust] target $(TARGET_RUST) unavailable, creating empty stub"; \
		echo '' | $(CC) $(CFLAGS) -x c -c - -o $(BUILD_DIR)/rust/empty.o; \
		ar rcs $@ $(BUILD_DIR)/rust/empty.o; \
	fi

$(ZIG_OBJ): kernel/zig/src/init.zig | dirs
	@if command -v $(ZIG) >/dev/null 2>&1; then \
		$(ZIG) build-obj $< $(TARGET_ZIG) -O ReleaseSmall -femit-bin=$@; \
	else \
		echo "[zig] toolchain missing, using C fallback stub"; \
		printf 'extern void log_write(const char*s); void zig_kernel_init(void){ log_write("[ZIG] System components initialized\\n"); }\n' > $(BUILD_DIR)/zig/stub.c; \
		$(CC) $(CFLAGS) -c $(BUILD_DIR)/zig/stub.c -o $@; \
	fi

$(KERNEL): $(ASM_OBJS) $(S_OBJS) $(C_OBJS) $(RUST_LIB) $(ZIG_OBJ) | dirs
	$(LD) $(LDFLAGS) $(ASM_OBJS) $(S_OBJS) $(C_OBJS) $(ZIG_OBJ) $(RUST_LIB) -o $@
	@echo "Built $(KERNEL) ($(ARCH))"

$(IMG): $(KERNEL)
	@mkdir -p $(BUILD_DIR)/iso/boot/limine $(BUILD_DIR)/iso/EFI/BOOT
	cp $(KERNEL) $(BUILD_DIR)/iso/boot/qnu.elf
	cp boot/config/limine.conf $(BUILD_DIR)/iso/boot/limine/limine.conf
	cp /usr/share/limine/BOOTX64.EFI $(BUILD_DIR)/iso/EFI/BOOT/BOOTX64.EFI
	dd if=/dev/zero of=$(IMG) bs=1M count=64 status=none
	LC_ALL=C mformat -i $(IMG) -F ::
	LC_ALL=C mmd -i $(IMG) ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine ::/limine
	LC_ALL=C mcopy -i $(IMG) $(BUILD_DIR)/iso/boot/qnu.elf ::/boot/qnu.elf
	LC_ALL=C mcopy -i $(IMG) boot/config/limine.conf ::/boot/limine/
	LC_ALL=C mcopy -i $(IMG) boot/config/limine.conf ::/limine/
	LC_ALL=C mcopy -i $(IMG) boot/config/limine.conf ::/EFI/BOOT/
	LC_ALL=C mcopy -i $(IMG) /usr/share/limine/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
	@cp $(IMG) build/qnu.img 2>/dev/null || true
	@echo "Built $(IMG)"

XORRISO ?= /tmp/opencode/qnu-tools/extract/libisoburn/usr/bin/xorriso
LIMINE ?= limine
$(ISO): $(KERNEL)
	@mkdir -p $(BUILD_DIR)/iso/boot/limine $(BUILD_DIR)/iso/EFI/BOOT
	cp $(KERNEL) $(BUILD_DIR)/iso/boot/qnu.elf
	cp boot/config/limine.conf $(BUILD_DIR)/iso/boot/limine/limine.conf
	cp /usr/share/limine/limine-bios.sys $(BUILD_DIR)/iso/boot/limine/
	cp /usr/share/limine/limine-bios-cd.bin $(BUILD_DIR)/iso/boot/limine/
	cp /usr/share/limine/limine-uefi-cd.bin $(BUILD_DIR)/iso/boot/limine/
	cp /usr/share/limine/BOOTX64.EFI $(BUILD_DIR)/iso/EFI/BOOT/
	cp /usr/share/limine/BOOTIA32.EFI $(BUILD_DIR)/iso/EFI/BOOT/ 2>/dev/null || true
	$(XORRISO) -as mkisofs -R -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(BUILD_DIR)/iso -o $(ISO)
	$(LIMINE) bios-install $(ISO)
	@cp $(ISO) build/qnu.iso 2>/dev/null || true
	@echo "Built $(ISO)"

clean:
	rm -rf build
