#include <qnu/log.h>
#include <qnu/types.h>
typedef struct { const char *name; const char *desc; int (*init)(void); } kmod_t;
#define MAX_MODS 32
static kmod_t mods[MAX_MODS]; static int mod_count=0;
int module_register(const char *name,const char *desc,int (*init)(void)){
    if(mod_count>=MAX_MODS) return -1; mods[mod_count++] = (kmod_t){name,desc,init}; return 0;
}
void modules_init(void){ for(int i=0;i<mod_count;i++){ if(mods[i].init) mods[i].init(); log_printf("[mod] %s: %s\n", mods[i].name, mods[i].desc); } log_ok("Module registry initialized"); }
