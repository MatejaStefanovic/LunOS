#ifndef __KERNEL_TASK_MANAGER_H
#define __KERNEL_TASK_MANAGER_H

#include <kernel/scheduler.h>
#include <kernel/tasks.h>
#include <kernel/apic.h>

extern int cpu_id;

struct task* create_and_schedule_kernel_task(void (*func)(void));
void init_task_ctr(int cpu_count);
void task_exit(int exit_code);
void wake_up_task(struct task* task);

#endif
