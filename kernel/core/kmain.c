#include <qnu/types.h>
#include <qnu/log.h>
#include <qnu/boot.h>
#include <qnu/console.h>
#include <qnu/serial.h>
#include <qnu/arch.h>
#include <qnu/mm.h>
#include <qnu/sched.h>
#include <qnu/hal.h>
#include <qnu/panic.h>
#include <qnu/acpi.h>
#include <qnu/cpu.h>
#include <qnu/lapic.h>
#include <qnu/shell.h>
#include <qnu/pci.h>
#include <qnu/ioapic.h>
#include <qnu/vfs.h>
#include <qnu/syscall.h>

void boot_init(void);
void pic_eoi(u8 irq);
void pic_clear_mask(u8 irq);
void irq_register(u8 irq, void (*handler)(void));
void keyboard_init(void);
u64 pit_get_ticks(void);

/* Rust component */
extern void rust_kernel_init(void);
/* Zig component */
extern void zig_kernel_init(void);

static void timer_banner(void *arg) {
    (void)arg;
    /* simple idle kernel thread */
    for (;;) __asm__ volatile ("hlt");
}

void kmain(void) {
    boot_init();
    const struct qnu_boot_info *bi = boot_info_get();

    log_init();
    console_init(bi->fb_addr, bi->fb_width, bi->fb_height, bi->fb_pitch, bi->fb_bpp);

    log_write("QNU Kernel 0.0.4\n");
    log_write("Independent Operating System Kernel\n\n");
    log_write("Architecture: x86_64\n");
    log_write("Boot: UEFI\n");
    log_write("Kernel: Modular Monolithic\n\n");

    arch_cpu_init();
    log_write("[C] Core initialized\n");

    /* ASM CPU environment is set up during boot; confirm */
    log_write("[ASM] CPU environment initialized\n");

    rust_kernel_init();
    zig_kernel_init();

    pmm_init();
    vmm_init();
    heap_init();
    log_ok("Memory manager initialized");

    cpu_info_init();
    acpi_init();
    lapic_init();
    ioapic_init();
    pci_init();
    vfs_init();
    vfs_create("/hello.txt", VFS_FILE);
    vfs_write(vfs_lookup("/hello.txt"), 0, "Hello from QNU ramfs\n", 21);
    syscall_init();

    interrupts_init();
    log_ok("Interrupt manager initialized");

    pit_init(100);
    pic_clear_mask(0);
    pic_clear_mask(1);
    keyboard_init();
    log_ok("Timer initialized");

    sched_init();
    sched_create_kernel_thread("idle", timer_banner, NULL);

    log_write("\nQNU: Kernel initialized successfully.\n");

    shell_init();
    hal_irq_enable();
    lapic_timer_init(100);
    if (lapic_timer_active()) { extern void pic_set_mask(u8 irq); pic_set_mask(0); }
    if (ioapic_active()) { for (u8 i=0;i<16;i++) { extern void pic_set_mask(u8 irq); pic_set_mask(i); } }

    for (;;) {
        shell_poll();
        __asm__ volatile ("hlt");
    }
}
