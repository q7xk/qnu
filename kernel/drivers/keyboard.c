#include <qnu/io.h>
#include <qnu/log.h>
#include <qnu/types.h>

static const char scancode_map[128] = {
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0,
};

void keyboard_irq(void) {
    u8 sc = inb(0x60);
    if (sc & 0x80) return;
    if (sc < 128) {
        char c = scancode_map[sc];
        if (c) log_printf("%c", c);
    }
}

void keyboard_init(void) {
    /* IRQ will be unmasked by interrupt init */
}
