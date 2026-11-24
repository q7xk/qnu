#include <qnu/mm.h>
#include <qnu/log.h>
#include <qnu/panic.h>
#include <qnu/boot.h>

#define HEAP_START 0xFFFF900000000000ULL
#define PAGE_SIZE 4096
#define SLAB_MAGIC 0x51AB51ABu

typedef struct block { size_t size; bool free; struct block *next; } block_t;
static block_t *head;
static u64 heap_end;
static u64 hhdm_off;

typedef struct slab_hdr { u32 magic; u32 size; } slab_hdr_t;
typedef struct slab_obj { struct slab_obj *next; } slab_obj_t;
typedef struct slab { u32 obj_size; slab_obj_t *free; } slab_t;
static slab_t slabs[6];
static const u32 slab_sizes[6] = {32,64,128,256,512,1024};

static void heap_grow(size_t needed) {
    size_t pages = (needed + sizeof(block_t) + PAGE_SIZE - 1) / PAGE_SIZE;
    for (size_t i = 0; i < pages; i++) {
        void *p = pmm_alloc_page();
        if (!p) panic("Heap OOM");
        u64 phys = (u64)p - hhdm_off;
        vmm_map(heap_end, phys, 0x3);
        heap_end += PAGE_SIZE;
    }
}

static void *slab_alloc(slab_t *s) {
    u32 real = s->obj_size + sizeof(slab_hdr_t);
    if (!s->free) {
        void *p = pmm_alloc_page(); if (!p) return NULL;
        u64 phys = (u64)p - hhdm_off; u64 va = heap_end; heap_end += PAGE_SIZE;
        vmm_map(va, phys, 0x3);
        u8 *base = (u8*)va;
        for (u32 off = 0; off + real <= PAGE_SIZE; off += real) {
            slab_obj_t *o = (slab_obj_t*)(base + off); o->next = s->free; s->free = o;
        }
    }
    slab_obj_t *o = s->free; s->free = o->next;
    slab_hdr_t *h = (slab_hdr_t*)o; h->magic = SLAB_MAGIC; h->size = s->obj_size;
    u8 *b=(u8*)(h+1); for(u32 i=0;i<s->obj_size;i++) b[i]=0;
    return b;
}

void heap_init(void) {
    hhdm_off = boot_info_get()->hhdm_offset;
    heap_end = HEAP_START;
    heap_grow(1024 * 1024);
    head = (block_t *)HEAP_START;
    head->size = (heap_end - HEAP_START) - sizeof(block_t);
    head->free = true; head->next = NULL;
    for (int i=0;i<6;i++) { slabs[i].obj_size = slab_sizes[i]; slabs[i].free = NULL; }
    log_ok("Heap initialized (slab+free-list)");
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    for (int i=0;i<6;i++) if (size <= slab_sizes[i]) return slab_alloc(&slabs[i]);
    size = (size + 15) & ~((size_t)15);
    block_t *cur = head;
    while (cur) {
        if (cur->free && cur->size >= size) {
            if (cur->size >= size + sizeof(block_t) + 16) {
                block_t *n = (block_t *)((u8 *)cur + sizeof(block_t) + size);
                n->size = cur->size - size - sizeof(block_t);
                n->free = true; n->next = cur->next; cur->next = n; cur->size = size;
            }
            cur->free = false; return (u8 *)cur + sizeof(block_t);
        }
        if (!cur->next) {
            u64 old_end = heap_end; heap_grow(size + sizeof(block_t));
            if (cur->free) cur->size += (heap_end - old_end);
            else { block_t *n=(block_t*)old_end; n->size=(heap_end-old_end)-sizeof(block_t); n->free=true; n->next=NULL; cur->next=n; }
            continue;
        }
        cur = cur->next;
    }
    return NULL;
}

void kfree(void *ptr) {
    if (!ptr) return;
    slab_hdr_t *sh = (slab_hdr_t *)((u8*)ptr - sizeof(slab_hdr_t));
    if (sh->magic == SLAB_MAGIC) {
        for (int i=0;i<6;i++) if (slabs[i].obj_size == sh->size) { slab_obj_t *o=(slab_obj_t*)sh; o->next=slabs[i].free; slabs[i].free=o; return; }
    }
    block_t *b = (block_t *)((u8 *)ptr - sizeof(block_t));
    b->free = true;
    block_t *cur = head;
    while (cur && cur->next) {
        if (cur->free && cur->next->free) { cur->size += sizeof(block_t)+cur->next->size; cur->next=cur->next->next; }
        else cur = cur->next;
    }
}

void heap_stats(u64 *total, u64 *used) {
    u64 t=0,u=0; for(block_t*c=head;c;c=c->next){t+=c->size; if(!c->free)u+=c->size;} if(total)*total=t; if(used)*used=u;
}
