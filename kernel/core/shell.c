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
#include <qnu/vfs.h>
#include <qnu/sched.h>

static char buf[128];
static int len = 0;
static bool ready = false;

static int streq(const char *a, const char *b) {
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return *a == *b;
}

static void prompt(void) { log_write("qnu> "); }

static const char *arg1(const char *cmd){ while(*cmd && *cmd!=' ') cmd++; while(*cmd==' ') cmd++; return cmd; }
static void run_cmd(const char *cmd) {
    char a[128]; int i=0; while(cmd[i] && cmd[i]!=' ' && i<127){a[i]=cmd[i];i++;} a[i]=0;
    const char *rest = arg1(cmd);
    if (streq(a, "help")) {
        log_write("Commands: help version uptime ps mem heap memmap cpu acpi pci wifi bluetooth ls cat mkdir subvol snap info ticks reboot clear\n");
    } else if (streq(a, "version")) {
        log_write("QNU Kernel 0.0.5\n");
    } else if (streq(a, "mem")) {
        const struct qnu_boot_info *bi = boot_info_get();
        log_printf("total=%luMB usable=%luMB free_pages=%lu\n", bi->total_memory/1024/1024, bi->usable_memory/1024/1024, pmm_free_pages());
    } else if (streq(a, "heap")) {
        u64 t=0,u=0; heap_stats(&t,&u); log_printf("heap total=%lu used=%lu free=%lu\n", t, u, t-u);
    } else if (streq(a, "memmap")) {
        struct limine_memmap_response *mm = boot_memmap();
        if (!mm) { log_write("no memmap\n"); }
        else for (u64 i2=0;i2<mm->entry_count;i2++){ struct limine_memmap_entry *e=mm->entries[i2]; log_printf(" %x base=%lx len=%lx\n", e->type, e->base, e->length); }
    } else if (streq(a, "cpu")) { cpu_info_dump(); }
    else if (streq(a, "acpi")) { acpi_dump(); }
    else if (streq(a, "pci")) { pci_dump(); }
    else if (streq(a, "wifi")) { pci_wireless_dump(); log_write("WiFi stack: not implemented\n"); }
    else if (streq(a, "bluetooth")) { pci_wireless_dump(); log_write("BLE stack: not implemented\n"); }
    else if (streq(a, "ls")) { vfs_ls(*rest?rest:"/"); }
    else if (streq(a, "cat")) { const char *p=*rest?rest:"/hello.txt"; vfs_node_t *n=vfs_lookup(p); if(n){ char buf[256]; int r=vfs_read(n,0,buf,255); if(r>0){buf[r]=0; log_write(buf);} else log_write("\n"); } else log_write("not found\n"); }
    else if (streq(a, "mkdir")) { if(*rest) { if(vfs_mkdir(rest)) log_write("ok\n"); else log_write("err\n"); } }
    else if (streq(a, "subvol")) { if(*rest) { if(vfs_subvol_create(rest)) log_write("ok\n"); else log_write("err\n"); } }
    else if (streq(a, "snap")) { char src[128],dst[128]; int si=0,di=0; const char*r=rest; while(*r&&*r!=' '&&si<127)src[si++]=*r++; src[si]=0; while(*r==' ')r++; while(*r&&di<127)dst[di++]=*r++; dst[di]=0; if(si&&di){ if(vfs_snapshot(src,dst)==0) log_write("ok\n"); else log_write("err\n"); } else log_write("usage: snap <src> <dst>\n"); }
    else if (streq(a, "info")) { vfs_info(*rest?rest:"/"); }
    else if (streq(a, "uptime")) { extern u64 pit_get_ticks(void); u64 t=pit_get_ticks(); log_printf("uptime: %lu ticks (~%lu s)\n", t, t/100); }
    else if (streq(a, "ps")) { sched_ps(); }
    else if (streq(a, "ticks")) { extern u64 pit_get_ticks(void); log_printf("ticks=%lu sched=%lu\n", pit_get_ticks(), sched_get_ticks()); }
    else if (streq(a, "clear")) { log_write("\n"); }
    else if (streq(a, "reboot")) { log_write("Rebooting...\n"); struct { u16 l; u64 b; } __attribute__((packed)) idtr = {0,0}; __asm__ volatile("lidt %0; int $0x03" :: "m"(idtr)); }
    else if (len > 0) { log_printf("Unknown command: %s\n", cmd); }
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
