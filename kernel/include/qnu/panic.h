#ifndef QNU_PANIC_H
#define QNU_PANIC_H

#include <qnu/types.h>

__attribute__((noreturn)) void panic(const char *msg);
void qnu_assert(bool cond, const char *expr, const char *file, int line);

#define QNU_ASSERT(x) qnu_assert((x), #x, __FILE__, __LINE__)

#endif
