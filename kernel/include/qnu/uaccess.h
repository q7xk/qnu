#ifndef QNU_UACCESS_H
#define QNU_UACCESS_H
#include <qnu/types.h>
static inline bool user_range_ok(const void *p, u64 len){
    u64 a=(u64)p; u64 end=a+len;
    if(end < a) return false;
    return a < 0x0000800000000000ULL && end <= 0x0000800000000000ULL;
}
static inline int copy_from_user(void *dst,const void *src,u64 len){
    if(!user_range_ok(src,len)) return -1;
    const u8*s=(const u8*)src; u8*d=(u8*)dst; for(u64 i=0;i<len;i++) d[i]=s[i]; return 0;
}
static inline int copy_to_user(void *dst,const void *src,u64 len){
    if(!user_range_ok(dst,len)) return -1;
    const u8*s=(const u8*)src; u8*d=(u8*)dst; for(u64 i=0;i<len;i++) d[i]=s[i]; return 0;
}
#endif
