#ifndef QNU_USER_SYSCALL_H
#define QNU_USER_SYSCALL_H
typedef unsigned long u64; typedef long i64;
static inline i64 sys0(i64 n){ i64 r; __asm__ volatile("syscall":"=a"(r):"a"(n):"rcx","r11","memory"); return r; }
static inline i64 sys1(i64 n,i64 a1){ i64 r; __asm__ volatile("syscall":"=a"(r):"a"(n),"D"(a1):"rcx","r11","memory"); return r; }
static inline i64 sys2(i64 n,i64 a1,i64 a2){ i64 r; __asm__ volatile("syscall":"=a"(r):"a"(n),"D"(a1),"S"(a2):"rcx","r11","memory"); return r; }
static inline i64 sys3(i64 n,i64 a1,i64 a2,i64 a3){ i64 r; __asm__ volatile("syscall":"=a"(r):"a"(n),"D"(a1),"S"(a2),"d"(a3):"rcx","r11","memory"); return r; }
static inline i64 write(int fd,const void*buf,u64 len){ return sys3(1,fd,(i64)buf,(i64)len); }
static inline i64 read(int fd,void*buf,u64 len){ return sys3(0,fd,(i64)buf,(i64)len); }
static inline i64 open(const char*p,int flags){ return sys2(2,(i64)p,flags); }
static inline i64 close(int fd){ return sys1(3,fd); }
static inline void exit(int c){ sys1(60,c); for(;;){} }
static inline i64 getpid(void){ return sys0(39); }
static inline i64 uname(void*buf){ return sys1(63,(i64)buf); }
#endif
