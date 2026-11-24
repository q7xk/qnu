#include <qnu/panic.h>
#include <qnu/log.h>

__attribute__((noreturn)) static inline void cli_hlt(void) {
    for (;;) __asm__ volatile ("cli; hlt");
}

__attribute__((noreturn)) void panic(const char *msg) {
    log_write("\n[KERNEL PANIC] ");
    log_write(msg ? msg : "(unknown)");
    log_write("\n");
    cli_hlt();
}

void qnu_assert(bool cond, const char *expr, const char *file, int line) {
    if (cond) return;
    log_printf("\n[ASSERT FAILED] %s at %s:%d\n", expr, file, line);
    panic("assertion failed");
}
