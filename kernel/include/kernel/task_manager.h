#ifndef __KERNEL_TASK_MANAGER_H
#define __KERNEL_TASK_MANAGER_H

#include <kernel/scheduler.h>
#include <kernel/tasks.h>

struct task* create_and_schedule_kernel_task(void (*func)(void));
void run_kernel_task(struct task* t);

#endif
