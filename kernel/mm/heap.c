#include <qnu/mm.h>
#include <qnu/log.h>
#include <qnu/panic.h>
#include <qnu/boot.h>

#define HEAP_START 0xFFFF900000000000ULL
#define HEAP_PAGES 1024  /* 4MB initial */
#define PAGE_SIZE 4096

typedef struct block {
    size_t size;
    bool free;
    struct block *next;
} block_t;

static block_t *head;
static u64 heap_end;
static u64 hhdm_off;

static void heap_grow(size_t needed) {
    size_t pages = (needed + sizeof(block_t) + PAGE_SIZE - 1) / PAGE_SIZE;
    for (size_t i = 0; i < pages; i++) {
        void *p = pmm_alloc_page();
        if (!p) panic("Heap OOM");
        u64 phys = (u64)p - hhdm_off;
        vmm_map(heap_end, phys, 0x3); /* present+write */
        heap_end += PAGE_SIZE;
    }
}

void heap_init(void) {
    hhdm_off = boot_info_get()->hhdm_offset;
    heap_end = HEAP_START;
    heap_grow(HEAP_PAGES * PAGE_SIZE / 16);
    head = (block_t *)HEAP_START;
    head->size = (heap_end - HEAP_START) - sizeof(block_t);
    head->free = true;
    head->next = NULL;
    log_ok("Heap initialized");
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    size = (size + 15) & ~((size_t)15);
    block_t *cur = head;
    while (cur) {
        if (cur->free && cur->size >= size) {
            if (cur->size >= size + sizeof(block_t) + 16) {
                block_t *n = (block_t *)((u8 *)cur + sizeof(block_t) + size);
                n->size = cur->size - size - sizeof(block_t);
                n->free = true;
                n->next = cur->next;
                cur->next = n;
                cur->size = size;
            }
            cur->free = false;
            return (u8 *)cur + sizeof(block_t);
        }
        if (!cur->next) {
            u64 old_end = heap_end;
            heap_grow(size + sizeof(block_t));
            if (cur->free) {
                cur->size += (heap_end - old_end);
            } else {
                block_t *n = (block_t *)old_end;
                n->size = (heap_end - old_end) - sizeof(block_t);
                n->free = true;
                n->next = NULL;
                cur->next = n;
            }
            continue;
        }
        cur = cur->next;
    }
    return NULL;
}

void kfree(void *ptr) {
    if (!ptr) return;
    block_t *b = (block_t *)((u8 *)ptr - sizeof(block_t));
    b->free = true;
    /* coalesce */
    block_t *cur = head;
    while (cur && cur->next) {
        if (cur->free && cur->next->free) {
            cur->size += sizeof(block_t) + cur->next->size;
            cur->next = cur->next->next;
        } else cur = cur->next;
    }
}
