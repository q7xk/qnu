#ifndef QNU_CPU_H
#define QNU_CPU_H
#include <qnu/types.h>
void cpu_info_init(void);
void cpu_info_dump(void);
bool cpu_has_apic(void);
#endif
