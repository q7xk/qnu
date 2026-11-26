#include <qnu/log.h>
#include <qnu/types.h>
static volatile u32 online_cpus = 1;
void smp_init(void){
    u32 n = 1; online_cpus=1;
    log_printf("[SMP] detected %u CPUs, online %u (AP bring-up scaffolding)\n", n, online_cpus);
}
u32 smp_online_cpus(void){ return online_cpus; }
