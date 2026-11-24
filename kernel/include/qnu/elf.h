#ifndef QNU_ELF_H
#define QNU_ELF_H
#include <qnu/types.h>
u64 elf_load(const u8 *data, u64 size, u64 *entry_out);
#endif
