#include <qnu/io.h>
#include <qnu/log.h>
#include <qnu/types.h>

static bool shift = false, caps = false, ext = false;
extern void shell_input_char(char c);

static const char map[128] = {0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0};
static const char map_shift[128] = {0,27,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,'A','S','D','F','G','H','J','K','L',':','"','~',0,'|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' ',0};

void keyboard_irq(void) {
    u8 sc = inb(0x60);
    if (sc == 0xE0) { ext = true; return; }
    if (sc == 0x2A || sc == 0x36) { shift = true; ext=false; return; }
    if (sc == 0xAA || sc == 0xB6) { shift = false; ext=false; return; }
    if (sc == 0x3A) { caps = !caps; ext=false; return; }
    if (sc & 0x80) { ext = false; return; }
    if (ext) {
        ext = false;
        /* extended keys: arrows, home/end etc. - ignore for now but don't corrupt state */
        return;
    }
    if (sc < 128) {
        char c = shift ? map_shift[sc] : map[sc];
        if (c >= 'a' && c <= 'z' && caps) c = c - 'a' + 'A';
        else if (c >= 'A' && c <= 'Z' && caps) c = c - 'A' + 'a';
        if (c) shell_input_char(c);
    }
}

void keyboard_init(void) { shift = false; caps = false; ext = false; }
