#ifndef QNU_CONSOLE_H
#define QNU_CONSOLE_H

#include <qnu/types.h>

void console_init(void *fb_addr, u32 width, u32 height, u32 pitch, u32 bpp);
void console_putc(char c);
void console_write(const char *s);
void console_clear(void);

#endif
