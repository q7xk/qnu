#ifndef QNU_MM_H
#define QNU_MM_H
#include <qnu/types.h>
void pmm_init(void);
void *pmm_alloc_page(void);
void pmm_free_page(void *page);
u64 pmm_free_pages(void);

void vmm_init(void);
void vmm_map(u64 virt, u64 phys, u64 flags);
void vmm_map_to(u64 *pml4_root, u64 virt, u64 phys, u64 flags);
u64 vmm_new_address_space(void);
void vmm_switch(u64 pml4_phys);
u64 vmm_get_pml4(void);

void heap_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);
void heap_stats(u64 *total, u64 *used);
#endif
