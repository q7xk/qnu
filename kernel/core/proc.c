#include <qnu/proc.h>
#include <qnu/elf.h>
#include <qnu/mm.h>
#include <qnu/log.h>
#include <qnu/sched.h>
#include <qnu/boot.h>

static proc_t procs[MAX_PROC];
static proc_t *current = NULL;
static int next_pid = 1;
extern void syscall_set_kernel_rsp(u64 rsp);
extern void enter_user(u64 rip, u64 rsp);
extern void tss_set_rsp0(u64 rsp0);

void proc_init(void) {
    for (int i=0;i<MAX_PROC;i++){ procs[i].state=PROC_UNUSED; procs[i].pid=0; }
    current = NULL;
    log_ok("Process subsystem initialized");
}

proc_t *proc_current(void){ return current; }
void proc_set_current(proc_t *p){ current=p; }

proc_t *proc_alloc(const char *name){
    for(int i=0;i<MAX_PROC;i++){
        if(procs[i].state==PROC_UNUSED){
            proc_t *p=&procs[i];
            for(int j=0;j<(int)sizeof(*p);j++) ((u8*)p)[j]=0;
            p->pid=next_pid++;
            p->ppid=current?current->pid:0;
            p->uid=0; p->gid=0;
            p->state=PROC_READY;
            p->waited=0;
            for(int j=0;j<31&&name[j];j++) p->name[j]=name[j];
            for(int j=0;j<PROC_FD_MAX;j++) p->fds[j].used=false;
            return p;
        }
    }
    return NULL;
}

proc_t *proc_get(int pid){ for(int i=0;i<MAX_PROC;i++) if(procs[i].state!=PROC_UNUSED && procs[i].pid==pid) return &procs[i]; return NULL; }

void proc_exit(int code){
    if(!current) return;
    current->exit_code=code;
    current->state=PROC_ZOMBIE;
    log_printf("[proc] pid=%d exit=%d\n", current->pid, code);
}

int proc_kill(int pid, int sig){
    proc_t *p=proc_get(pid); if(!p) return -1;
    if(sig==9 || sig==15){ p->state=PROC_ZOMBIE; p->exit_code=128+sig; return 0; }
    return 0;
}

void proc_ps(void){
    log_write("PID PPID STATE    NAME\n");
    for(int i=0;i<MAX_PROC;i++){
        proc_t *p=&procs[i]; if(p->state==PROC_UNUSED) continue;
        const char *st = p->state==PROC_READY?"READY":p->state==PROC_RUNNING?"RUN":p->state==PROC_SLEEPING?"SLEEP":"ZOMB";
        log_printf("%d   %d    %s  %s%s\n", p->pid, p->ppid, st, p->name, p==current?" *":"");
    }
}

int proc_exec(const char *path, char *const argv[]){
    (void)argv;
    vfs_node_t *f=vfs_lookup(path);
    if(!f || f->type!=VFS_FILE || !f->data || f->size==0){ log_write("exec: not found\n"); return -1; }
    proc_t *p=proc_alloc(path);
    if(!p){ log_write("exec: no proc\n"); return -1; }
    u64 cr3 = vmm_new_address_space();
    if(!cr3){ log_write("exec: no cr3\n"); p->state=PROC_UNUSED; return -1; }
    p->cr3 = cr3;
    u64 *pml4_root = (u64*)(cr3 + boot_info_get()->hhdm_offset);
    u64 entry=0;
    if(!elf_load(f->data, f->size, &entry, pml4_root)){ log_write("exec: bad elf\n"); p->state=PROC_UNUSED; return -1; }
    /* allocate user stack */
    u64 ustack_top = 0x00007FFFFFFFE000ULL;
    u64 hhdm = boot_info_get()->hhdm_offset;
    for(int i=0;i<16;i++){
        void *pg=pmm_alloc_page(); if(!pg){ p->state=PROC_UNUSED; return -1; }
        u64 phys=(u64)pg-hhdm;
        vmm_map_to(pml4_root, ustack_top - (u64)(i+1)*0x1000, phys, 0x7);
    }
    p->entry=entry;
    p->user_rsp = ustack_top - 8;
    p->brk_base = 0x0000000040000000ULL;
    p->brk = p->brk_base;
    /* kernel stack for syscalls */
    u8 *kstack = (u8*)kmalloc(16384);
    if(!kstack){ p->state=PROC_UNUSED; return -1; }
    p->kernel_rsp = ((u64)kstack + 16384) & ~0xFULL;
    p->state=PROC_RUNNING;
    current=p;
    u64 old_cr3 = vmm_get_pml4();
    tss_set_rsp0(p->kernel_rsp);
    syscall_set_kernel_rsp(p->kernel_rsp);
    vmm_switch(p->cr3);
    log_printf("[proc] exec %s pid=%d entry=%lx\n", path, p->pid, entry);
    enter_user(entry, p->user_rsp);
    /* returned via syscall exit */
    vmm_switch(old_cr3);
    if(current==p && p->state==PROC_RUNNING) p->state=PROC_ZOMBIE;
    current=NULL;
    return p->exit_code;
}


int proc_fork(void){ return -38; /* ENOSYS - needs preemptive user scheduling */ }

int proc_execve(const char *path, char *const argv[], char *const envp[]){
    (void)envp;
    return proc_exec(path, (char*const*)argv);
}

i64 proc_wait4(int pid, int *status, int options){
    (void)options;
    for(int i=0;i<MAX_PROC;i++){
        proc_t *p=&procs[i];
        if(p->state==PROC_ZOMBIE && (pid==-1 || p->pid==pid || (pid==0 && p->ppid==(current?current->pid:0)))){
            if(status) *status = (p->exit_code & 0xff) << 8;
            int rpid=p->pid; p->state=PROC_UNUSED; p->waited=1; return rpid;
        }
    }
    return -10; /* ECHILD/no child */
}
