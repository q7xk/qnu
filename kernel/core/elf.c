#include <qnu/elf.h>
#include <qnu/mm.h>
#include <qnu/log.h>
#include <qnu/boot.h>

typedef struct { u8 e_ident[16]; u16 e_type,e_machine; u32 e_version; u64 e_entry,e_phoff,e_shoff; u32 e_flags; u16 e_ehsize,e_phentsize,e_phnum,e_shentsize,e_shnum,e_shstrndx; } __attribute__((packed)) Elf64_Ehdr;
typedef struct { u32 p_type,p_flags; u64 p_offset,p_vaddr,p_paddr,p_filesz,p_memsz,p_align; } __attribute__((packed)) Elf64_Phdr;
#define PT_LOAD 1
#define PF_W 2

u64 elf_load(const u8 *data, u64 size, u64 *entry_out){
    if(size < sizeof(Elf64_Ehdr)) return 0;
    const Elf64_Ehdr *eh=(const Elf64_Ehdr*)data;
    if(!(eh->e_ident[0]==0x7f && eh->e_ident[1]=='E' && eh->e_ident[2]=='L' && eh->e_ident[3]=='F')) return 0;
    if(eh->e_machine != 62) return 0;
    u64 hhdm = boot_info_get()->hhdm_offset;
    for(u16 i=0;i<eh->e_phnum;i++){
        const Elf64_Phdr *ph=(const Elf64_Phdr*)(data + eh->e_phoff + i*eh->e_phentsize);
        if(ph->p_type!=PT_LOAD) continue;
        u64 vstart = ph->p_vaddr & ~0xFFFULL;
        u64 vend = (ph->p_vaddr + ph->p_memsz + 0xFFF) & ~0xFFFULL;
        for(u64 va=vstart; va<vend; va+=0x1000){
            void *pg = pmm_alloc_page(); if(!pg) return 0;
            u64 phys = (u64)pg - hhdm;
            u64 flags = 0x7; /* present+write+user */
            vmm_map(va, phys, flags);
        }
        /* copy */
        for(u64 j=0;j<ph->p_filesz;j++){
            u8 *dst = (u8*)(ph->p_vaddr + j);
            dst[0] = data[ph->p_offset + j];
        }
        for(u64 j=ph->p_filesz;j<ph->p_memsz;j++){
            u8 *dst = (u8*)(ph->p_vaddr + j);
            dst[0]=0;
        }
    }
    if(entry_out) *entry_out = eh->e_entry;
    return 1;
}
