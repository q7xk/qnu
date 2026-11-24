#include <qnu/sched.h>
#include <qnu/mm.h>
#include <qnu/log.h>
#include <qnu/panic.h>

#define MAX_TASKS 32
#define STACK_SIZE 16384

typedef struct task {
    u64 rsp;
    u64 stack_base;
    task_state_t state;
    char name[32];
    struct task *next;
} task_t;

static task_t *current;
static task_t *ready_queue;
static task_t tasks[MAX_TASKS];
static int task_count;
static bool sched_enabled = false;

extern void context_switch(u64 *old_rsp, u64 new_rsp);
void sched_yield(void);

static void task_trampoline(void) {
    void (*entry)(void*);
    void *arg;
    __asm__ volatile ("mov %%r12, %0" : "=r"(entry));
    __asm__ volatile ("mov %%r13, %0" : "=r"(arg));
    __asm__ volatile ("sti");
    entry(arg);
    current->state = TASK_DEAD;
    sched_yield();
    for (;;) { __asm__ volatile ("hlt"); }
}

void sched_init(void) {
    for (int i = 0; i < MAX_TASKS; i++) { tasks[i].state = TASK_DEAD; tasks[i].next = NULL; }
    task_count = 0;
    current = &tasks[0];
    current->state = TASK_RUNNING;
    current->stack_base = 0;
    current->rsp = 0;
    const char *n = "kmain";
    for (int i = 0; i < 31 && n[i]; i++) current->name[i] = n[i];
    current->name[31] = 0;
    current->next = current;
    ready_queue = current;
    task_count = 1;
    sched_enabled = true;
    log_ok("Scheduler initialized");
}

int sched_create_kernel_thread(const char *name, void (*entry)(void*), void *arg) {
    int idx = -1;
    for (int i = 1; i < MAX_TASKS; i++) if (tasks[i].state == TASK_DEAD) { idx = i; break; }
    if (idx < 0) return -1;
    task_t *t = &tasks[idx];
    u8 *stack = (u8 *)kmalloc(STACK_SIZE);
    if (!stack) return -1;
    t->stack_base = (u64)stack;
    u64 sp = ((u64)stack + STACK_SIZE) & ~0xFULL;
    /* build initial stack for context_switch: r15,r14,r13,r12,rbx,rbp,ret */
    sp -= 8; *(u64*)sp = (u64)task_trampoline; /* ret */
    sp -= 8; *(u64*)sp = 0; /* rbp */
    sp -= 8; *(u64*)sp = 0; /* rbx */
    sp -= 8; *(u64*)sp = (u64)entry; /* r12 - we'll use registers? easier: set up via trampoline args */
    sp -= 8; *(u64*)sp = (u64)arg;   /* r13 */
    sp -= 8; *(u64*)sp = 0; /* r14 */
    sp -= 8; *(u64*)sp = 0; /* r15 */
    /* We need trampoline to get entry/arg. Simpler: put entry in r12, arg in r13 and use a wrapper */
    t->rsp = sp;
    t->state = TASK_READY;
    for (int i = 0; i < 31 && name[i]; i++) t->name[i] = name[i];
    t->name[31] = 0;
    /* insert into round-robin */
    t->next = current->next;
    current->next = t;
    task_count++;
    return idx;
}

void sched_yield(void) {
    if (!sched_enabled || !current) return;
    task_t *next = current->next;
    for (int i = 0; i < MAX_TASKS + 1 && next; i++) {
        if (next->state == TASK_READY || next->state == TASK_RUNNING) break;
        next = next->next;
    }
    if (!next || next == current) return;
    if (current->state == TASK_RUNNING) current->state = TASK_READY;
    next->state = TASK_RUNNING;
    task_t *prev = current;
    current = next;
    context_switch(&prev->rsp, next->rsp);
}

void sched_tick(void) {
    if (!sched_enabled) return;
    sched_yield();
}

void sched_start(void) {
    /* scheduler is already active; nothing extra needed for 0.0.1 */
}
