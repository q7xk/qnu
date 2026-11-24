#ifndef QNU_IO_H
#define QNU_IO_H
#include <qnu/types.h>
static inline void outb(u16 port, u8 val) { __asm__ volatile("outb %0,%1"::"a"(val),"Nd"(port)); }
static inline u8 inb(u16 port) { u8 r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(port)); return r; }
static inline void io_wait(void) { outb(0x80, 0); }
#endif
