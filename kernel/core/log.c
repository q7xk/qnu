#include <qnu/log.h>
#include <qnu/serial.h>
#include <qnu/console.h>
#include <stdarg.h>

static void putc_both(char c) {
    serial_putc(c);
    console_putc(c);
}

static void write_both(const char *s) {
    while (*s) putc_both(*s++);
}

static void print_uint(u64 val, u32 base, bool upper) {
    char buf[32];
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    int i = 0;
    if (val == 0) { putc_both('0'); return; }
    while (val > 0 && i < 31) {
        buf[i++] = digits[val % base];
        val /= base;
    }
    while (i--) putc_both(buf[i]);
}

static void print_int(i64 val) {
    if (val < 0) { putc_both('-'); print_uint((u64)(-(val + 1)) + 1, 10, false); }
    else print_uint((u64)val, 10, false);
}

void log_init(void) {
    serial_init();
}

void log_putc(char c) { putc_both(c); }

void log_write(const char *s) { write_both(s); }

void log_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    for (usize i = 0; fmt[i]; i++) {
        if (fmt[i] != '%') { putc_both(fmt[i]); continue; }
        i++;
        bool is_long = false;
        if (fmt[i] == 'l') { is_long = true; i++; }
        switch (fmt[i]) {
            case 's': { const char *s = va_arg(ap, const char*); write_both(s ? s : "(null)"); break; }
            case 'c': putc_both((char)va_arg(ap, int)); break;
            case 'd': if (is_long) print_int(va_arg(ap, i64)); else print_int(va_arg(ap, int)); break;
            case 'u': if (is_long) print_uint(va_arg(ap, u64), 10, false); else print_uint(va_arg(ap, u32), 10, false); break;
            case 'x': if (is_long) print_uint(va_arg(ap, u64), 16, false); else print_uint(va_arg(ap, u32), 16, false); break;
            case 'X': if (is_long) print_uint(va_arg(ap, u64), 16, true); else print_uint(va_arg(ap, u32), 16, true); break;
            case 'p': print_uint((u64)va_arg(ap, void*), 16, false); break;
            case '%': putc_both('%'); break;
            case 0: i--; break;
            default: putc_both('%'); putc_both(fmt[i]); break;
        }
    }
    va_end(ap);
}

void log_ok(const char *msg) {
    log_write("[OK] ");
    log_write(msg);
    log_write("\n");
}

void log_fail(const char *msg) {
    log_write("[FAIL] ");
    log_write(msg);
    log_write("\n");
}
