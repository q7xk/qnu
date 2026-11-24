#ifndef QNU_BOOT_H
#define QNU_BOOT_H

#include <qnu/types.h>

struct qnu_boot_info {
    u64 memory_map_entries;
    u64 usable_memory;
    u64 total_memory;
    u64 hhdm_offset;
    u64 kernel_phys;
    u64 kernel_virt;
    u32 fb_width;
    u32 fb_height;
    u32 fb_pitch;
    u32 fb_bpp;
    void *fb_addr;
};

const struct qnu_boot_info *boot_info_get(void);

#endif
