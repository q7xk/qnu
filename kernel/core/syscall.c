#include <qnu/syscall.h>
#include <qnu/log.h>
#include <qnu/vfs.h>
#include <qnu/mm.h>

#define MAX_FD 64
typedef struct { vfs_node_t *node; u64 off; bool used; } fd_t;
static fd_t fds[MAX_FD];

static inline void wrmsr(u32 msr,u64 val){ u32 lo=val&0xFFFFFFFF,hi=val>>32; __asm__ volatile("wrmsr"::"a"(lo),"d"(hi),"c"(msr)); }
extern void syscall_entry(void);

void syscall_init(void){
    for(int i=0;i<MAX_FD;i++){fds[i].used=false;fds[i].node=0;fds[i].off=0;}
    wrmsr(0xC0000081, ((u64)0x08<<32) | ((u64)0x10<<48)); /* STAR */
    wrmsr(0xC0000082, (u64)syscall_entry); /* LSTAR */
    wrmsr(0xC0000084, 0); /* SFMASK */
    /* enable SCE in EFER */
    u32 lo,hi; __asm__ volatile("rdmsr":"=a"(lo),"=d"(hi):"c"(0xC0000080)); u64 e=((u64)hi<<32)|lo; e|=1; wrmsr(0xC0000080,e);
    log_ok("Syscall interface initialized");
}

static int fd_alloc(vfs_node_t*n){ for(int i=3;i<MAX_FD;i++) if(!fds[i].used){fds[i].used=true;fds[i].node=n;fds[i].off=0;return i;} return -1; }

i64 syscall_dispatch(u64 n,u64 a1,u64 a2,u64 a3,u64 a4,u64 a5){
    (void)a4; (void)a5;
    switch(n){
        case 1: /* write fd,buf,len */ {
            u64 fd=a1; const char*buf=(const char*)a2; u64 len=a3;
            if(fd==1||fd==2){ for(u64 i=0;i<len;i++){ char c[2]={buf[i],0}; log_write(c);} return (i64)len; }
            if(fd<MAX_FD && fds[fd].used) return vfs_write(fds[fd].node,fds[fd].off,buf,len);
            return -1;
        }
        case 0: /* read */ {
            u64 fd=a1; char*buf=(char*)a2; u64 len=a3;
            if(fd<MAX_FD && fds[fd].used){ int r=vfs_read(fds[fd].node,fds[fd].off,buf,len); if(r>0) fds[fd].off+=r; return r; }
            return -1;
        }
        case 2: /* open path,flags */ {
            const char *path=(const char*)a1; vfs_node_t*n=vfs_lookup(path);
            if(!n && (a2&1)) n=vfs_create(path,VFS_FILE);
            if(!n) return -1; return fd_alloc(n);
        }
        case 3: /* close */ { u64 fd=a1; if(fd<MAX_FD) fds[fd].used=false; return 0; }
        case 39: return 1; /* getpid */
        case 60: /* exit */ { log_write("[proc] exit\n"); return 0; }
        default: return -1;
    }
}
