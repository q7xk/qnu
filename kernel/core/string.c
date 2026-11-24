#include <qnu/types.h>

void *memset(void *dst, int c, size_t n) {
    u8 *d = (u8 *)dst;
    for (size_t i = 0; i < n; i++) d[i] = (u8)c;
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
    u8 *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
    u8 *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;
    if (d < s) for (size_t i = 0; i < n; i++) d[i] = s[i];
    else if (d > s) for (size_t i = n; i > 0; i--) d[i-1] = s[i-1];
    return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
    const u8 *x = (const u8 *)a, *y = (const u8 *)b;
    for (size_t i = 0; i < n; i++) if (x[i] != y[i]) return (int)x[i] - (int)y[i];
    return 0;
}

size_t strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}
