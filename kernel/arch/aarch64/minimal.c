#include <qnu/log.h>
#include <qnu/boot.h>
#include <qnu/types.h>
static char kbuf[4096]; static int kpos=0;
void log_init(void){ kpos=0; }
void log_write(const char*s){ while(*s){ if(kpos<4095){kbuf[kpos++]=*s; kbuf[kpos]=0;} s++; } }
void log_printf(const char*fmt,...){ (void)fmt; }
void log_ok(const char*msg){ log_write("[OK] "); log_write(msg); log_write("\n"); }
void boot_init(void){}
const struct qnu_boot_info *boot_info_get(void){ static struct qnu_boot_info i; return &i; }
void *boot_rsdp(void){ return 0; }
struct limine_memmap_response *boot_memmap(void){ return 0; }
void kmain(void){
    log_init();
    log_write("QNU aarch64 scaffold boot\n");
    for(;;) __asm__ volatile("wfe");
}
