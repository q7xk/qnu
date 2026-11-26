#include <qnu/syscall.h>
#include <qnu/log.h>
#include <qnu/vfs.h>
#include <qnu/mm.h>
#include <qnu/proc.h>
#include <qnu/boot.h>
#include <qnu/sched.h>
#include <qnu/uaccess.h>

#define MAX_FD 64
#define FD_FILE 1
#define FD_PIPE_R 2
#define FD_PIPE_W 3
#define PIPE_BUF 1024
typedef struct { u8 buf[PIPE_BUF]; u32 r,w,count; int readers,writers; } pipe_t;
typedef struct { int type; vfs_node_t *node; u64 off; bool used; pipe_t *pipe; } fd_t;
static fd_t fds[MAX_FD];
extern volatile u64 proc_exit_requested;
extern void proc_exit(int code);

static inline void wrmsr(u32 msr,u64 val){ u32 lo=val&0xFFFFFFFF,hi=val>>32; __asm__ volatile("wrmsr"::"a"(lo),"d"(hi),"c"(msr)); }
extern void syscall_entry(void);
extern u64 syscall_kernel_rsp;
void syscall_set_kernel_rsp(u64 rsp){ syscall_kernel_rsp = rsp; }

void syscall_init(void){
    for(int i=0;i<MAX_FD;i++){fds[i].used=false;fds[i].node=0;fds[i].off=0;fds[i].type=0;fds[i].pipe=0;}
    wrmsr(0xC0000081, ((u64)0x08<<32) | ((u64)0x10<<48));
    wrmsr(0xC0000082, (u64)syscall_entry);
    wrmsr(0xC0000084, 0);
    u32 lo,hi; __asm__ volatile("rdmsr":"=a"(lo),"=d"(hi):"c"(0xC0000080)); u64 e=((u64)hi<<32)|lo; e|=1; wrmsr(0xC0000080,e);
    log_ok("Syscall interface initialized");
}

static int fd_alloc_file(vfs_node_t*n){ for(int i=3;i<MAX_FD;i++) if(!fds[i].used){fds[i].used=true;fds[i].type=FD_FILE;fds[i].node=n;fds[i].off=0;fds[i].pipe=0;return i;} return -1; }
static int fd_alloc_pipe(pipe_t*p,int type){ for(int i=3;i<MAX_FD;i++) if(!fds[i].used){fds[i].used=true;fds[i].type=type;fds[i].node=0;fds[i].off=0;fds[i].pipe=p;return i;} return -1; }

static char cwd_buf[128] = "/";

static int is_dev(vfs_node_t*n,const char*name){ if(!n) return 0; int i=0; while(name[i]&&n->name[i]){ if(name[i]!=n->name[i]) return 0; i++; } return name[i]==0 && n->name[i]==0; }

i64 syscall_dispatch(u64 n,u64 a1,u64 a2,u64 a3,u64 a4,u64 a5){
    (void)a4; (void)a5;
    proc_t *cur = proc_current();
    switch(n){
        case 0: { /* read */
            u64 fd=a1; char*buf=(char*)a2; u64 len=a3; if(len>0 && !user_range_ok(buf,len)) return -14;
            if(fd>=MAX_FD || !fds[fd].used) return -1;
            if(fds[fd].type==FD_FILE){
                vfs_node_t*node=fds[fd].node;
                if(node && (node->flags&VFS_DEV) && is_dev(node,"null")) return 0;
                if(node && (node->flags&VFS_DEV) && is_dev(node,"zero")){ for(u64 i=0;i<len;i++) buf[i]=0; return (i64)len; }
                if(node && (node->flags&VFS_DEV) && is_dev(node,"console")) return 0;
                int r=vfs_read(node,fds[fd].off,buf,len); if(r>0) fds[fd].off+=r; return r;
            }
            if(fds[fd].type==FD_PIPE_R){
                pipe_t*p=fds[fd].pipe; u64 i=0; while(i<len && p->count>0){ buf[i++]=p->buf[p->r]; p->r=(p->r+1)%PIPE_BUF; p->count--; } return (i64)i;
            }
            return -1;
        }
        case 1: { /* write */
            u64 fd=a1; const char*buf=(const char*)a2; u64 len=a3; if(len>0 && !user_range_ok(buf,len)) return -14;
            if(fd==1||fd==2){ for(u64 i=0;i<len;i++){ char c[2]={buf[i],0}; log_write(c);} return (i64)len; }
            if(fd>=MAX_FD || !fds[fd].used) return -1;
            if(fds[fd].type==FD_FILE){
                vfs_node_t*node=fds[fd].node;
                if(node && (node->flags&VFS_DEV) && (is_dev(node,"null")||is_dev(node,"zero"))) return (i64)len;
                if(node && (node->flags&VFS_DEV) && is_dev(node,"console")){ for(u64 i=0;i<len;i++){ char c[2]={buf[i],0}; log_write(c);} return (i64)len; }
                return vfs_write(node,fds[fd].off,buf,len);
            }
            if(fds[fd].type==FD_PIPE_W){
                pipe_t*p=fds[fd].pipe; u64 i=0; while(i<len && p->count<PIPE_BUF){ p->buf[p->w]=buf[i++]; p->w=(p->w+1)%PIPE_BUF; p->count++; } return (i64)i;
            }
            return -1;
        }
        case 2: { /* open */
            const char *path=(const char*)a1; vfs_node_t*n=vfs_lookup(path);
            if(!n && (a2&64)) n=vfs_create(path,VFS_FILE);
            if(!n) return -1; return fd_alloc_file(n);
        }
        case 3: { if(a1<MAX_FD && fds[a1].used){ fds[a1].used=false; } return 0; }
        case 5: return 0; /* fstat stub */
        case 16: return 0; /* ioctl stub */
        case 22: { /* pipe */
            int *pfd=(int*)a1; if(!user_range_ok(pfd,sizeof(int)*2)) return -14; pipe_t*p=(pipe_t*)kmalloc(sizeof(pipe_t)); if(!p) return -1;
            p->r=p->w=p->count=0; p->readers=1; p->writers=1;
            int r=fd_alloc_pipe(p,FD_PIPE_R), w=fd_alloc_pipe(p,FD_PIPE_W);
            if(r<0||w<0) return -1; pfd[0]=r; pfd[1]=w; return 0;
        }
        case 32: { /* dup */
            int old=(int)a1; if(old<0||old>=MAX_FD||!fds[old].used) return -1;
            for(int i=0;i<MAX_FD;i++) if(!fds[i].used){ fds[i]=fds[old]; return i; } return -1;
        }
        case 33: { /* dup2 */
            int old=(int)a1, neu=(int)a2; if(old<0||old>=MAX_FD||!fds[old].used||neu<0||neu>=MAX_FD) return -1;
            if(old==neu) return neu; fds[neu]=fds[old]; fds[neu].used=true; return neu;
        }
        case 12: { /* brk */
            if(!cur) return -1;
            if(a1==0) return (i64)cur->brk;
            u64 newbrk=a1; if(newbrk < cur->brk_base) return (i64)cur->brk;
            u64 hhdm = boot_info_get()->hhdm_offset;
            while(cur->brk < newbrk){ void*pg=pmm_alloc_page(); if(!pg) break; vmm_map_to((u64*)(cur->cr3+hhdm), cur->brk & ~0xFFFULL, (u64)pg-hhdm, 0x7); cur->brk = (cur->brk + 0x1000) & ~0xFFFULL; }
            if(cur->brk < newbrk) cur->brk=newbrk;
            return (i64)cur->brk;
        }
        case 39: return cur?cur->pid:1;
        case 60: proc_exit((int)a1); proc_exit_requested=1; return 0;
        case 57: return proc_fork();
        case 59: { /* execve - minimal: path only */ const char*path=(const char*)a1; return proc_execve(path,0,0); }
        case 61: { int *st=(int*)a2; if(st && !user_range_ok(st,sizeof(int))) return -14; int kst=0; i64 r=proc_wait4((int)a1,&kst,(int)a3); if(r>0 && st) *st=kst; return r; }
        case 62: return proc_kill((int)a1,(int)a2); /* kill */
        case 63: { /* uname */
            if(!user_range_ok((void*)a1,sizeof(struct { char s[65],n[65],r[65],v[65],m[65]; }))) return -14;
            struct { char sysname[65],nodename[65],release[65],version[65],machine[65]; } *u = (void*)a1;
            const char *sn="QNU", *nn="qnu", *rl="0.0.9", *ver="QNU 0.0.9", *ma="x86_64";
            for(int i=0;i<65;i++){ u->sysname[i]=0;u->nodename[i]=0;u->release[i]=0;u->version[i]=0;u->machine[i]=0; }
            for(int i=0;sn[i]&&i<64;i++)u->sysname[i]=sn[i];
            for(int i=0;nn[i]&&i<64;i++)u->nodename[i]=nn[i];
            for(int i=0;rl[i]&&i<64;i++)u->release[i]=rl[i];
            for(int i=0;ver[i]&&i<64;i++)u->version[i]=ver[i];
            for(int i=0;ma[i]&&i<64;i++)u->machine[i]=ma[i];
            return 0;
        }
        case 79: { char *buf=(char*)a1; u64 len=a2; if(len>0 && !user_range_ok(buf,len)) return -14; int i=0; while(cwd_buf[i] && (u64)i+1<len){ buf[i]=cwd_buf[i]; i++; } buf[i]=0; return (i64)buf; }
        case 80: { const char*p=(const char*)a1; vfs_node_t*n=vfs_lookup(p); if(!n||n->type!=VFS_DIR) return -1; int i=0; for(;p[i]&&i<127;i++)cwd_buf[i]=p[i]; cwd_buf[i]=0; return 0; }
        case 9: return -38; /* mmap stub */
        case 10: return 0; /* mprotect stub */
        case 11: return 0; /* munmap stub */
        case 102: return cur?cur->uid:0;
        case 104: return cur?cur->gid:0;
        case 105: if(cur) cur->uid=(u32)a1; return 0;
        case 107: return cur?cur->uid:0;
        case 108: return cur?cur->gid:0;
        case 110: return cur?cur->ppid:0; /* getppid */
        case 158: return 0; /* arch_prctl stub */
        case 217: { /* getdents64 - minimal */
            int fd=(int)a1; if(a3>0 && !user_range_ok((void*)a2,a3)) return -14;
            struct { u64 d_ino; i64 d_off; unsigned short d_reclen; unsigned char d_type; char d_name[]; } *dent;
            if(fd<0||fd>=MAX_FD||!fds[fd].used||fds[fd].type!=FD_FILE) return -1;
            vfs_node_t*dir=fds[fd].node; if(!dir||dir->type!=VFS_DIR) return -1;
            char name[64]; u32 type=0; u32 idx=(u32)fds[fd].off;
            int r=vfs_readdir(dir,idx,name,64,&type); if(r<=0) return 0;
            int namelen=0; while(name[namelen]) namelen++;
            int reclen = 8+8+2+1+namelen+1; reclen=(reclen+7)&~7;
            if((u64)reclen > a3) return -1;
            dent=(void*)a2; dent->d_ino=idx+1; dent->d_off=idx+1; dent->d_reclen=(unsigned short)reclen; dent->d_type=(unsigned char)(type==VFS_DIR?4:8);
            for(int i=0;i<=namelen;i++) dent->d_name[i]=name[i];
            fds[fd].off++; return reclen;
        }
        case 228: return 0; /* clock_gettime stub */
        case 231: proc_exit((int)a1); proc_exit_requested=1; return 0;
        default:
            return -38; /* -ENOSYS */
    }
}
