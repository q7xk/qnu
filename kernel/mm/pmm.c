#include <qnu/mm.h>
#include <qnu/log.h>
#include <qnu/panic.h>
#include <qnu/boot.h>
#include <qnu/boot_internal.h>

#define PAGE_SIZE 4096
static u8 *bitmap;
static u64 bitmap_size;
static u64 total_pages;
static u64 free_pages_count;
static u64 hhdm;

static inline void bm_set(u64 i) { bitmap[i/8] |= (1u << (i%8)); }
static inline void bm_clear(u64 i) { bitmap[i/8] &= ~(1u << (i%8)); }
static inline bool bm_test(u64 i) { return bitmap[i/8] & (1u << (i%8)); }

void pmm_init(void) {
    hhdm = boot_info_get()->hhdm_offset;
    struct limine_memmap_response *mm = boot_memmap();
    if (!mm) panic("No memmap");

    u64 highest = 0;
    for (u64 i = 0; i < mm->entry_count; i++) {
        struct limine_memmap_entry *e = mm->entries[i];
        u64 top = e->base + e->length;
        if (top > highest) highest = top;
    }
    total_pages = highest / PAGE_SIZE;
    bitmap_size = (total_pages + 7) / 8;

    /* find a place for bitmap in usable memory */
    bitmap = NULL;
    for (u64 i = 0; i < mm->entry_count; i++) {
        struct limine_memmap_entry *e = mm->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE && e->length >= bitmap_size) {
            bitmap = (u8 *)(e->base + hhdm);
            break;
        }
    }
    if (!bitmap) panic("No space for PMM bitmap");

    /* mark all used */
    for (u64 i = 0; i < bitmap_size; i++) bitmap[i] = 0xFF;
    free_pages_count = 0;

    /* free usable regions */
    for (u64 i = 0; i < mm->entry_count; i++) {
        struct limine_memmap_entry *e = mm->entries[i];
        if (e->type != LIMINE_MEMMAP_USABLE) continue;
        u64 base = (e->base + PAGE_SIZE - 1) / PAGE_SIZE;
        u64 end = (e->base + e->length) / PAGE_SIZE;
        for (u64 p = base; p < end && p < total_pages; p++) {
            if (bm_test(p)) { bm_clear(p); free_pages_count++; }
        }
    }

    /* reserve bitmap pages */
    u64 bmp_phys = (u64)bitmap - hhdm;
    u64 bmp_start = bmp_phys / PAGE_SIZE;
    u64 bmp_pages = (bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (u64 p = bmp_start; p < bmp_start + bmp_pages && p < total_pages; p++) {
        if (!bm_test(p)) { bm_set(p); free_pages_count--; }
    }

    log_printf("[OK] PMM initialized (%lu free pages)\n", free_pages_count);
}

void *pmm_alloc_page(void) {
    for (u64 i = 0; i < total_pages; i++) {
        if (!bm_test(i)) {
            bm_set(i);
            if (free_pages_count) free_pages_count--;
            void *addr = (void *)(i * PAGE_SIZE + hhdm);
            u8 *b = (u8 *)addr;
            for (u64 j = 0; j < PAGE_SIZE; j++) b[j] = 0;
            return addr;
        }
    }
    return NULL;
}

void pmm_free_page(void *page) {
    u64 phys = (u64)page - hhdm;
    u64 i = phys / PAGE_SIZE;
    if (i < total_pages && bm_test(i)) { bm_clear(i); free_pages_count++; }
}

u64 pmm_free_pages(void) { return free_pages_count; }
