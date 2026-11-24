#ifndef QNU_BOOT_INTERNAL_H
#define QNU_BOOT_INTERNAL_H
#include <limine.h>
struct limine_memmap_response *boot_memmap(void);
void *boot_rsdp(void);
#endif
