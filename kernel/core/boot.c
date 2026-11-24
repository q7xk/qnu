#include <limine.h>
#include <qnu/boot.h>
#include <qnu/log.h>

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_kernel_address_request kernel_addr_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

static struct qnu_boot_info boot_info;

void boot_init(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        for (;;) __asm__ volatile ("hlt");
    }
    boot_info.hhdm_offset = hhdm_request.response ? hhdm_request.response->offset : 0;
    if (kernel_addr_request.response) {
        boot_info.kernel_phys = kernel_addr_request.response->physical_base;
        boot_info.kernel_virt = kernel_addr_request.response->virtual_base;
    }
    if (framebuffer_request.response && framebuffer_request.response->framebuffer_count > 0) {
        struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
        boot_info.fb_addr = fb->address;
        boot_info.fb_width = (u32)fb->width;
        boot_info.fb_height = (u32)fb->height;
        boot_info.fb_pitch = (u32)fb->pitch;
        boot_info.fb_bpp = (u32)fb->bpp;
    }
    boot_info.memory_map_entries = memmap_request.response ? memmap_request.response->entry_count : 0;
    boot_info.usable_memory = 0;
    boot_info.total_memory = 0;
    if (memmap_request.response) {
        for (u64 i = 0; i < memmap_request.response->entry_count; i++) {
            struct limine_memmap_entry *e = memmap_request.response->entries[i];
            boot_info.total_memory += e->length;
            if (e->type == LIMINE_MEMMAP_USABLE) boot_info.usable_memory += e->length;
        }
    }
}

const struct qnu_boot_info *boot_info_get(void) { return &boot_info; }
struct limine_memmap_response *boot_memmap(void) { return memmap_request.response; }
