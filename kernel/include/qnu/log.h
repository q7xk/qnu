#ifndef QNU_LOG_H
#define QNU_LOG_H

#include <qnu/types.h>

void log_init(void);
void log_putc(char c);
void log_write(const char *s);
void log_printf(const char *fmt, ...);
void log_ok(const char *msg);
void log_fail(const char *msg);

#endif
