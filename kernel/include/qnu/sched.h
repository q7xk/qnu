#ifndef QNU_SCHED_H
#define QNU_SCHED_H
#include <qnu/types.h>
typedef enum { TASK_READY=0, TASK_RUNNING, TASK_SLEEPING, TASK_DEAD } task_state_t;
void sched_init(void);
void sched_start(void);
void sched_tick(void);
void sched_yield(void);
int sched_create_kernel_thread(const char *name, void (*entry)(void*), void *arg);
#endif
