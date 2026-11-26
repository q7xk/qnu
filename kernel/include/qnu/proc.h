#ifndef QNU_PROC_H
#define QNU_PROC_H
#include <qnu/types.h>
#include <qnu/vfs.h>
#define MAX_PROC 16
#define PROC_FD_MAX 32
typedef enum { PROC_UNUSED=0, PROC_READY, PROC_RUNNING, PROC_SLEEPING, PROC_ZOMBIE } proc_state_t;
typedef struct {
    vfs_node_t *node;
    u64 off;
    bool used;
} proc_fd_t;
typedef struct proc {
    int pid;
    int ppid;
    u32 uid,gid;
    proc_state_t state;
    char name[32];
    u64 entry;
    u64 user_rsp;
    u64 kernel_rsp;
    u64 brk;
    u64 brk_base;
    u64 cr3;
    proc_fd_t fds[PROC_FD_MAX];
    int exit_code;
    int waited;
} proc_t;
void proc_init(void);
proc_t *proc_current(void);
proc_t *proc_alloc(const char *name);
proc_t *proc_get(int pid);
int proc_exec(const char *path, char *const argv[]);
void proc_exit(int code);
int proc_fork(void);
int proc_execve(const char *path, char *const argv[], char *const envp[]);
i64 proc_wait4(int pid, int *status, int options);
int proc_kill(int pid, int sig);
void proc_ps(void);
void proc_set_current(proc_t *p);
#endif
