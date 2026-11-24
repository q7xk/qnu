#include <qnu/shell.h>
#include <qnu/log.h>
#include <qnu/mm.h>
#include <qnu/boot.h>
#include <qnu/acpi.h>
#include <qnu/cpu.h>
#include <qnu/io.h>
#include <qnu/pci.h>
#include <qnu/boot_internal.h>
#include <limine.h>

static char buf[128];
static int len = 0;
static bool ready = false;

static int streq(const char *a, const char *b) {
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return *a == *b;
}

static void prompt(void) { log_write("qnu> "); }

static void run_cmd(const char *cmd) {
    if (streq(cmd, "help")) {
        log_write("Commands: help version mem heap memmap cpu acpi pci ticks reboot clear\n");
    } else if (streq(cmd, "version")) {
        log_write("QNU Kernel 0.0.3\n");
    } else if (streq(cmd, "mem")) {
        const struct qnu_boot_info *bi = boot_info_get();
        log_printf("total=%luMB usable=%luMB free_pages=%lu\n", bi->total_memory/1024/1024, bi->usable_memory/1024/1024, pmm_free_pages());
    } else if (streq(cmd, "heap")) {
        u64 t=0,u=0; heap_stats(&t,&u);
        log_printf("heap total=%lu used=%lu free=%lu\n", t, u, t-u);
    } else if (streq(cmd, "memmap")) {
        struct limine_memmap_response *mm = boot_memmap();
        if (!mm) { log_write("no memmap\n"); }
        else for (u64 i=0;i<mm->entry_count;i++){ struct limine_memmap_entry *e=mm->entries[i]; log_printf(" %x base=%lx len=%lx\n", e->type, e->base, e->length); }
    } else if (streq(cmd, "cpu")) {
        cpu_info_dump();
    } else if (streq(cmd, "acpi")) {
        acpi_dump();
    } else if (streq(cmd, "pci")) {
        pci_dump();
    } else if (streq(cmd, "ticks")) {
        extern u64 pit_get_ticks(void);
        log_printf("ticks=%lu\n", pit_get_ticks());
    } else if (streq(cmd, "clear")) {
        log_write("\n");
    } else if (streq(cmd, "reboot")) {
        log_write("Rebooting...\n");
        struct { u16 l; u64 b; } __attribute__((packed)) idtr = {0,0};
        __asm__ volatile("lidt %0; int $0x03" :: "m"(idtr));
    } else if (len > 0) {
        log_printf("Unknown command: %s\n", cmd);
    }
}

void shell_init(void) { len = 0; ready = true; log_write("\nQNU shell ready. Type 'help'.\n"); prompt(); }

void shell_input_char(char c) {
    if (!ready) return;
    if (c == '\r') c = '\n';
    if (c == '\n') {
        log_write("\n");
        buf[len] = 0;
        run_cmd(buf);
        len = 0;
        prompt();
        return;
    }
    if ((c == '\b' || c == 127) && len > 0) { len--; log_write("\b \b"); return; }
    if (c >= 32 && c < 127 && len < 127) { buf[len++] = c; char s[2]={c,0}; log_write(s); }
}

void shell_poll(void) {
    /* poll COM1 */
    if (inb(0x3FD) & 1) {
        char c = (char)inb(0x3F8);
        shell_input_char(c);
    }
}
