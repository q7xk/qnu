#ifndef QNU_SYSCALL_H
#define QNU_SYSCALL_H
#include <qnu/types.h>
void syscall_init(void);
i64 syscall_dispatch(u64 n,u64 a1,u64 a2,u64 a3,u64 a4,u64 a5);
#endif
