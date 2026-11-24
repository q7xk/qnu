#include <qnu/mm.h>
#include <qnu/log.h>
#include <qnu/panic.h>
#include <qnu/boot.h>

#define PAGE_SIZE 4096
#define PTE_PRESENT (1ULL<<0)
#define PTE_WRITE   (1ULL<<1)
#define PTE_USER    (1ULL<<2)

static u64 *pml4;
static u64 hhdm;

static inline u64 read_cr3(void) { u64 v; __asm__ volatile("mov %%cr3,%0":"=r"(v)); return v; }
static inline void write_cr3(u64 v) { __asm__ volatile("mov %0,%%cr3"::"r"(v):"memory"); }

static u64 *get_next(u64 *table, u64 idx, bool alloc) {
    if (table[idx] & PTE_PRESENT) {
        return (u64 *)((table[idx] & 0x000FFFFFFFFFF000ULL) + hhdm);
    }
    if (!alloc) return NULL;
    void *p = pmm_alloc_page();
    if (!p) return NULL;
    table[idx] = ((u64)p - hhdm) | PTE_PRESENT | PTE_WRITE;
    return (u64 *)p;
}

void vmm_map(u64 virt, u64 phys, u64 flags) {
    u64 pml4_i = (virt >> 39) & 0x1FF;
    u64 pdpt_i = (virt >> 30) & 0x1FF;
    u64 pd_i   = (virt >> 21) & 0x1FF;
    u64 pt_i   = (virt >> 12) & 0x1FF;

    u64 *pdpt = get_next(pml4, pml4_i, true);
    u64 *pd   = get_next(pdpt, pdpt_i, true);
    u64 *pt   = get_next(pd, pd_i, true);
    if (!pdpt || !pd || !pt) panic("VMM map: out of memory");
    pt[pt_i] = (phys & 0x000FFFFFFFFFF000ULL) | flags | PTE_PRESENT;
    __asm__ volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

u64 vmm_get_pml4(void) { return (u64)pml4 - hhdm; }

void vmm_init(void) {
    hhdm = boot_info_get()->hhdm_offset;
    u64 cr3 = read_cr3();
    pml4 = (u64 *)(cr3 + hhdm);
    log_ok("VMM initialized");
}
