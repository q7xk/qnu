#include <qnu/syscall.h>
#include <qnu/log.h>
#include <qnu/vfs.h>
#include <qnu/mm.h>
#include <qnu/proc.h>
#include <qnu/boot.h>

#define MAX_FD 64
typedef struct { vfs_node_t *node; u64 off; bool used; } fd_t;
static fd_t fds[MAX_FD];
extern volatile u64 proc_exit_requested;
extern void proc_exit(int code);

static inline void wrmsr(u32 msr,u64 val){ u32 lo=val&0xFFFFFFFF,hi=val>>32; __asm__ volatile("wrmsr"::"a"(lo),"d"(hi),"c"(msr)); }
extern void syscall_entry(void);
extern u64 syscall_kernel_rsp;
void syscall_set_kernel_rsp(u64 rsp){ syscall_kernel_rsp = rsp; }

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

static char cwd_buf[128] = "/";

i64 syscall_dispatch(u64 n,u64 a1,u64 a2,u64 a3,u64 a4,u64 a5){
    (void)a4; (void)a5;
    proc_t *cur = proc_current();
    switch(n){
        case 0: { /* read */
            u64 fd=a1; char*buf=(char*)a2; u64 len=a3;
            if(fd<MAX_FD && fds[fd].used){ int r=vfs_read(fds[fd].node,fds[fd].off,buf,len); if(r>0) fds[fd].off+=r; return r; }
            return 0;
        }
        case 1: { /* write */
            u64 fd=a1; const char*buf=(const char*)a2; u64 len=a3;
            if(fd==1||fd==2){ for(u64 i=0;i<len;i++){ char c[2]={buf[i],0}; log_write(c);} return (i64)len; }
            if(fd<MAX_FD && fds[fd].used) return vfs_write(fds[fd].node,fds[fd].off,buf,len);
            return -1;
        }
        case 2: { /* open */
            const char *path=(const char*)a1; vfs_node_t*n=vfs_lookup(path);
            if(!n && (a2&64)) n=vfs_create(path,VFS_FILE); /* O_CREAT */
            if(!n) return -1; return fd_alloc(n);
        }
        case 3: { if(a1<MAX_FD) fds[a1].used=false; return 0; }
        case 5: { /* fstat - minimal */ return 0; }
        case 12: { /* brk */
            if(!cur) return -1;
            if(a1==0) return (i64)cur->brk;
            u64 newbrk=a1;
            if(newbrk < cur->brk_base) return (i64)cur->brk;
            u64 hhdm = boot_info_get()->hhdm_offset;
            while(cur->brk < newbrk){ void*pg=pmm_alloc_page(); if(!pg) break; vmm_map(cur->brk & ~0xFFFULL, (u64)pg-hhdm, 0x7); cur->brk = (cur->brk + 0x1000) & ~0xFFFULL; }
            if(cur->brk < newbrk) cur->brk=newbrk;
            return (i64)cur->brk;
        }
        case 39: return cur?cur->pid:1; /* getpid */
        case 60: /* exit */
            proc_exit((int)a1);
            proc_exit_requested = 1;
            return 0;
        case 63: { /* uname */
            struct { char sysname[65],nodename[65],release[65],version[65],machine[65]; } *u = (void*)a1;
            const char *sn="QNU", *nn="qnu", *rl="0.0.7", *ver="QNU 0.0.7", *ma="x86_64";
            for(int i=0;i<65;i++){ u->sysname[i]=0;u->nodename[i]=0;u->release[i]=0;u->version[i]=0;u->machine[i]=0; }
            for(int i=0;sn[i]&&i<64;i++)u->sysname[i]=sn[i];
            for(int i=0;nn[i]&&i<64;i++)u->nodename[i]=nn[i];
            for(int i=0;rl[i]&&i<64;i++)u->release[i]=rl[i];
            for(int i=0;ver[i]&&i<64;i++)u->version[i]=ver[i];
            for(int i=0;ma[i]&&i<64;i++)u->machine[i]=ma[i];
            return 0;
        }
        case 79: { /* getcwd */
            char *buf=(char*)a1; u64 len=a2; int i=0; while(cwd_buf[i] && (u64)i+1<len){ buf[i]=cwd_buf[i]; i++; } buf[i]=0; return (i64)buf;
        }
        case 80: { /* chdir */ const char*p=(const char*)a1; vfs_node_t*n=vfs_lookup(p); if(!n||n->type!=VFS_DIR) return -1; int i=0; for(;p[i]&&i<127;i++)cwd_buf[i]=p[i]; cwd_buf[i]=0; return 0; }
        case 158: return 0; /* arch_prctl stub */
        case 231: proc_exit((int)a1); proc_exit_requested=1; return 0; /* exit_group */ 
        default:
            log_printf("[syscall] unknown %lu\n", n);
            return -1;
    }
}
